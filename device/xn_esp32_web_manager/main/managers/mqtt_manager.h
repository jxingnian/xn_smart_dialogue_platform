/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\mqtt_manager.h
 * @Description: MQTT应用管理器头文件 - 通过事件与其他模块通信
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化MQTT管理器
 * 
 * - 配置 Broker 地址（默认为 mqtt://broker.emqx.io:1883）
 * - 创建 MQTT Client 句柄
 * - 注册 MQTT 事件回调
 * - 订阅相关系统事件 (CMD_MQTT_CONNECT, EVT_WIFI_GOT_IP 等)
 * 
 * @return esp_err_t 初始化结果
 */
esp_err_t mqtt_manager_init(void);

/**
 * @brief 反初始化MQTT管理器
 * 
 * - 销毁 MQTT Client 句柄
 * - 取消事件订阅
 * 
 * @return esp_err_t 反初始化结果
 */
esp_err_t mqtt_manager_deinit(void);

/**
 * @brief 连接MQTT服务器
 * 
 * 启动 MQTT 客户端，尝试建立连接。
 * 
 * @return esp_err_t 启动结果
 */
esp_err_t mqtt_manager_connect(void);

/**
 * @brief 断开MQTT连接
 * 
 * 停止 MQTT 客户端。
 * 
 * @return esp_err_t 停止结果
 */
esp_err_t mqtt_manager_disconnect(void);

/**
 * @brief 发布消息
 * 
 * @param topic 主题
 * @param data 负载数据（字符串）
 * @param len 数据长度
 * @param qos 服务质量 (0, 1, 2)
 * @return esp_err_t 发布请求提交结果
 */
esp_err_t mqtt_manager_publish(const char *topic, const void *data, size_t len, int qos);

/**
 * @brief 订阅主题
 * 
 * @param topic 主题过滤器
 * @param qos 服务质量
 * @return esp_err_t 订阅请求提交结果
 */
esp_err_t mqtt_manager_subscribe(const char *topic, int qos);

/**
 * @brief 检查MQTT是否已连接
 * 
 * @return true 已连接到Broker
 * @return false 未连接
 */
bool mqtt_manager_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_MANAGER_H */

