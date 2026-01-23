/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_blufi\include\xn_wifi_manager.h
 * @Description: WiFi管理层 - 头文件
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef XN_WIFI_MANAGER_H
#define XN_WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_wifi.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi连接状态枚举 */
typedef enum {
    XN_WIFI_DISCONNECTED = 0,   // 未连接状态
    XN_WIFI_CONNECTING,         // 正在连接状态
    XN_WIFI_CONNECTED,          // 已连接到AP但尚未获取IP
    XN_WIFI_GOT_IP              // 已连接并成功获取IP
} xn_wifi_status_t;

/* WiFi扫描结果回调函数类型定义 */
typedef void (*xn_wifi_scan_done_cb_t)(uint16_t ap_count, wifi_ap_record_t *ap_list);

/* WiFi状态变化回调函数类型定义 */
typedef void (*xn_wifi_status_cb_t)(xn_wifi_status_t status);

/* WiFi管理器实例前置声明 */
typedef struct xn_wifi_manager_s xn_wifi_manager_t;

/**
 * @brief 创建WiFi管理器实例
 * 
 * 分配内存并初始化管理器结构体。
 * 
 * @return xn_wifi_manager_t* 管理器实例指针，失败返回NULL
 */
xn_wifi_manager_t* xn_wifi_manager_create(void);

/**
 * @brief 销毁WiFi管理器实例
 * 
 * 释放管理器占用的所有资源。
 * 
 * @param manager 管理器实例指针
 */
void xn_wifi_manager_destroy(xn_wifi_manager_t *manager);

/**
 * @brief 初始化WiFi管理器
 * 
 * 初始化底层WiFi栈、事件循环和网络接口。
 * 
 * @param manager 管理器实例指针
 * @return esp_err_t 
 *      - ESP_OK: 初始化成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - 其他: 底层初始化失败代码
 */
esp_err_t xn_wifi_manager_init(xn_wifi_manager_t *manager);

/**
 * @brief 反初始化WiFi管理器
 * 
 * 停止WiFi栈，注销事件处理程序，释放底层资源。
 * 
 * @param manager 管理器实例指针
 * @return esp_err_t 
 *      - ESP_OK: 反初始化成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t xn_wifi_manager_deinit(xn_wifi_manager_t *manager);

/**
 * @brief 连接到指定WiFi
 * 
 * @param manager 管理器实例指针
 * @param ssid WiFi名称（SSID）
 * @param password WiFi密码
 * @return esp_err_t 
 *      - ESP_OK: 连接请求发送成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - ESP_FAIL: 连接启动失败
 */
esp_err_t xn_wifi_manager_connect(xn_wifi_manager_t *manager, 
                                   const char *ssid, 
                                   const char *password);

/**
 * @brief 断开当前WiFi连接
 * 
 * @param manager 管理器实例指针
 * @return esp_err_t 
 *      - ESP_OK: 断开请求发送成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t xn_wifi_manager_disconnect(xn_wifi_manager_t *manager);

/**
 * @brief 扫描周围WiFi
 * 
 * @param manager 管理器实例指针
 * @param callback 扫描完成时的回调函数
 * @return esp_err_t 
 *      - ESP_OK: 扫描请求发送成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - ESP_FAIL: 扫描启动失败
 */
esp_err_t xn_wifi_manager_scan(xn_wifi_manager_t *manager, 
                                xn_wifi_scan_done_cb_t callback);

/**
 * @brief 获取当前WiFi连接状态
 * 
 * @param manager 管理器实例指针
 * @return xn_wifi_status_t 当前状态枚举值
 */
xn_wifi_status_t xn_wifi_manager_get_status(xn_wifi_manager_t *manager);

/**
 * @brief 注册WiFi状态变化回调
 * 
 * @param manager 管理器实例指针
 * @param callback 状态变化时的回调函数
 */
void xn_wifi_manager_register_status_cb(xn_wifi_manager_t *manager, 
                                         xn_wifi_status_cb_t callback);

#ifdef __cplusplus
}
#endif

#endif // XN_WIFI_MANAGER_H
