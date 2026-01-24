/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24 20:00:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-24 20:00:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\display_manager.c
 * @Description: 显示管理器实现 - 负责 UI 业务逻辑和事件响应
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include "display_manager.h"
#include "xn_display.h"
#include "xn_event_bus.h"
#include "xn_event_types.h"
#include "ui_init.h"
#include "ui_home.h"
#include "ui_wifi.h"
#include "ui_status.h"
#include "ui_settings.h"
#include "ui_ota.h"
#include "esp_log.h"

static const char *TAG = "display_mgr";

/*===========================================================================
 *                          内部数据结构
 *===========================================================================*/

typedef struct {
    bool initialized;                   ///< 初始化标志
    ui_page_t current_page;             ///< 当前页面
    xn_event_subscriber_t event_sub;    ///< 事件订阅者
} display_manager_ctx_t;

static display_manager_ctx_t s_ctx = {0};

/*===========================================================================
 *                          内部函数声明
 *===========================================================================*/

static void on_event_received(uint16_t event_id, void *event_data, void *user_data);
static void handle_wifi_event(uint16_t event_id, void *event_data);
static void handle_mqtt_event(uint16_t event_id, void *event_data);
static void handle_system_event(uint16_t event_id, void *event_data);

/*===========================================================================
 *                          API 实现
 *===========================================================================*/

esp_err_t display_manager_init(void)
{
    if (s_ctx.initialized) {
        ESP_LOGW(TAG, "Display manager already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing display manager...");
    
    // 1. 初始化 xn_display 组件
    xn_display_config_t config = xn_display_get_default_config();
    
    // 根据你的硬件配置
    config.lcd_type = XN_DISPLAY_LCD_ST7789;
    config.width = 240;
    config.height = 320;
    config.spi_host = SPI2_HOST;
    config.pin_mosi = GPIO_NUM_47;
    config.pin_sclk = GPIO_NUM_48;
    config.pin_cs = GPIO_NUM_NC;
    config.pin_dc = GPIO_NUM_21;
    config.pin_rst = GPIO_NUM_14;
    config.pin_bckl = GPIO_NUM_45;
    config.spi_clk_hz = 20 * 1000 * 1000;
    config.spi_mode = 3;
    config.mirror_x = true;
    config.mirror_y = false;
    config.swap_xy = true;
    config.invert_color = true;
    config.rgb_order = XN_DISPLAY_RGB_ORDER_RGB;
    config.offset_x = 0;
    config.offset_y = 0;
    config.backlight_output_invert = false;
    
    esp_err_t ret = xn_display_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 2. 初始化 UI 系统
    ESP_LOGI(TAG, "Initializing UI...");
    ret = ui_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UI: %s", esp_err_to_name(ret));
        xn_display_deinit();
        return ret;
    }
    
    // 3. 创建所有 UI 页面
    ESP_LOGI(TAG, "Creating UI pages...");
    ui_home_create();
    ui_wifi_create();
    ui_status_create();
    ui_settings_create();
    ui_ota_create();
    
    // 4. 订阅事件总线
    s_ctx.event_sub.callback = on_event_received;
    s_ctx.event_sub.user_data = NULL;
    ret = xn_event_subscribe(XN_EVT_ANY, &s_ctx.event_sub);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to subscribe to events: %s", esp_err_to_name(ret));
    }
    
    // 5. 显示主页面
    s_ctx.current_page = UI_PAGE_HOME;
    display_manager_show_page(UI_PAGE_HOME);
    
    s_ctx.initialized = true;
    
    ESP_LOGI(TAG, "Display manager initialized successfully");
    
    return ESP_OK;
}

esp_err_t display_manager_deinit(void)
{
    if (!s_ctx.initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing display manager...");
    
    // 取消订阅事件
    xn_event_unsubscribe(XN_EVT_ANY, &s_ctx.event_sub);
    
    // 反初始化显示
    xn_display_deinit();
    
    s_ctx.initialized = false;
    
    ESP_LOGI(TAG, "Display manager deinitialized");
    
    return ESP_OK;
}

esp_err_t display_manager_show_page(ui_page_t page)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (page >= UI_PAGE_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Switching to page: %d", page);
    
    // 锁定 LVGL
    if (!xn_display_lock(1000)) {
        ESP_LOGE(TAG, "Failed to lock display");
        return ESP_ERR_TIMEOUT;
    }
    
    // 切换页面
    switch (page) {
        case UI_PAGE_HOME:
            ui_home_show();
            break;
        case UI_PAGE_WIFI:
            ui_wifi_show();
            break;
        case UI_PAGE_STATUS:
            ui_status_show();
            break;
        case UI_PAGE_SETTINGS:
            ui_settings_show();
            break;
        case UI_PAGE_OTA:
            ui_ota_show();
            break;
        case UI_PAGE_ERROR:
            // 错误页面通过 display_manager_show_error() 显示
            break;
        default:
            break;
    }
    
    s_ctx.current_page = page;
    
    // 解锁 LVGL
    xn_display_unlock();
    
    return ESP_OK;
}

esp_err_t display_manager_update_home(
    app_state_t state,
    const char *wifi_ssid,
    int8_t wifi_rssi,
    uint32_t ip_addr,
    bool mqtt_connected)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 锁定 LVGL
    if (!xn_display_lock(1000)) {
        return ESP_ERR_TIMEOUT;
    }
    
    // 更新主页面数据
    ui_home_update(state, wifi_ssid, wifi_rssi, ip_addr, mqtt_connected);
    
    // 解锁 LVGL
    xn_display_unlock();
    
    return ESP_OK;
}

