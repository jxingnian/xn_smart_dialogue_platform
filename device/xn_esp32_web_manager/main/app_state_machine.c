/**
 * @file app_state_machine.c
 * @brief 应用顶层状态机实现
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "xn_fsm.h"
#include "xn_event_bus.h"
#include "app_state_machine.h"
#include "managers/wifi_manager.h"
#include "managers/mqtt_manager.h"
#include "managers/blufi_manager.h"

static const char *TAG = "app_fsm";

/*===========================================================================
 *                          状态机实例
 *===========================================================================*/

static xn_fsm_t s_fsm;
static bool s_initialized = false;

/*===========================================================================
 *                          状态回调
 *===========================================================================*/

/**
 * @brief 进入 INIT 状态动作
 * 发送 INIT_DONE 事件，触发自动跳转到 WIFI_CONNECTING
 */
static void on_enter_init(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> INIT state");
    // 发布"系统初始化完成"事件，驱动状态机流转
    xn_event_post(XN_EVT_SYSTEM_INIT_DONE, XN_EVT_SRC_SYSTEM);
}

/**
 * @brief 进入 WIFI_CONNECTING 状态动作
 * 启动 WiFi 连接尝试
 */
static void on_enter_wifi_connecting(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> WIFI_CONNECTING state");
    // 调用 WiFi Manager 开始连接
    wifi_manager_start();
}

/**
 * @brief 进入 WIFI_CONNECTED 状态动作
 * 已连接物理层，等待 IP 分配(Smart Config / DHCP)
 */
static void on_enter_wifi_connected(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> WIFI_CONNECTED state (waiting for IP)");
}

/**
 * @brief 进入 MQTT_CONNECTING 状态动作
 */
static void on_enter_mqtt_connecting(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> MQTT_CONNECTING state");
    // MQTT Manager 负责监听 GOT_IP 事件，此时应该已经开始自动连接
    // 此状态作为一个逻辑检查点
}

/**
 * @brief 进入 READY 状态动作
 * 系统已完全就绪（WiFi IP ok + MQTT Connected）
 */
static void on_enter_ready(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> READY state - System is fully operational");
    // 发布系统就绪事件，通知业务层或其他服务
    xn_event_post(XN_EVT_SYSTEM_READY, XN_EVT_SRC_SYSTEM);
}

/**
 * @brief 进入 BLUFI_CONFIG 状态动作
 * 启动 BluFi 配网服务
 */
static void on_enter_blufi_config(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> BLUFI_CONFIG state");
    // 启动蓝牙配网
    blufi_manager_start();
}

/**
 * @brief 退出 BLUFI_CONFIG 状态动作
 * 停止 BluFi 配网服务
 */
static void on_exit_blufi_config(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "<== Exiting BLUFI_CONFIG state");
    // 停止蓝牙配网，释放蓝牙资源（如果策略允许）
    blufi_manager_stop();
}

/**
 * @brief 进入 ERROR 状态动作
 */
static void on_enter_error(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGE(TAG, "==> ERROR state");
    xn_event_post(XN_EVT_SYSTEM_ERROR, XN_EVT_SRC_SYSTEM);
}

/*===========================================================================
 *                          状态定义表
 *===========================================================================*/

static const xn_fsm_state_t s_states[] = {
    {APP_STATE_INIT,            "INIT",             on_enter_init,              NULL, NULL},
    {APP_STATE_WIFI_CONNECTING, "WIFI_CONNECTING",  on_enter_wifi_connecting,   NULL, NULL},
    {APP_STATE_WIFI_CONNECTED,  "WIFI_CONNECTED",   on_enter_wifi_connected,    NULL, NULL},
    {APP_STATE_MQTT_CONNECTING, "MQTT_CONNECTING",  on_enter_mqtt_connecting,   NULL, NULL},
    {APP_STATE_READY,           "READY",            on_enter_ready,             NULL, NULL},
    {APP_STATE_BLUFI_CONFIG,    "BLUFI_CONFIG",     on_enter_blufi_config,      on_exit_blufi_config, NULL},
    {APP_STATE_ERROR,           "ERROR",            on_enter_error,             NULL, NULL},
};

/*===========================================================================
 *                          转换定义表
 *===========================================================================*/

