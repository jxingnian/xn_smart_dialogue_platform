/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\wifi_manager.h
 * @Description: WiFi应用管理器头文件 - 通过事件与其他模块通信
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
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
 * 
 * - 创建事件组
 * - 初始化底层 Netif 和 WiFi Driver
 * - 注册默认事件处理函数
 * - 设置为 STA 模式
 * 
 * @return esp_err_t 初始化结果
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief 反初始化WiFi管理器
 * 
 * - 停止 WiFi
 * - 注销事件处理函数
 * - 释放资源
 * 
 * @return esp_err_t 反初始化结果
 */
esp_err_t wifi_manager_deinit(void);

/**
 * @brief 启动WiFi连接
 * 
 * 使用 NVS 中已保存的 SSID/Password 尝试连接。
 * 如果没有保存的配置，将无法连接成功。
 * 
 * @return esp_err_t 启动结果
 */
esp_err_t wifi_manager_start(void);

/**
 * @brief 停止WiFi连接
 * 
 * 调用 esp_wifi_stop()，彻底关闭 WiFi 射频。
 * 
 * @return esp_err_t 停止结果
 */
esp_err_t wifi_manager_stop(void);

/**
 * @brief 使用指定凭据连接WiFi
 * 
 * 更新 WiFi 配置并立即启动连接。配置将自动保存到 NVS。
 * 
 * @param ssid WiFi名称
 * @param password WiFi密码（开放网络可为NULL或空串）
 * @return esp_err_t 连接请求提交结果
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief 断开WiFi连接
 * 
 * 仅断开当前连接，不关闭 WiFi 射频。
 * 
 * @return esp_err_t 断开请求结果
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief 检查WiFi是否已连接且获取到IP
 * 
 * @return true 已连接且有IP
 * @return false 未连接或正在连接
 */
bool wifi_manager_is_connected(void);

/**
 * @brief 获取当前IP地址
 * 
 * @return uint32_t IPv4地址（网络字节序）
 */
uint32_t wifi_manager_get_ip(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */

