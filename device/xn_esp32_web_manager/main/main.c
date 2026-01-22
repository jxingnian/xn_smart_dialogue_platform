/**
 * @file main.c
 * @brief 应用程序入口
 * 
 * 初始化系统组件和模块，启动顶层状态机。
 * 按顺序初始化: NVS -> EventLoop -> EventBus -> Managers -> AppStateMachine
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

// 模块日志标签
static const char *TAG = "main";

/**
 * @brief 初始化非易失性存储(NVS)
 * 
 * NVS用于存储WiFi配置、MQTT配置等掉电保存参数。
 * 如果NVS分区满或版本不匹配，会自动擦除重置。
 * 
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
static esp_err_t init_nvs(void)
{
    // 尝试初始化NVS Flash
    esp_err_t ret = nvs_flash_init();
    
    // 如果没有空闲页或发现新版本，需要擦除NVS分区后重试
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash erase and init...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief 应用程序主入口
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  XN Smart Dialogue Platform Starting");
    ESP_LOGI(TAG, "========================================");
    
    // 1. 初始化NVS子系统
    // 必须最先初始化，后续的WiFi/BluFi等模块都依赖NVS读取配置
    ESP_ERROR_CHECK(init_nvs());
    ESP_LOGI(TAG, "NVS initialized");
    
    // 2. 创建系统默认事件循环
    // ESP-IDF的WiFi、IP等底层事件都通过此默认循环分发
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "Event loop created");
    
    // 3. 初始化自定义事件总线
    // 用于模块间（Managers、StateMachine）的解耦通信
    ESP_ERROR_CHECK(xn_event_bus_init());
    ESP_LOGI(TAG, "Event bus initialized");
    
    // 4. 初始化各功能管理器 (Managers)
    // 此时仅初始化资源和注册事件回调，不立即启动业务（如不立即连接WiFi）
    
    // 4.1 初始化WiFi管理器
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_LOGI(TAG, "WiFi manager initialized");
    
    // 4.2 初始化MQTT管理器
    ESP_ERROR_CHECK(mqtt_manager_init());
    ESP_LOGI(TAG, "MQTT manager initialized");
    
    // 4.3 初始化BluFi配网管理器
    ESP_ERROR_CHECK(blufi_manager_init());
    ESP_LOGI(TAG, "BluFi manager initialized");
    
    // 5. 初始化并启动顶层状态机
    // 状态机负责协调各Manager的工作流程（如先配网，再连WiFi，再连MQTT）
    ESP_ERROR_CHECK(app_state_machine_init());
    ESP_ERROR_CHECK(app_state_machine_start());
    ESP_LOGI(TAG, "App state machine started");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  System initialization complete!");
    ESP_LOGI(TAG, "========================================");
    
    // 主循环（低优先级任务）
    // 用于周期性打印系统状态监控信息
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // 每10秒打印一次
        // 获取并打印当前应用状态机所处状态
        ESP_LOGI(TAG, "System state: %s", app_state_machine_get_state_name());
        
        // 可选：打印内存剩余情况
        // ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    }
}
