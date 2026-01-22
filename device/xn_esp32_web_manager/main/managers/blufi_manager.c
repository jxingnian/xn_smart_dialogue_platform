/**
 * @file blufi_manager.c
 * @brief BluFi配网应用管理器实现
 * 
 * 调用底层 xn_blufi 组件，通过事件总线发布配网状态
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_blufi_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "xn_event_bus.h"
#include "blufi_manager.h"

static const char *TAG = "blufi_manager";

/*===========================================================================
 *                          内部变量
 *===========================================================================*/

static bool s_initialized = false;
static bool s_running = false;

/*===========================================================================
 *                          BluFi回调（简化示例）
 *===========================================================================*/

static void blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    switch (event) {
        case ESP_BLUFI_EVENT_INIT_FINISH:
            ESP_LOGI(TAG, "BluFi init finish");
            xn_event_post(XN_EVT_BLUFI_INIT_DONE, XN_EVT_SRC_BLUFI);
            break;
            
        case ESP_BLUFI_EVENT_DEINIT_FINISH:
            ESP_LOGI(TAG, "BluFi deinit finish");
            xn_event_post(XN_EVT_BLUFI_DEINIT_DONE, XN_EVT_SRC_BLUFI);
            break;
            
        case ESP_BLUFI_EVENT_BLE_CONNECT:
            ESP_LOGI(TAG, "BluFi BLE connected");
            xn_event_post(XN_EVT_BLUFI_CONNECTED, XN_EVT_SRC_BLUFI);
            break;
            
        case ESP_BLUFI_EVENT_BLE_DISCONNECT:
            ESP_LOGI(TAG, "BluFi BLE disconnected");
            xn_event_post(XN_EVT_BLUFI_DISCONNECTED, XN_EVT_SRC_BLUFI);
            break;
            
        case ESP_BLUFI_EVENT_RECV_STA_SSID:
            ESP_LOGI(TAG, "Received SSID: %.*s", param->sta_ssid.ssid_len, param->sta_ssid.ssid);
            break;
            
        case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
            ESP_LOGI(TAG, "Received password");
            break;
            
        case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
            // 可返回当前WiFi状态
            break;
            
        case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
            esp_blufi_close(NULL);
            break;
            
        default:
            ESP_LOGD(TAG, "BluFi event: %d", event);
            break;
    }
}

static esp_blufi_callbacks_t blufi_callbacks = {
    .event_cb = blufi_event_callback,
    // 加密相关回调可以根据需要添加
    .negotiate_data_handler = NULL,
    .encrypt_func = NULL,
    .decrypt_func = NULL,
    .checksum_func = NULL,
};

/*===========================================================================
 *                          命令事件处理
 *===========================================================================*/

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
    
    // 订阅命令事件
    xn_event_subscribe(XN_CMD_BLUFI_START, cmd_event_handler, NULL);
    xn_event_subscribe(XN_CMD_BLUFI_STOP, cmd_event_handler, NULL);
    
    s_initialized = true;
    ESP_LOGI(TAG, "BluFi manager initialized");
    
    return ESP_OK;
}

esp_err_t blufi_manager_deinit(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_running) {
        blufi_manager_stop();
    }
    
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
        return ESP_OK;
    }
    
    // 释放经典蓝牙内存
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_blufi_register_callbacks(&blufi_callbacks);
    if (ret) {
        ESP_LOGE(TAG, "BluFi register callbacks failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_blufi_profile_init();
    if (ret) {
        ESP_LOGE(TAG, "BluFi profile init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_running = true;
    ESP_LOGI(TAG, "BluFi started");
    
    return ESP_OK;
}

esp_err_t blufi_manager_stop(void)
{
    if (!s_initialized || !s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_blufi_profile_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    
    s_running = false;
    ESP_LOGI(TAG, "BluFi stopped");
    
    return ESP_OK;
}

bool blufi_manager_is_running(void)
{
    return s_running;
}