static const xn_fsm_transition_t s_transitions[] = {
    // 从 INIT -> WIFI_CONNECTING
    {APP_STATE_INIT,            XN_EVT_SYSTEM_INIT_DONE,    APP_STATE_WIFI_CONNECTING,  NULL, NULL},
    
    // 从 WIFI_CONNECTING
    {APP_STATE_WIFI_CONNECTING, XN_EVT_WIFI_CONNECTED,      APP_STATE_WIFI_CONNECTED,   NULL, NULL}, // 连接成功
    {APP_STATE_WIFI_CONNECTING, XN_CMD_BLUFI_START,         APP_STATE_BLUFI_CONFIG,     NULL, NULL}, // 用户强制配网
    
    // 从 WIFI_CONNECTED
    {APP_STATE_WIFI_CONNECTED,  XN_EVT_WIFI_GOT_IP,         APP_STATE_MQTT_CONNECTING,  NULL, NULL}, // 获取IP -> 连MQTT
    {APP_STATE_WIFI_CONNECTED,  XN_EVT_WIFI_DISCONNECTED,   APP_STATE_WIFI_CONNECTING,  NULL, NULL}, // 掉线 -> 重连
    
    // 从 MQTT_CONNECTING
    {APP_STATE_MQTT_CONNECTING, XN_EVT_MQTT_CONNECTED,      APP_STATE_READY,            NULL, NULL}, // MQTT连上 -> 就绪
    {APP_STATE_MQTT_CONNECTING, XN_EVT_WIFI_DISCONNECTED,   APP_STATE_WIFI_CONNECTING,  NULL, NULL}, // WiFi掉线 -> 重连WiFi
    
    // 从 READY
    {APP_STATE_READY,           XN_EVT_WIFI_DISCONNECTED,   APP_STATE_WIFI_CONNECTING,  NULL, NULL}, // WiFi掉线
    {APP_STATE_READY,           XN_EVT_MQTT_DISCONNECTED,   APP_STATE_MQTT_CONNECTING,  NULL, NULL}, // MQTT掉线 -> 重连MQTT
    {APP_STATE_READY,           XN_CMD_BLUFI_START,         APP_STATE_BLUFI_CONFIG,     NULL, NULL}, // 强制配网
    
    // 从 BLUFI_CONFIG
    {APP_STATE_BLUFI_CONFIG,    XN_EVT_BLUFI_CONFIG_DONE,   APP_STATE_WIFI_CONNECTING,  NULL, NULL}, // 配网完成 -> 连WiFi
    {APP_STATE_BLUFI_CONFIG,    XN_CMD_BLUFI_STOP,          APP_STATE_WIFI_CONNECTING,  NULL, NULL}, // 取消配网
    
    // 任意状态 -> ERROR
    {XN_STATE_ANY,              XN_EVT_SYSTEM_ERROR,        APP_STATE_ERROR,            NULL, NULL},
};

/*===========================================================================
 *                          事件处理
 *===========================================================================*/

/**
 * @brief 系统事件总线回调
 * 
 * 将接收到的事件转发给 FSM 处理，由 FSM 决定是否进行状态转换。
 */
static void event_handler(const xn_event_t *event, void *user_data)
{
    esp_err_t ret = xn_fsm_process_event(&s_fsm, event->id);
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "State transition triggered by event 0x%04x", event->id);
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t app_state_machine_init(void)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xn_fsm_config_t config = {
        .name = "AppFSM",
        .initial_state = APP_STATE_INIT,
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .transitions = s_transitions,
        .transition_count = sizeof(s_transitions) / sizeof(s_transitions[0]),
        .user_data = NULL,
    };
    
    esp_err_t ret = xn_fsm_init(&s_fsm, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init FSM: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 订阅所有相关事件（这里简单地订阅所有事件，实际可优化只订阅关心的ID）
    xn_event_subscribe(XN_EVT_ANY, event_handler, NULL);
    
    s_initialized = true;
    ESP_LOGI(TAG, "App state machine initialized");
    
    return ESP_OK;
}

esp_err_t app_state_machine_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return xn_fsm_start(&s_fsm);
}

esp_err_t app_state_machine_stop(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xn_event_unsubscribe_all(event_handler);
    
    return xn_fsm_stop(&s_fsm);
}

app_state_t app_state_machine_get_state(void)
{
    return (app_state_t)xn_fsm_get_state(&s_fsm);
}

const char *app_state_machine_get_state_name(void)
{
    return xn_fsm_get_state_name(&s_fsm);
}

esp_err_t app_state_machine_enter_blufi(void)
{
    xn_event_post(XN_CMD_BLUFI_START, XN_EVT_SRC_SYSTEM);
    return ESP_OK;
}
