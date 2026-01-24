/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24 20:00:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-24 20:00:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_display\src\xn_display.c
 * @Description: 显示组件实现 - LVGL + LCD 驱动适配层
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include "xn_display.h"
#include "xn_display_lcd.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"

static const char *TAG = "xn_display";

/*===========================================================================
 *                          内部数据结构
 *===========================================================================*/

typedef struct {
    bool initialized;                       ///< 初始化标志
    xn_display_config_t config;             ///< 配置信息
    
    // LVGL 相关 (LVGL 9.x API)
    lv_display_t *disp;                     ///< LVGL 显示对象 (LVGL 9.x: lv_disp_t → lv_display_t)
    lv_color_t *buf1;                       ///< 缓冲区1
    lv_color_t *buf2;                       ///< 缓冲区2
    SemaphoreHandle_t lvgl_mutex;           ///< LVGL 互斥锁
    TaskHandle_t lvgl_task_handle;          ///< LVGL 任务句柄
    esp_timer_handle_t lvgl_tick_timer;     ///< LVGL tick 定时器
    
    // LCD 相关
    esp_lcd_panel_handle_t panel_handle;    ///< LCD 面板句柄
    esp_lcd_panel_io_handle_t io_handle;    ///< LCD IO 句柄
    
    // 背光相关
    ledc_channel_t backlight_channel;       ///< 背光 PWM 通道
    uint8_t current_brightness;             ///< 当前亮度
    bool is_sleeping;                       ///< 休眠状态
} xn_display_ctx_t;

static xn_display_ctx_t s_ctx = {0};

/*===========================================================================
 *                          内部函数声明
 *===========================================================================*/

static void lvgl_tick_timer_cb(void *arg);
static void lvgl_task(void *arg);
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);  // LVGL 9.x API
static esp_err_t backlight_init(void);
static esp_err_t backlight_set_duty(uint8_t brightness);

/*===========================================================================
 *                          API 实现
 *===========================================================================*/

xn_display_config_t xn_display_get_default_config(void)
{
    xn_display_config_t config = {
        .lcd_type = XN_DISPLAY_LCD_ST7789,
        .width = 320,
        .height = 240,
        
        .spi_host = 1,  // SPI2_HOST
        .pin_mosi = GPIO_NUM_47,
        .pin_sclk = GPIO_NUM_48,
        .pin_cs = GPIO_NUM_NC,
        .pin_dc = GPIO_NUM_21,
        .pin_rst = GPIO_NUM_14,
        .pin_bckl = GPIO_NUM_45,
        .spi_clk_hz = 20 * 1000 * 1000,
        .spi_mode = 3,
        
        .mirror_x = true,
        .mirror_y = false,
        .swap_xy = true,
        .invert_color = true,
        .rgb_order = XN_DISPLAY_RGB_ORDER_RGB,
        .offset_x = 0,
        .offset_y = 0,
        .backlight_output_invert = false,
        
        .lvgl_tick_period_ms = 5,
        .lvgl_task_stack_size = 4096,
        .lvgl_task_priority = 5,
        .lvgl_buffer_size = 0,  // 0 表示使用默认值 width * 10
    };
    return config;
}

