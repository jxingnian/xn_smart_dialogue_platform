/**
 * @file blufi_manager.c
 * @brief BluFi配网应用管理器实现（精简版）
 * 
 * 注意：完整的BluFi功能需要在 menuconfig 中启用蓝牙：
 * Component config -> Bluetooth -> Bluetooth (勾选)
 * Component config -> Bluetooth -> Bluedroid Options -> BluFi (勾选)
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "xn_event_bus.h"
#include "blufi_manager.h"

static const char *TAG = "blufi_manager";

/*===========================================================================
 *                          内部变量
 *===========================================================================*/

static bool s_initialized = false;          ///< 初始化标志
static bool s_running = false;              ///< 运行状态标志

/*===========================================================================
 *                          命令事件处理
 *===========================================================================*/

/**
 * @brief XN事件总线命令回调
 */
static void cmd_event_handler(const xn_event_t *event, void *user_data)
{
    switch (event->id) {
        case XN_CMD_BLUFI_START:
            ESP_LOGI(TAG, "Received BLUFI_START command");
            blufi_manager_start();
            break;
            
        case XN_CMD_BLUFI_STOP:
            ESP_LOGI(TAG, "Received BLUFI_STOP command");
            blufi_manager_stop();
            break;
            
        default:
            break;
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t blufi_manager_init(void)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 订阅开始和停止命令
    xn_event_subscribe(XN_CMD_BLUFI_START, cmd_event_handler, NULL);
    xn_event_subscribe(XN_CMD_BLUFI_STOP, cmd_event_handler, NULL);
    
    s_initialized = true;
    ESP_LOGI(TAG, "BluFi manager initialized (stub mode - enable BT in menuconfig for full support)");
    
    return ESP_OK;
}

esp_err_t blufi_manager_deinit(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 确保已停止
    if (s_running) {
        blufi_manager_stop();
    }
    
    // 取消订阅
    xn_event_unsubscribe_all(cmd_event_handler);
    
    s_initialized = false;
    
    ESP_LOGI(TAG, "BluFi manager deinitialized");
    
    return ESP_OK;
}

esp_err_t blufi_manager_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_running) {
        ESP_LOGW(TAG, "BluFi already running");
        return ESP_OK; // 已经运行则视为成功
    }
    
    // TODO: 在 menuconfig 启用蓝牙后，添加以下代码：
    // 1. 初始化BT Controller
    // esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    // esp_bt_controller_init(&bt_cfg);
    // esp_bt_controller_enable(ESP_BT_MODE_BLE);
    
    // 2. 初始化Bluedroid Host
    // esp_bluedroid_init();
    // esp_bluedroid_enable();
    
    // 3. 注册回调并启动Profile
    // esp_blufi_register_callbacks(&blufi_callbacks);
    // esp_blufi_profile_init();
    
    ESP_LOGW(TAG, "BluFi start - Bluetooth not enabled in menuconfig");
    ESP_LOGW(TAG, "Please run: idf.py menuconfig -> Component config -> Bluetooth");
    
    s_running = true;
    
    // 模拟配网初始化完成
    xn_event_post(XN_EVT_BLUFI_INIT_DONE, XN_EVT_SRC_BLUFI);
    
    return ESP_OK;
}

esp_err_t blufi_manager_stop(void)
{
    if (!s_initialized || !s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: 释放蓝牙资源
    // esp_blufi_profile_deinit();
    // esp_bluedroid_disable();
    // esp_bluedroid_deinit();
    // esp_bt_controller_disable();
    // esp_bt_controller_deinit();
    
    s_running = false;
    ESP_LOGI(TAG, "BluFi stopped");
    
    return ESP_OK;
}

bool blufi_manager_is_running(void)
{
    return s_running;
}
