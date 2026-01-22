/**
 * @file mqtt_manager.h
 * @brief MQTT应用管理器 - 通过事件与其他模块通信
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
 */
esp_err_t mqtt_manager_init(void);

/**
 * @brief 反初始化MQTT管理器
 */
esp_err_t mqtt_manager_deinit(void);

/**
 * @brief 连接MQTT服务器
 */
esp_err_t mqtt_manager_connect(void);

/**
 * @brief 断开MQTT连接
 */
esp_err_t mqtt_manager_disconnect(void);

/**
 * @brief 发布消息
 */
esp_err_t mqtt_manager_publish(const char *topic, const char *data, int qos);

/**
 * @brief 订阅主题
 */
esp_err_t mqtt_manager_subscribe(const char *topic, int qos);

/**
 * @brief 检查MQTT是否已连接
 */
bool mqtt_manager_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_MANAGER_H */
