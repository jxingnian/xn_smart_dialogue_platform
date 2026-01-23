/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:35:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_blufi\include\xn_blufi.h
 * @Description: BluFi蓝牙配网组件 - 接口定义 (Refactored)
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#pragma once // 防止头文件重复包含

#include "esp_err.h" // 包含ESP错误码定义
#include <stdint.h> // 包含标准整型定义
#include <stdbool.h> // 包含布尔类型定义

#ifdef __cplusplus // 如果是C++编译器
extern "C" { // 使用C链接约定
#endif // 结束C++编译器判断

// 前置声明 BluFi实例结构体
typedef struct xn_blufi_s xn_blufi_t;

/**
 * @brief BluFi回调函数集结构体定义
 */
typedef struct {
    /**
     * @brief 收到配网信息 (SSID/Password) 回调
     * @param blufi 实例指针
     * @param ssid WiFi名称
     * @param password WiFi密码
     */
    void (*on_recv_sta_config)(xn_blufi_t *blufi, const char *ssid, const char *password);

    /**
     * @brief 手机请求连接WiFi 回调
     * @param blufi 实例指针
     */
    void (*on_connect_request)(xn_blufi_t *blufi);

    /**
     * @brief 手机请求断开WiFi 回调
     * @param blufi 实例指针
     */
    void (*on_disconnect_request)(xn_blufi_t *blufi);

    /**
     * @brief 手机请求扫描WiFi 回调
     * @param blufi 实例指针
     */
    void (*on_scan_request)(xn_blufi_t *blufi);

    /**
     * @brief 收到自定义数据 回调
     * @param blufi 实例指针
     * @param data 数据内容指针
     * @param len 数据长度
     */
    void (*on_recv_custom_data)(xn_blufi_t *blufi, uint8_t *data, size_t len);

    /**
     * @brief 手机请求获取WiFi状态 回调
     * @param blufi 实例指针
     */
    void (*on_request_wifi_status)(xn_blufi_t *blufi);

} xn_blufi_callbacks_t; // BluFi回调结构体类型定义

/**
 * @brief 创建BluFi实例
 * 
 * @param device_name 蓝牙设备名称
 * @return xn_blufi_t* 返回实例指针，失败返回NULL
 */
xn_blufi_t* xn_blufi_create(const char *device_name); // 创建实例函数声明

/**
 * @brief 销毁BluFi实例
 * 
 * @param blufi 实例指针
 */
void xn_blufi_destroy(xn_blufi_t *blufi); // 销毁实例函数声明

/**
 * @brief 初始化并启动BluFi服务
 * 
 * @param blufi 实例指针
 * @param callbacks 回调函数结构体指针
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_blufi_init(xn_blufi_t *blufi, xn_blufi_callbacks_t *callbacks); // 初始化函数声明

/**
 * @brief 停止并反初始化BluFi服务
 * 
 * @param blufi 实例指针
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_blufi_deinit(xn_blufi_t *blufi); // 反初始化函数声明

/**
 * @brief 发送WiFi扫描结果给手机
 * 
 * @param ap_count AP数量
 * @param ap_list ESP-IDF wifi_ap_record_t 数组指针
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_blufi_send_wifi_list(uint16_t ap_count, void *ap_list); // 发送扫描结果函数声明

/**
 * @brief 发送连接报告给手机
 * 
 * @param connected 是否连接成功
 * @param ssid 连接的SSID
 * @param rssi 信号强度
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_blufi_send_connect_report(bool connected, const char *ssid, int rssi); // 发送连接报告函数声明

/**
 * @brief 发送自定义数据给手机
 * 
 * @param data 数据指针
 * @param len 数据长度
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_blufi_send_custom_data(uint8_t *data, size_t len); // 发送自定义数据函数声明

/**
 * @brief 检查蓝牙是否已连接
 * 
 * @param blufi 实例指针
 * @return true 已连接
 * @return false 未连接
 */
bool xn_blufi_is_ble_connected(xn_blufi_t *blufi); // 检查蓝牙连接状态函数声明

#ifdef __cplusplus // 如果是C++编译器
}
#endif // 结束C++编译器判断
