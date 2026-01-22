/**
 * @file wifi_manager.h
 * @brief WiFi应用管理器 - 通过事件与其他模块通信
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化WiFi管理器
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief 反初始化WiFi管理器
 */
esp_err_t wifi_manager_deinit(void);

/**
 * @brief 启动WiFi连接（使用已保存的凭据）
 */
esp_err_t wifi_manager_start(void);

/**
 * @brief 停止WiFi连接
 */
esp_err_t wifi_manager_stop(void);

/**
 * @brief 使用指定凭据连接WiFi
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief 断开WiFi连接
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief 检查WiFi是否已连接
 */
bool wifi_manager_is_connected(void);

/**
 * @brief 获取当前IP地址
 */
uint32_t wifi_manager_get_ip(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */
