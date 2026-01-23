/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:51:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\mqtt_manager.h
 * @Description: MQTT应用管理器头文件 - 业务逻辑管理（策略层）
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#ifndef MQTT_MANAGER_H                              // 防止头文件重复包含
#define MQTT_MANAGER_H                              // 定义头文件宏

#include "esp_err.h"                                // 包含ESP错误码定义
#include <stdbool.h>                                // 包含布尔类型定义
#include <stdint.h>                                 // 包含标准整型定义

#ifdef __cplusplus                                  // 如果是C++编译器
extern "C" {                                        // 使用C链接约定
#endif                                              // 结束C++编译器判断

/* ========================================================================== */
/*                              状态与配置定义                                  */
/* ========================================================================== */

/**
 * @brief MQTT管理器状态机运行周期（单位：ms）
 *
 * 管理任务按该间隔周期性驱动内部状态机：
 * - 间隔过小：状态刷新更及时，但占用更多CPU
 * - 间隔过大：重连/心跳响应会变慢
 */
#define MQTT_MANAGER_STEP_INTERVAL_MS   5000        // 默认状态机运行间隔（ms）

/**
 * @brief MQTT管理器状态枚举
 *
 * 抽象出业务层关心的几个重要阶段
 */
typedef enum {
    MQTT_MANAGER_STATE_IDLE = 0,                    // 空闲状态，尚未启动
    MQTT_MANAGER_STATE_DISCONNECTED,                // 已断开或尚未开始连接
    MQTT_MANAGER_STATE_CONNECTING,                  // 正在与服务器建立连接
    MQTT_MANAGER_STATE_CONNECTED,                   // 已连接到MQTT服务器
    MQTT_MANAGER_STATE_ERROR,                       // 出现错误，等待自动重连或人工干预
} mqtt_manager_state_t;                             // MQTT管理器状态类型定义

/**
 * @brief MQTT管理器状态回调函数原型
 *
 * 管理模块在状态变化时调用，用于通知上层做相应处理
 *
 * @param state 当前MQTT管理器状态（见 @ref mqtt_manager_state_t）
 */
typedef void (*mqtt_manager_state_cb_t)(mqtt_manager_state_t state); // 状态回调类型定义

/**
 * @brief MQTT管理器配置结构体
 *
 * 该结构体在初始化时读取一次，之后由管理器内部持有副本
 */
typedef struct {
    const char              *broker_uri;            // MQTT服务器URI，如 "mqtt://192.168.1.10:1883"
    const char              *client_id;             // MQTT客户端ID，NULL表示使用芯片唯一ID
    const char              *username;              // MQTT用户名，可为NULL表示匿名
    const char              *password;              // MQTT密码，可为NULL表示无密码
    const char              *base_topic;            // 项目基础Topic前缀，如 "xn/device"
    int                      keepalive_sec;         // MQTT keepalive保活时间（秒），<=0使用组件默认值
    int                      reconnect_interval_ms; // 连接失败后自动重连间隔(ms)；<0表示关闭自动重连
    int                      step_interval_ms;      // 状态机运行周期（ms），<=0使用默认值
    mqtt_manager_state_cb_t  state_cb;              // 状态变更回调，可为NULL表示不关心
    /**
     * @brief 消息接收回调
     * 
     * @param topic       Topic字符串
     * @param topic_len   Topic长度
     * @param payload     Payload数据
     * @param payload_len Payload长度
     */
    void (*message_cb)(const char *topic, int topic_len, const uint8_t *payload, int payload_len);
} mqtt_manager_config_t;                            // MQTT管理器配置类型定义

/**
 * @brief MQTT管理器默认配置宏
 */
#define MQTT_MANAGER_DEFAULT_CONFIG()                               \
    (mqtt_manager_config_t) {                                       \
        .broker_uri            = NULL,                              \
        .client_id             = NULL,                              \
        .username              = "xn_mqtt",                         \
        .password              = "xn_mqtt_pass",                    \
        .base_topic            = NULL,                              \
        .keepalive_sec         = 60,                                \
        .reconnect_interval_ms = 5000,                              \
        .step_interval_ms      = MQTT_MANAGER_STEP_INTERVAL_MS,     \
        .state_cb              = NULL,                              \
        .message_cb            = NULL,                              \
    }

/* ========================================================================== */
/*                                公共API                                      */
/* ========================================================================== */

/**
 * @brief 初始化MQTT管理器
 * 
 * 功能概览：
 * - 初始化内部MQTT客户端及相关资源
 * - 创建管理任务并启动内部状态机
 * - 根据配置自动尝试与服务器建立连接
 *
 * @note 调用前应确保WiFi/以太网已经就绪并具备网络连接
 *
 * @param config 若为NULL，则使用 @ref MQTT_MANAGER_DEFAULT_CONFIG
 *
 * @return
 *      - ESP_OK            : 初始化并启动成功
 *      - ESP_ERR_INVALID_ARG: 参数无效（如broker_uri为空）
 *      - ESP_ERR_NO_MEM    : 内存不足
 *      - 其它esp_err_t     : 由底层组件返回的错误码
 */
esp_err_t mqtt_manager_init(const mqtt_manager_config_t *config); // 初始化函数声明

/**
 * @brief 反初始化MQTT管理器
 * 
 * - 销毁MQTT Client句柄
 * - 停止管理任务
 * - 取消事件订阅
 * 
 * @return esp_err_t 反初始化结果
 */
esp_err_t mqtt_manager_deinit(void);                // 反初始化函数声明

/**
 * @brief 启动MQTT连接
 * 
 * 手动触发连接尝试（通常由状态机自动处理）
 * 
 * @return esp_err_t 启动结果
 */
esp_err_t mqtt_manager_start(void);                 // 手动启动函数声明

/**
 * @brief 停止MQTT连接
 * 
 * 手动停止MQTT客户端
 * 
 * @return esp_err_t 停止结果
 */
esp_err_t mqtt_manager_stop(void);                  // 手动停止函数声明

/**
 * @brief 发布消息
 * 
 * @param topic 主题
 * @param data 负载数据
 * @param len 数据长度
 * @param qos 服务质量 (0, 1, 2)
 * @return esp_err_t 发布请求提交结果
 */
esp_err_t mqtt_manager_publish(const char *topic, const void *data, size_t len, int qos); // 发布函数声明

/**
 * @brief 订阅主题
 * 
 * @param topic 主题过滤器
 * @param qos 服务质量
 * @return esp_err_t 订阅请求提交结果
 */
esp_err_t mqtt_manager_subscribe(const char *topic, int qos); // 订阅函数声明

/**
 * @brief 取消订阅主题
 * 
 * @param topic 主题过滤器
 * @return esp_err_t 取消订阅请求提交结果
 */
esp_err_t mqtt_manager_unsubscribe(const char *topic); // 取消订阅函数声明

/**
 * @brief 获取当前MQTT管理器状态
 * 
 * @return mqtt_manager_state_t 当前状态
 */
mqtt_manager_state_t mqtt_manager_get_state(void);  // 获取状态函数声明

/**
 * @brief 检查MQTT是否已连接
 * 
 * @return true 已连接到Broker
 * @return false 未连接
 */
bool mqtt_manager_is_connected(void);               // 检查连接状态函数声明

/**
 * @brief 获取当前MQTT客户端ID
 * 
 * @return const char* 客户端ID字符串
 */
const char *mqtt_manager_get_client_id(void);       // 获取客户端ID函数声明

#ifdef __cplusplus                                  // 如果是C++编译器
}
#endif                                              // 结束C++编译器判断

#endif /* MQTT_MANAGER_H */                         // 结束头文件保护
