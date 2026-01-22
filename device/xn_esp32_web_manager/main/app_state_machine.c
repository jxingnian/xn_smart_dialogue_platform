/**
 * @file app_state_machine.c
 * @brief 应用顶层状态机实现
 * 
 * 本文件实现了系统的核心状态流转逻辑。
 * 状态机主要负责协调 WiFi、MQTT 和 BluFi 配网等模块的工作顺序。
 * 它通过订阅系统事件总线上的事件（如 WIFI_CONNECTED, MQTT_CONNECTED 等）
 * 来驱动系统状态在 INIT, WIFI_CONNECTING, READY 等状态间流转。
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

// 状态机TAG，用于日志输出
static const char *TAG = "app_fsm";

/*===========================================================================
 *                          状态机实例
 *===========================================================================*/

static xn_fsm_t s_fsm;              // 状态机核心结构体实例
static bool s_initialized = false;   // 模块初始化标志位，防止重复初始化

/*===========================================================================
 *                          状态回调
 *===========================================================================*/

/**
 * @brief 进入 INIT 状态动作
 * 
 * 动作：
 * 1. 打印状态日志
 * 2. 发布 XN_EVT_SYSTEM_INIT_DONE 事件
 * 
 * 逻辑：
 * 系统启动后首先进入此状态，完成必要的底层初始化后，
 * 立即触发事件自动跳转到 WIFI_CONNECTING 状态。
 */
static void on_enter_init(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> INIT state"); // 打印日志：进入INIT状态
    // 发布"系统初始化完成"事件，驱动状态机流转
    xn_event_post(XN_EVT_SYSTEM_INIT_DONE, XN_EVT_SRC_SYSTEM); // 发送事件告知系统初始化已完成
}

/**
 * @brief 进入 WIFI_CONNECTING 状态动作
 * 
 * 动作：
 * 1. 打印状态日志
 * 2. 调用 wifi_manager_start() 启动 WiFi 连接流程
 * 
 * 触发条件：
 * - 系统初始化完成
 * - WiFi 异常断开重连
 * - 配网结束
 */
static void on_enter_wifi_connecting(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> WIFI_CONNECTING state"); // 打印日志：进入WiFi连接状态
    // 调用 WiFi Manager 开始连接
    wifi_manager_start(); // 调用WiFi管理器接口，启动WiFi连接任务
}

/**
 * @brief 进入 WIFI_CONNECTED 状态动作
 * 
 * 说明：
 * 此时 WiFi 链路层已建立连接（Sta Connected），但尚未获取 IP 地址。
 * 系统正在后台进行 DHCP 握手。
 * 
 * 下一步：
 * 等待 WiFi Manager 发布 XN_EVT_WIFI_GOT_IP 事件。
 */
static void on_enter_wifi_connected(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> WIFI_CONNECTED state (waiting for IP)"); // 打印日志：WiFi链路已通，等待IP分配
}

/**
 * @brief 进入 MQTT_CONNECTING 状态动作
 * 
 * 前置条件：
 * - WiFi 已连接且已获取 IP 地址
 * 
 * 逻辑：
 * 此时网络通道已就绪。MQTT Manager 应监听到 GOT_IP 事件并开始连接 Broker。
 * 本状态作为逻辑检查点，等待 XN_EVT_MQTT_CONNECTED 事件。
 */
static void on_enter_mqtt_connecting(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> MQTT_CONNECTING state"); // 打印日志：进入MQTT连接状态
    // MQTT Manager 负责监听 GOT_IP 事件，此时应该已经开始自动连接
    // 此状态作为一个逻辑检查点
}

/**
 * @brief 进入 READY 状态动作
 * 
 * 说明：
 * 系统已完全就绪：
 * 1. WiFi 已连接且有 IP
 * 2. MQTT 已连接到服务器
 * 
 * 动作：
 * 发布 XN_EVT_SYSTEM_READY 事件，通知业务层可以开始正常工作。
 */
static void on_enter_ready(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> READY state - System is fully operational"); // 打印日志：系统完全就绪
    // 发布系统就绪事件，通知业务层或其他服务
    xn_event_post(XN_EVT_SYSTEM_READY, XN_EVT_SRC_SYSTEM); // 发送系统就绪事件，通知应用层
}

/**
 * @brief 进入 BLUFI_CONFIG 状态动作
 * 
 * 触发条件：
 * - 用户按键强制配网 (XN_CMD_BLUFI_START)
 * - 或者其他未能自动连接 WiFi 的场景（取决于策略）
 * 
 * 动作：
 * 启动 BluFi 蓝牙配网服务，等待用户通过 App 发送配置信息。
 */
