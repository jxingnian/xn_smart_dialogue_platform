/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_blufi\include\xn_blufi.h
 * @Description: BluFi蓝牙配网组件 - 头文件
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef XN_BLUFI_H
#define XN_BLUFI_H

#include "esp_err.h"
#include "esp_wifi.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 引入WiFi管理器和存储层的类型定义
#include "xn_wifi_manager.h"
#include "xn_wifi_storage.h"

/* BluFi配网组件类前置声明 */
typedef struct xn_blufi_s xn_blufi_t;

/**
 * @brief 创建BluFi配网组件实例
 * 
 * 分配内存并初始化基本的组件结构。
 * 
 * @param device_name 蓝牙设备名称，将显示在手机小程序或蓝牙列表中
 * @return xn_blufi_t* 组件实例指针，如果内存分配失败返回NULL
 */
xn_blufi_t* xn_blufi_create(const char *device_name);

/**
 * @brief 销毁BluFi配网组件实例
 * 
 * 释放组件占用的所有资源，包括WiFi管理器和内部结构体。
 * 
 * @param blufi 组件实例指针，如果为NULL则不做任何操作
 */
void xn_blufi_destroy(xn_blufi_t *blufi);

/**
 * @brief 初始化BluFi配网组件
 * 
 * 初始化底层蓝牙协议栈（NimBLE）、WiFi管理器和NVS存储。
 * 启动蓝牙广播，等待设备连接。
 * 
 * @param blufi 组件实例指针
 * @return esp_err_t 
 *      - ESP_OK: 初始化成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - 其他: 底层初始化失败代码
 */
esp_err_t xn_blufi_init(xn_blufi_t *blufi);

/**
 * @brief 反初始化BluFi配网组件
 * 
 * 停止蓝牙协议栈，释放相关资源。
 * 
 * @param blufi 组件实例指针
 * @return esp_err_t 
 *      - ESP_OK: 反初始化成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t xn_blufi_deinit(xn_blufi_t *blufi);

/**
 * @brief 连接到指定WiFi
 * 
 * @param blufi 组件实例指针
 * @param ssid WiFi名称（SSID）
 * @param password WiFi密码
 * @return esp_err_t 
 *      - ESP_OK: 连接请求发送成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - ESP_FAIL: 连接失败
 */
esp_err_t xn_blufi_wifi_connect(xn_blufi_t *blufi, const char *ssid, const char *password);

/**
 * @brief 断开当前WiFi连接
 * 
 * @param blufi 组件实例指针
 * @return esp_err_t 
 *      - ESP_OK: 断开请求发送成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t xn_blufi_wifi_disconnect(xn_blufi_t *blufi);

/**
 * @brief 保存WiFi配置到NVS
 * 
 * @param blufi 组件实例指针
 * @param ssid WiFi名称
 * @param password WiFi密码
 * @return esp_err_t 
 *      - ESP_OK: 保存成功
 *      - 其他: NVS写入失败
 */
esp_err_t xn_blufi_wifi_save(xn_blufi_t *blufi, const char *ssid, const char *password);

/**
 * @brief 从NVS删除WiFi配置
 * 
 * @param blufi 组件实例指针
 * @return esp_err_t 
 *      - ESP_OK: 删除成功
 *      - 其他: NVS擦除失败
 */
esp_err_t xn_blufi_wifi_delete(xn_blufi_t *blufi);

/**
 * @brief 从NVS加载WiFi配置
 * 
 * @param blufi 组件实例指针
 * @param config 输出参数，用于保存加载的配置信息
 * @return esp_err_t 
 *      - ESP_OK: 加载成功
 *      - ESP_ERR_NVS_NOT_FOUND: 未找到配置
 */
esp_err_t xn_blufi_wifi_load(xn_blufi_t *blufi, xn_wifi_config_t *config);

/**
 * @brief 扫描周围WiFi
 * 
 * @param blufi 组件实例指针
 * @param callback 扫描完成时的回调函数
 * @return esp_err_t 
 *      - ESP_OK: 扫描请求发送成功
 *      - ESP_FAIL: 扫描启动失败
 */
esp_err_t xn_blufi_wifi_scan(xn_blufi_t *blufi, xn_wifi_scan_done_cb_t callback);

/**
 * @brief 获取当前WiFi连接状态
 * 
 * @param blufi 组件实例指针
 * @return xn_wifi_status_t WiFi连接状态枚举值
 */
xn_wifi_status_t xn_blufi_wifi_get_status(xn_blufi_t *blufi);

/**
 * @brief 注册WiFi状态变化回调
 * 
 * @param blufi 组件实例指针
 * @param callback 状态变化时的回调函数
 */
void xn_blufi_wifi_register_status_cb(xn_blufi_t *blufi, xn_wifi_status_cb_t callback);

/**
 * @brief 获取蓝牙连接状态
 * 
 * @param blufi 组件实例指针
 * @return true 表示蓝牙设备已连接
 * @return false 表示蓝牙设备未连接
 */
bool xn_blufi_is_ble_connected(xn_blufi_t *blufi);

#ifdef __cplusplus
}
#endif

#endif // XN_BLUFI_H
