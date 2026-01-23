/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-23 09:48:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 09:48:43
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\button_manager.c
 * @Description: 按键管理器实现 - Boot按键长按检测
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "xn_event_bus.h"
#include "button_manager.h"

static const char *TAG = "button_manager";

// 配置参数
#define BUTTON_GPIO             GPIO_NUM_0      // Boot按键 GPIO
#define BUTTON_ACTIVE_LEVEL     0               // 按下电平 (0为按下)
#define SCAN_INTERVAL_MS        50              // 扫描周期 (ms)
#define LONG_PRESS_TIME_MS      1000            // 长按判定时间 (ms)
#define DEBOUNCE_TICKS          (SCAN_INTERVAL_MS / portTICK_PERIOD_MS)

// 内部状态
static bool s_initialized = false;

/**
 * @brief 按键扫描任务
 */
static void button_scan_task(void *arg)
{
    uint32_t press_duration = 0;
    bool long_press_triggered = false;

    while (1) {
        // 读取按键电平
        int level = gpio_get_level(BUTTON_GPIO);

        if (level == BUTTON_ACTIVE_LEVEL) {
            // 按键按下
            press_duration += SCAN_INTERVAL_MS;

            // 检查是否达到长按阈值
            if (press_duration >= LONG_PRESS_TIME_MS && !long_press_triggered) {
                ESP_LOGI(TAG, "Button long press detected (%d ms)", press_duration);
                // 触发长按事件
                xn_event_post(XN_EVT_BUTTON_LONG_PRESS, XN_EVT_SRC_BUTTON);
                // 标记已触发，避免重复触发
                long_press_triggered = true;
            }
        } else {
            // 按键释放
            if (press_duration > 0) {
                // 如果需要单击事件，可以在这里判断 press_duration < LONG_PRESS_TIME_MS
                ESP_LOGD(TAG, "Button released (held for %d ms)", press_duration);
            }
            // 重置计数器和标志位
            press_duration = 0;
            long_press_triggered = false;
        }

        // 延时等待下一次扫描
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}

esp_err_t button_manager_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    // 配置 GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,       // 启用内部上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        return ret;
    }

    // 创建扫描任务
    // 堆栈大小 2048 字节，优先级 10 (由系统具体情况调整)
    BaseType_t task_ret = xTaskCreate(button_scan_task, "btn_scan", 2048, NULL, 10, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create scan task");
        return ESP_FAIL;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Button manager initialized (GPIO %d)", BUTTON_GPIO);
    
    return ESP_OK;
}
