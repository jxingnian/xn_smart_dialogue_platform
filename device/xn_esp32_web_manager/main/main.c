/**
 * @file main.c
 * @brief 应用程序入口
 * 
 * 初始化系统组件和模块，启动顶层状态机
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "xn_event_bus.h"
#include "app_state_machine.h"
#include "managers/wifi_manager.h"
#include "managers/mqtt_manager.h"
#include "managers/blufi_manager.h"

static const char *TAG = "main";

/**
 * @brief 初始化NVS
 */
static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief 应用程序入口
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  XN Smart Dialogue Platform Starting");
    ESP_LOGI(TAG, "========================================");
    
    // 1. 初始化NVS
    ESP_ERROR_CHECK(init_nvs());
    ESP_LOGI(TAG, "NVS initialized");
    
    // 2. 创建默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "Event loop created");
    
    // 3. 初始化事件总线
    ESP_ERROR_CHECK(xn_event_bus_init());
    ESP_LOGI(TAG, "Event bus initialized");
    
    // 4. 初始化各Manager
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_LOGI(TAG, "WiFi manager initialized");
    
    ESP_ERROR_CHECK(mqtt_manager_init());
    ESP_LOGI(TAG, "MQTT manager initialized");
    
    ESP_ERROR_CHECK(blufi_manager_init());
    ESP_LOGI(TAG, "BluFi manager initialized");
    
    // 5. 初始化并启动顶层状态机
    ESP_ERROR_CHECK(app_state_machine_init());
    ESP_ERROR_CHECK(app_state_machine_start());
    ESP_LOGI(TAG, "App state machine started");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  System initialization complete!");
    ESP_LOGI(TAG, "========================================");
    
    // 主循环（可选，用于周期性任务）
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "System state: %s", app_state_machine_get_state_name());
    }
}