static void on_enter_blufi_config(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "==> BLUFI_CONFIG state"); // 打印日志：进入BluFi配网状态
    // 启动蓝牙配网
    blufi_manager_start(); // 调用BluFi管理器接口，启动蓝牙配网服务
}

/**
 * @brief 退出 BLUFI_CONFIG 状态动作
 * 
 * 动作：
 * 停止 BluFi 服务，释放蓝牙栈资源（如果需要节省内存），
 * 准备切换回 WiFi 连接模式。
 */
static void on_exit_blufi_config(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGI(TAG, "<== Exiting BLUFI_CONFIG state"); // 打印日志：退出BluFi配网状态
    // 停止蓝牙配网，释放蓝牙资源（如果策略允许）
    blufi_manager_stop(); // 调用BluFi管理器接口，停止蓝牙配网服务
}

/**
 * @brief 进入 ERROR 状态动作
 * 
 * 说明：
 * 系统遇到不可恢复的严重错误时进入此状态。
 * 可用于执行系统复位、错误上报或安全停机等操作。
 */
static void on_enter_error(xn_fsm_t *fsm, void *user_data)
{
    ESP_LOGE(TAG, "==> ERROR state"); // 打印错误日志：进入ERROR状态
    xn_event_post(XN_EVT_SYSTEM_ERROR, XN_EVT_SRC_SYSTEM); // 发送系统错误事件
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
    // ============================================================
    // 初始化与启动
    // ============================================================
    // 从 INIT -> WIFI_CONNECTING
    // 描述：初始化完成后，自动尝试连接 WiFi
    {APP_STATE_INIT,            XN_EVT_SYSTEM_INIT_DONE,    APP_STATE_WIFI_CONNECTING,  NULL, NULL},
    
    // ============================================================
    // WiFi 连接流程
    // ============================================================
    // 从 WIFI_CONNECTING
    // 成功连接物理层 -> 等待分配 IP
    {APP_STATE_WIFI_CONNECTING, XN_EVT_WIFI_CONNECTED,      APP_STATE_WIFI_CONNECTED,   NULL, NULL},
    // 在连接过程中，用户强制启动配网 -> 进入配网模式
    {APP_STATE_WIFI_CONNECTING, XN_CMD_BLUFI_START,         APP_STATE_BLUFI_CONFIG,     NULL, NULL},
    
    // ============================================================
    // IP 获取与网络层
    // ============================================================
    // 从 WIFI_CONNECTED
    // 成功获取 IP -> 开始连接 MQTT
    {APP_STATE_WIFI_CONNECTED,  XN_EVT_WIFI_GOT_IP,         APP_STATE_MQTT_CONNECTING,  NULL, NULL},
    // IP 获取还没完成就断开了 -> 重新连接 WiFi
    {APP_STATE_WIFI_CONNECTED,  XN_EVT_WIFI_DISCONNECTED,   APP_STATE_WIFI_CONNECTING,  NULL, NULL},
    
    // ============================================================
    // MQTT 连接流程
    // ============================================================
    // 从 MQTT_CONNECTING
    // MQTT 协议握手成功 -> 系统就绪 (READY)
    {APP_STATE_MQTT_CONNECTING, XN_EVT_MQTT_CONNECTED,      APP_STATE_READY,            NULL, NULL},
    // 如果在连接 MQTT 过程中 WiFi 掉了 -> 回退到 WiFi 连接
    {APP_STATE_MQTT_CONNECTING, XN_EVT_WIFI_DISCONNECTED,   APP_STATE_WIFI_CONNECTING,  NULL, NULL},
    
    // ============================================================
    // 系统就绪状态 (稳定态)
    // ============================================================
    // 从 READY
    // 任意时刻 WiFi 掉线 -> 回退到 WiFi 连接
    {APP_STATE_READY,           XN_EVT_WIFI_DISCONNECTED,   APP_STATE_WIFI_CONNECTING,  NULL, NULL},
    // 仅 MQTT 掉线 (WiFi还在) -> 重新连接 MQTT
    {APP_STATE_READY,           XN_EVT_MQTT_DISCONNECTED,   APP_STATE_MQTT_CONNECTING,  NULL, NULL},
    // 在就绪状态下，用户强制配网 -> 进入配网模式
    {APP_STATE_READY,           XN_CMD_BLUFI_START,         APP_STATE_BLUFI_CONFIG,     NULL, NULL},
    
    // ============================================================
    // 配网模式
    // ============================================================
    // 从 BLUFI_CONFIG
    // 配网完成（收到配置信息） -> 尝试连接 WiFi
    {APP_STATE_BLUFI_CONFIG,    XN_EVT_BLUFI_CONFIG_DONE,   APP_STATE_WIFI_CONNECTING,  NULL, NULL},
    // 用户取消或停止配网 -> 尝试连接 WiFi (尝试使用旧配置或重试)
    {APP_STATE_BLUFI_CONFIG,    XN_CMD_BLUFI_STOP,          APP_STATE_WIFI_CONNECTING,  NULL, NULL},
    
    // ============================================================
    // 异常处理
    // ============================================================
    // 任意状态 -> ERROR
    // 收到系统级致命错误 -> 进入 ERROR 状态
    {XN_STATE_ANY,              XN_EVT_SYSTEM_ERROR,        APP_STATE_ERROR,            NULL, NULL},
};