esp_err_t xn_display_init(const xn_display_config_t *config)
{
    if (s_ctx.initialized) {
        ESP_LOGW(TAG, "Display already initialized");
        return ESP_OK;
    }
    
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing display...");
    
    // 保存配置
    memcpy(&s_ctx.config, config, sizeof(xn_display_config_t));
    
    // 设置默认缓冲区大小
    if (s_ctx.config.lvgl_buffer_size == 0) {
        s_ctx.config.lvgl_buffer_size = s_ctx.config.width * 10;
    }
    
    esp_err_t ret;
    
    // 1. 初始化 LCD 驱动
    ESP_LOGI(TAG, "Initializing LCD driver (ST7789)...");
    ret = lcd_st7789_init(config, &s_ctx.panel_handle, &s_ctx.io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD driver: %s", esp_err_to_name(ret));
        goto err;
    }
    
    // 2. 初始化背光
    ESP_LOGI(TAG, "Initializing backlight...");
    ret = backlight_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize backlight: %s", esp_err_to_name(ret));
        goto err;
    }
    
    // 3. 初始化 LVGL
    ESP_LOGI(TAG, "Initializing LVGL...");
    lv_init();
    
    // 4. 创建 LVGL 互斥锁
    s_ctx.lvgl_mutex = xSemaphoreCreateMutex();
    if (s_ctx.lvgl_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        ret = ESP_ERR_NO_MEM;
        goto err;
    }
    
    // 5. 分配显示缓冲区
    size_t buf_size = s_ctx.config.lvgl_buffer_size * sizeof(lv_color_t);
    s_ctx.buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    s_ctx.buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (s_ctx.buf1 == NULL || s_ctx.buf2 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate display buffer");
        ret = ESP_ERR_NO_MEM;
        goto err;
    }
    
    // 6. 创建显示对象 (LVGL 9.x 新 API)
    s_ctx.disp = lv_display_create(s_ctx.config.width, s_ctx.config.height);
    if (s_ctx.disp == NULL) {
        ESP_LOGE(TAG, "Failed to create display");
        ret = ESP_FAIL;
        goto err;
    }
    
    // 7. 设置显示缓冲区 (LVGL 9.x 新 API)
    lv_display_set_buffers(s_ctx.disp, s_ctx.buf1, s_ctx.buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // 8. 设置刷新回调 (LVGL 9.x 新 API)
    lv_display_set_flush_cb(s_ctx.disp, lvgl_flush_cb);
    
    // 9. 创建 LVGL tick 定时器
    const esp_timer_create_args_t timer_args = {
        .callback = lvgl_tick_timer_cb,
        .name = "lvgl_tick"
    };
    ret = esp_timer_create(&timer_args, &s_ctx.lvgl_tick_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LVGL tick timer: %s", esp_err_to_name(ret));
        goto err;
    }
    
    ret = esp_timer_start_periodic(s_ctx.lvgl_tick_timer, s_ctx.config.lvgl_tick_period_ms * 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LVGL tick timer: %s", esp_err_to_name(ret));
        goto err;
    }
    
    // 10. 创建 LVGL 任务
    BaseType_t task_ret = xTaskCreate(
        lvgl_task,
        "lvgl_task",
        s_ctx.config.lvgl_task_stack_size,
        NULL,
        s_ctx.config.lvgl_task_priority,
        &s_ctx.lvgl_task_handle
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        ret = ESP_ERR_NO_MEM;
        goto err;
    }
    
    // 11. 设置默认亮度
    xn_display_set_brightness(80);
    
    s_ctx.initialized = true;
    s_ctx.is_sleeping = false;
    
    ESP_LOGI(TAG, "Display initialized successfully (LCD: %dx%d)", 
             s_ctx.config.width, s_ctx.config.height);
    
    return ESP_OK;
    
err:
    xn_display_deinit();
    return ret;
}

esp_err_t xn_display_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing display...");
    
    // 停止 LVGL tick 定时器
    if (s_ctx.lvgl_tick_timer) {
        esp_timer_stop(s_ctx.lvgl_tick_timer);
        esp_timer_delete(s_ctx.lvgl_tick_timer);
        s_ctx.lvgl_tick_timer = NULL;
    }
    
    // 删除 LVGL 任务
    if (s_ctx.lvgl_task_handle) {
        vTaskDelete(s_ctx.lvgl_task_handle);
        s_ctx.lvgl_task_handle = NULL;
    }
    
    // 释放显示缓冲区
    if (s_ctx.buf1) {
        free(s_ctx.buf1);
        s_ctx.buf1 = NULL;
    }
    if (s_ctx.buf2) {
        free(s_ctx.buf2);
        s_ctx.buf2 = NULL;
    }
    
    // 删除互斥锁
    if (s_ctx.lvgl_mutex) {
        vSemaphoreDelete(s_ctx.lvgl_mutex);
        s_ctx.lvgl_mutex = NULL;
    }
    
    // 释放 LCD 资源
    if (s_ctx.panel_handle) {
        esp_lcd_panel_del(s_ctx.panel_handle);
        s_ctx.panel_handle = NULL;
    }
    if (s_ctx.io_handle) {
        esp_lcd_panel_io_del(s_ctx.io_handle);
        s_ctx.io_handle = NULL;
    }
    
    s_ctx.initialized = false;
    
    ESP_LOGI(TAG, "Display deinitialized");
    
    return ESP_OK;
}