esp_err_t display_manager_update_wifi(
    const char *ssid,
    int8_t rssi,
    const char *status)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 锁定 LVGL
    if (!xn_display_lock(1000)) {
        return ESP_ERR_TIMEOUT;
    }
    
    // 更新 WiFi 页面数据
    ui_wifi_update(ssid, rssi, status);
    
    // 解锁 LVGL
    xn_display_unlock();
    
    return ESP_OK;
}

esp_err_t display_manager_update_ota(
    uint8_t progress,
    const char *status)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 锁定 LVGL
    if (!xn_display_lock(1000)) {
        return ESP_ERR_TIMEOUT;
    }
    
    // 更新 OTA 页面数据
    ui_ota_update(progress, status);
    
    // 解锁 LVGL
    xn_display_unlock();
    
    return ESP_OK;
}

esp_err_t display_manager_show_error(const char *error_msg)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGE(TAG, "Showing error: %s", error_msg);
    
    // 锁定 LVGL
    if (!xn_display_lock(1000)) {
        return ESP_ERR_TIMEOUT;
    }
    
    // 显示错误消息（可以创建一个错误对话框）
    // 这里简化为在主页面显示
    lv_obj_t *mbox = lv_msgbox_create(NULL, "错误", error_msg, NULL, true);
    lv_obj_center(mbox);
    
    // 解锁 LVGL
    xn_display_unlock();
    
    return ESP_OK;
}

esp_err_t display_manager_show_toast(const char *msg, uint32_t duration_ms)
{
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 锁定 LVGL
    if (!xn_display_lock(1000)) {
        return ESP_ERR_TIMEOUT;
    }
    
    // 创建 Toast 消息（简化实现）
    lv_obj_t *toast = lv_label_create(lv_scr_act());
    lv_label_set_text(toast, msg);
    lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(toast, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_80, 0);
    lv_obj_set_style_text_color(toast, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_all(toast, 10, 0);
    
    // 设置自动删除（需要使用定时器）
    // 这里简化处理，实际应该使用 lv_anim 或定时器
    
    // 解锁 LVGL
    xn_display_unlock();
    
    return ESP_OK;
}

esp_err_t display_manager_set_brightness(uint8_t brightness)
{
    return xn_display_set_brightness(brightness);
}

/*===========================================================================
 *                          内部函数实现
 *===========================================================================*/

/**
 * @brief 事件回调函数
 */
static void on_event_received(uint16_t event_id, void *event_data, void *user_data)
{
    // 根据事件类别分发处理
    if (event_id >= XN_EVT_CAT_WIFI && event_id < XN_EVT_CAT_BLUFI) {
        handle_wifi_event(event_id, event_data);
    } else if (event_id >= XN_EVT_CAT_MQTT && event_id < XN_EVT_CAT_BUTTON) {
        handle_mqtt_event(event_id, event_data);
    } else if (event_id >= XN_EVT_CAT_SYSTEM && event_id < XN_EVT_CAT_WIFI) {
        handle_system_event(event_id, event_data);
    }
}

/**
 * @brief 处理 WiFi 事件
 */
static void handle_wifi_event(uint16_t event_id, void *event_data)
{
    switch (event_id) {
        case XN_EVT_WIFI_CONNECTED: {
            xn_evt_wifi_connected_t *data = (xn_evt_wifi_connected_t *)event_data;
            ESP_LOGI(TAG, "WiFi connected: %s", data->ssid);
            display_manager_update_wifi((char *)data->ssid, data->rssi, "已连接");
            break;
        }
        
        case XN_EVT_WIFI_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi disconnected");
            display_manager_update_wifi("", 0, "未连接");
            break;
        
        case XN_EVT_WIFI_GOT_IP: {
            xn_evt_wifi_got_ip_t *data = (xn_evt_wifi_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IP: %d.%d.%d.%d", 
                     (data->ip >> 0) & 0xFF,
                     (data->ip >> 8) & 0xFF,
                     (data->ip >> 16) & 0xFF,
                     (data->ip >> 24) & 0xFF);
            // 更新主页面
            display_manager_update_home(
                app_state_machine_get_state(),
                NULL, 0, data->ip, false
            );
            break;
        }
        
        default:
            break;
    }
}

/**
 * @brief 处理 MQTT 事件
 */
static void handle_mqtt_event(uint16_t event_id, void *event_data)
{
    switch (event_id) {
        case XN_EVT_MQTT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            display_manager_show_toast("MQTT 已连接", 2000);
            break;
        
        case XN_EVT_MQTT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            display_manager_show_toast("MQTT 已断开", 2000);
            break;
        
        default:
            break;
    }
}

/**
 * @brief 处理系统事件
 */
static void handle_system_event(uint16_t event_id, void *event_data)
{
    switch (event_id) {
        case XN_EVT_SYSTEM_READY:
            ESP_LOGI(TAG, "System ready");
            display_manager_show_toast("系统就绪", 2000);
            break;
        
        case XN_EVT_SYSTEM_ERROR:
            ESP_LOGE(TAG, "System error");
            display_manager_show_error("系统错误");
            break;
        
        default:
            break;
    }
}