/*===========================================================================
 *                          事件处理
 *===========================================================================*/

/**
 * @brief 系统事件总线回调
 * 
 * 作用：
 * 作为 Event Bus 和 FSM 之间的桥梁。
 * 收到任何系统事件时，将其 ID 转发给 FSM 的 input 处理函数。
 * FSM 内部会根据当前状态和转换表查找是否有匹配的跳转路径。
 */
static void event_handler(const xn_event_t *event, void *user_data)
{
    // 调用 FSM 处理函数，将事件 ID 传入状态机
    esp_err_t ret = xn_fsm_process_event(&s_fsm, event->id); 
    if (ret == ESP_OK) { // 如果返回值是 ESP_OK，说明发生了状态转换
        ESP_LOGD(TAG, "State transition triggered by event 0x%04x", event->id); // 打印调试日志
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t app_state_machine_init(void)
{
    if (s_initialized) { // 检查是否已经初始化
        return ESP_ERR_INVALID_STATE; // 如果已初始化，返回无效状态错误
    }
    
    xn_fsm_config_t config = {
        .name = "AppFSM",           // 状态机名称
        .initial_state = APP_STATE_INIT, // 初始状态为 INIT
        .states = s_states,         // 状态定义表
        .state_count = sizeof(s_states) / sizeof(s_states[0]), // 状态数量
        .transitions = s_transitions, // 转换定义表
        .transition_count = sizeof(s_transitions) / sizeof(s_transitions[0]), // 转换数量
        .user_data = NULL,          // 用户数据（此处未用到）
    };
    
    // 初始化 FSM 实例
    esp_err_t ret = xn_fsm_init(&s_fsm, &config);
    if (ret != ESP_OK) { // 检查初始化是否成功
        ESP_LOGE(TAG, "Failed to init FSM: %s", esp_err_to_name(ret)); // 打印错误日志
        return ret; // 返回错误码
    }
    
    // 订阅所有相关事件（这里简单地订阅所有事件，实际可优化只订阅关心的ID）
    // 注册 event_handler 回调处理所有事件
    xn_event_subscribe(XN_EVT_ANY, event_handler, NULL);
    
    s_initialized = true; // 标记初始化完成
    ESP_LOGI(TAG, "App state machine initialized"); // 打印初始化成功日志
    
    return ESP_OK; // 返回成功
}

esp_err_t app_state_machine_start(void)
{
    if (!s_initialized) { // 检查是否已初始化
        return ESP_ERR_INVALID_STATE; // 未初始化则报错
    }
    
    return xn_fsm_start(&s_fsm); // 启动状态机，使其进入初始状态
}

esp_err_t app_state_machine_stop(void)
{
    if (!s_initialized) { // 检查是否已初始化
        return ESP_ERR_INVALID_STATE; // 未初始化则报错
    }
    
    xn_event_unsubscribe_all(event_handler); // 取消订阅所有事件
    
    return xn_fsm_stop(&s_fsm); // 停止状态机
}

app_state_t app_state_machine_get_state(void)
{
    // 获取当前 FSM 的状态并强制转换为 app_state_t 枚举
    return (app_state_t)xn_fsm_get_state(&s_fsm);
}

const char *app_state_machine_get_state_name(void)
{
    // 获取当前状态的名称字符串（用于调试/显示）
    return xn_fsm_get_state_name(&s_fsm);
}

esp_err_t app_state_machine_enter_blufi(void)
{
    // 发送 BLUFI_START 命令事件，由 FSM 处理跳转
    xn_event_post(XN_CMD_BLUFI_START, XN_EVT_SRC_SYSTEM);
    return ESP_OK; // 命令发送成功
}