esp_err_t xn_display_set_brightness(uint8_t brightness)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (brightness > 100) {
        brightness = 100;
    }
    
    s_ctx.current_brightness = brightness;
    
    return backlight_set_duty(brightness);
}

esp_err_t xn_display_sleep(bool sleep)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (sleep == s_ctx.is_sleeping) {
        return ESP_OK;
    }
    
    if (sleep) {
        // 进入休眠：关闭背光和显示
        ESP_LOGI(TAG, "Display entering sleep mode");
        backlight_set_duty(0);
        esp_lcd_panel_disp_on_off(s_ctx.panel_handle, false);
    } else {
        // 唤醒：恢复显示和背光
        ESP_LOGI(TAG, "Display waking up");
        esp_lcd_panel_disp_on_off(s_ctx.panel_handle, true);
        backlight_set_duty(s_ctx.current_brightness);
    }
    
    s_ctx.is_sleeping = sleep;
    
    return ESP_OK;
}

lv_display_t* xn_display_get_disp(void)  // LVGL 9.x: lv_disp_t → lv_display_t
{
    return s_ctx.disp;
}

bool xn_display_lock(uint32_t timeout_ms)
{
    if (s_ctx.lvgl_mutex == NULL) {
        return false;
    }
    
    return xSemaphoreTake(s_ctx.lvgl_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void xn_display_unlock(void)
{
    if (s_ctx.lvgl_mutex) {
        xSemaphoreGive(s_ctx.lvgl_mutex);
    }
}

/*===========================================================================
 *                          内部函数实现
 *===========================================================================*/

/**
 * @brief LVGL tick 定时器回调
 */
static void lvgl_tick_timer_cb(void *arg)
{
    lv_tick_inc(s_ctx.config.lvgl_tick_period_ms);
}

/**
 * @brief LVGL 任务
 */
static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started");
    
    while (1) {
        // 锁定 LVGL
        if (xn_display_lock(portMAX_DELAY)) {
            // 处理 LVGL 任务
            uint32_t task_delay_ms = lv_timer_handler();
            
            // 解锁 LVGL
            xn_display_unlock();
            
            // 延时
            if (task_delay_ms > 500) {
                task_delay_ms = 500;
            }
            vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
        }
    }
}

/**
 * @brief LVGL 刷新回调 (LVGL 9.x 新 API)
 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    // 绘制位图到 LCD
    esp_lcd_panel_draw_bitmap(s_ctx.panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
    
    // 通知 LVGL 刷新完成 (LVGL 9.x 新 API)
    lv_display_flush_ready(disp);
}

/**
 * @brief 初始化背光 PWM
 */
static esp_err_t backlight_init(void)
{
    if (s_ctx.config.pin_bckl == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "Backlight pin not configured");
        return ESP_OK;
    }
    
    // 配置 LEDC 定时器
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 配置 LEDC 通道
    s_ctx.backlight_channel = LEDC_CHANNEL_0;
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = s_ctx.backlight_channel,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = s_ctx.config.pin_bckl,
        .duty = 0,
        .hpoint = 0
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        return ret;
    }
    
    return ESP_OK;
}

/**
 * @brief 设置背光 PWM 占空比
 */
static esp_err_t backlight_set_duty(uint8_t brightness)
{
    if (s_ctx.config.pin_bckl == GPIO_NUM_NC) {
        return ESP_OK;
    }
    
    // 将 0-100 映射到 0-255
    uint32_t duty = (brightness * 255) / 100;
    
    // 如果背光输出反转，则反转占空比
    if (s_ctx.config.backlight_output_invert) {
        duty = 255 - duty;
    }
    
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, s_ctx.backlight_channel, duty);
    if (ret != ESP_OK) {
        return ret;
    }
    
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, s_ctx.backlight_channel);
}
