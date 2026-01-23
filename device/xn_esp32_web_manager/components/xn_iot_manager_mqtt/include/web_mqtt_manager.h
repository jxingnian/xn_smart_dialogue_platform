/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_iot_manager_mqtt\include\web_mqtt_manager.h
 * @Description: Web MQTT 管理器 - 头文件
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef WEB_MQTT_MANAGER_H
#define WEB_MQTT_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Web MQTT 管理器内部状态机单步运行周期（单位：ms）
 *
 * 管理任务会按该间隔周期性驱动内部状态机：
 * - 间隔过小：状态刷新更及时，但占用更多 CPU；
 * - 间隔过大：重连 / 心跳响应会变慢。
 */
#define WEB_MQTT_MANAGER_STEP_INTERVAL_MS 5000 ///< 默认状态机运行间隔（ms）

/**
 * @brief Web MQTT 管理器状态枚举
 *
 * 仅抽象出上层关心的几个重要阶段，不暴露底层 MQTT 客户端细节。
 */
typedef enum {
    WEB_MQTT_STATE_DISCONNECTED = 0, ///< 已断开或尚未开始连接
    WEB_MQTT_STATE_CONNECTING,       ///< 正在与服务器建立连接
    WEB_MQTT_STATE_CONNECTED,        ///< 已连接但尚未完成必要订阅
    WEB_MQTT_STATE_READY,            ///< 已连接且完成必要订阅，可正常收发（本实现中目前等同于 CONNECTED）
    WEB_MQTT_STATE_ERROR,            ///< 出现错误，等待自动重连或人工干预
} web_mqtt_state_t;

/**
 * @brief Web MQTT 管理器事件回调函数原型
 *
 * 管理模块在状态变化时调用，用于通知上层做相应处理：
 * - 可用于更新 UI / 打日志 / 上报云端等。
 *
 * @param state 当前 MQTT 管理器状态（见 @ref web_mqtt_state_t）
 */
typedef void (*web_mqtt_event_cb_t)(web_mqtt_state_t state);

/**
 * @brief Web MQTT 默认认证信息
 *
 * 若上层未在配置结构体中显式设置 username/password，将使用这里的默认值。
 */
#ifndef WEB_MQTT_DEFAULT_USERNAME
#define WEB_MQTT_DEFAULT_USERNAME "xn_mqtt"     ///< 默认 MQTT 用户名
#endif

#ifndef WEB_MQTT_DEFAULT_PASSWORD
#define WEB_MQTT_DEFAULT_PASSWORD "xn_mqtt_pass" ///< 默认 MQTT 密码
#endif

#ifndef WEB_MQTT_UPLINK_BASE_TOPIC
#define WEB_MQTT_UPLINK_BASE_TOPIC "xn/esp"
#endif

/**
 * @brief Web MQTT 管理器配置结构体
 *
 * 该结构体仅在初始化时读取一次，之后由管理器内部持有副本。
 */
typedef struct {
    const char          *broker_uri;            ///< MQTT 服务器 URI，如 "mqtt://192.168.1.10:1883"
    const char          *client_id;             ///< MQTT 客户端 ID，NULL 表示使用芯片唯一 ID (ESP32_MAC)
    const char          *username;              ///< MQTT 用户名，可为 NULL 表示匿名
    const char          *password;              ///< MQTT 密码，可为 NULL 表示无密码
    const char          *base_topic;            ///< Web 管理相关的基础 Topic 前缀，如 "xn/web"
    int                  keepalive_sec;         ///< MQTT keepalive 保活时间（秒），<=0 使用组件默认值
    int                  reconnect_interval_ms; ///< 连接失败后自动重连间隔(ms)；<0 表示关闭自动重连
    int                  step_interval_ms;      ///< 状态机运行周期（ms），<=0 使用 WEB_MQTT_MANAGER_STEP_INTERVAL_MS
    web_mqtt_event_cb_t  event_cb;              ///< 状态及重要事件回调，可为 NULL 表示不关心
} web_mqtt_manager_config_t;

/**
 * @brief Web MQTT 管理器默认配置宏
 */
#define WEB_MQTT_MANAGER_DEFAULT_CONFIG()                              \
    (web_mqtt_manager_config_t) {                                      \
        .broker_uri            = NULL,                                 \
        .client_id             = NULL,                                 \
        .username              = WEB_MQTT_DEFAULT_USERNAME,            \
        .password              = WEB_MQTT_DEFAULT_PASSWORD,            \
        .base_topic            = NULL,                                 \
        .keepalive_sec         = 60,                                   \
        .reconnect_interval_ms = 5000,                                 \
        .step_interval_ms      = WEB_MQTT_MANAGER_STEP_INTERVAL_MS,    \
        .event_cb              = NULL,                                 \
    }

/**
 * @brief 初始化、配置并启动 Web MQTT 管理器
 *
 * 功能概览：
 * - 初始化内部 MQTT 客户端及相关资源；
 * - 创建管理任务并启动内部状态机；
 * - 根据配置自动尝试与服务器建立连接。
 *
 * @note 调用前应确保 WiFi / 以太网 已经就绪并具备网络连接。
 *
 * @param config 若为 NULL，则使用 @ref WEB_MQTT_MANAGER_DEFAULT_CONFIG
 *
 * @return
 *      - ESP_OK        : 初始化并启动成功
 *      - ESP_ERR_INVALID_ARG: 参数无效（如 broker_uri 为空）
 *      - ESP_ERR_NO_MEM: 内存不足
 *      - 其它 esp_err_t: 由底层组件返回的错误码
 */
esp_err_t web_mqtt_manager_init(const web_mqtt_manager_config_t *config);

/**
 * @brief 获取当前 MQTT 客户端 ID
 * 
 * 如果配置未指定，将返回自动生成的基于MAC的ID。
 * 
 * @return const char* 客户端ID字符串
 */
const char *web_mqtt_manager_get_client_id(void);

/**
 * @brief 获取当前配置的基础下行 Topic 前缀
 * 
 * @return const char* 基础Topic前缀（如 "xn/web"）
 */
const char *web_mqtt_manager_get_base_topic(void);

#ifdef __cplusplus
}
#endif

#endif /* WEB_MQTT_MANAGER_H */
