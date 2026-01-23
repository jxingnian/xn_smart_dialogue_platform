/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-23 12:45:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:15:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_wifi\include\xn_wifi.h
 * @Description: WiFi底层组件头文件 - 封装ESP-IDF WiFi Station功能
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#pragma once // 防止头文件重复包含

#include "esp_err.h" // 包含ESP错误码定义
#include "esp_wifi_types.h" // 包含ESP WiFi类型定义

#ifdef __cplusplus // 如果是C++编译器
extern "C" { // 使用C链接约定
#endif // 结束C++编译器判断

// WiFi状态枚举定义
typedef enum {
    XN_WIFI_DISCONNECTED, // WiFi断开连接状态
    XN_WIFI_CONNECTING, // WiFi正在连接状态
    XN_WIFI_CONNECTED,  // 物理连接成功，但可能未获取IP
    XN_WIFI_GOT_IP,     // 成功获取IP地址，WiFi完全可用
} xn_wifi_status_t; // WiFi状态类型定义

// 扫描完成回调函数原型定义
// @param ap_count: 扫描到的热点数量
// @param ap_list: 热点列表数据指针
typedef void (*xn_wifi_scan_done_cb_t)(uint16_t ap_count, wifi_ap_record_t *ap_list); 

// 状态变化回调函数原型定义
// @param status: 当前最新的WiFi状态
typedef void (*xn_wifi_status_cb_t)(xn_wifi_status_t status);

// WiFi组件句柄结构体前置声明
typedef struct xn_wifi_s xn_wifi_t;

/**
 * @brief 创建WiFi组件实例
 * 
 * @return xn_wifi_t* 返回创建的WiFi实例指针，失败返回NULL
 */
xn_wifi_t* xn_wifi_create(void); // 创建WiFi实例函数声明

/**
 * @brief 销毁WiFi组件实例
 * 
 * @param wifi WiFi实例指针
 */
void xn_wifi_destroy(xn_wifi_t *wifi); // 销毁WiFi实例函数声明

/**
 * @brief 初始化WiFi组件 (初始化Netif, EventLoop, WiFi Driver)
 * 
 * @param wifi WiFi实例指针
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_wifi_init(xn_wifi_t *wifi); // 初始化WiFi组件函数声明

/**
 * @brief 反初始化WiFi组件
 * 
 * @param wifi WiFi实例指针
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_wifi_deinit(xn_wifi_t *wifi); // 反初始化WiFi组件函数声明

/**
 * @brief 连接WiFi
 * 
 * @param wifi WiFi实例指针
 * @param ssid WiFi名称
 * @param password WiFi密码
 * @return esp_err_t 返回ESP_OK表示成功发起连接，其他表示失败
 */
esp_err_t xn_wifi_connect(xn_wifi_t *wifi, const char *ssid, const char *password); // 连接WiFi函数声明

/**
 * @brief 断开WiFi
 * 
 * @param wifi WiFi实例指针
 * @return esp_err_t 返回ESP_OK表示成功发起断开，其他表示失败
 */
esp_err_t xn_wifi_disconnect(xn_wifi_t *wifi); // 断开WiFi函数声明

/**
 * @brief 扫描WiFi
 * 
 * @param wifi WiFi实例指针
 * @param callback 扫描完成时的回调函数
 * @return esp_err_t 返回ESP_OK表示成功发起扫描，其他表示失败
 */
esp_err_t xn_wifi_scan(xn_wifi_t *wifi, xn_wifi_scan_done_cb_t callback); // 扫描WiFi函数声明

/**
 * @brief 获取当前WiFi状态
 * 
 * @param wifi WiFi实例指针
 * @return xn_wifi_status_t 返回当前状态
 */
xn_wifi_status_t xn_wifi_get_status(xn_wifi_t *wifi); // 获取WiFi状态函数声明

/**
 * @brief 注册状态变化回调
 * 
 * @param wifi WiFi实例指针
 * @param callback 状态变化时的回调函数
 */
void xn_wifi_register_status_cb(xn_wifi_t *wifi, xn_wifi_status_cb_t callback); // 注册状态回调函数声明

/**
 * @brief 获取当前连接的SSID
 * 
 * @param wifi WiFi实例指针
 * @param ssid 输出缓冲区 (必须 >= 33字节)
 * @return esp_err_t 成功返回ESP_OK，未连接返回错误
 */
esp_err_t xn_wifi_get_current_ssid(xn_wifi_t *wifi, char *ssid);

#ifdef __cplusplus // 如果是C++编译器
}
#endif // 结束C++编译器判断
