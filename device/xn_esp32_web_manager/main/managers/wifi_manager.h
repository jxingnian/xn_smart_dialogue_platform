/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:55:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\wifi_manager.h
 * @Description: WiFi应用管理器头文件
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#ifndef WIFI_MANAGER_H // 防止头文件重复包含
#define WIFI_MANAGER_H // 防止头文件重复包含

#include "esp_err.h" // 包含ESP错误码定义
#include <stdbool.h> // 包含布尔类型定义
#include <stdint.h> // 包含标准整型定义
#include "xn_wifi.h" // 引用xn_wifi中的回调定义

#ifdef __cplusplus // 如果是C++编译器
extern "C" { // 使用C链接约定
#endif // 结束C++编译器判断

/**
 * @brief 初始化WiFi管理器
 * 
 * - 初始化NVS存储
 * - 创建xn_wifi实例
 * - 注册内部状态回调
 * - 订阅系统命令事件
 * 
 * @return esp_err_t 初始化结果
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief 反初始化WiFi管理器
 * 
 * - 取消事件订阅
 * - 销毁xn_wifi实例
 * 
 * @return esp_err_t 反初始化结果
 */
esp_err_t wifi_manager_deinit(void);

/**
 * @brief 启动WiFi管理器
 * 
 * - 尝试连接最近一次保存的WiFi配置
 * 
 * @return esp_err_t 启动结果
 */
esp_err_t wifi_manager_start(void);

/**
 * @brief 停止WiFi管理器
 * 
 * - 断开当前连接
 * 
 * @return esp_err_t 停止结果
 */
esp_err_t wifi_manager_stop(void);

/**
 * @brief 连接指定WiFi
 * 
 * - 保存SSID和密码到NVS
 * - 调用底层连接接口
 * 
 * @param ssid WiFi名称
 * @param password WiFi密码
 * @return esp_err_t 连接请求结果
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief 断开当前WiFi连接
 * 
 * @return esp_err_t 断开请求结果
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief 检查是否已连接并获取到IP
 * 
 * @return true 已连接且获取IP
 * @return false 未连接或未获取IP
 */
bool wifi_manager_is_connected(void);

/**
 * @brief 获取当前IP地址
 * 
 * @return uint32_t IP地址（网络字节序或根据具体实现）
 */
uint32_t wifi_manager_get_ip(void);

/**
 * @brief 扫描附近WiFi
 * 
 * @param callback 扫描完成时的回调函数
 * @return esp_err_t 扫描请求结果
 */
esp_err_t wifi_manager_scan(xn_wifi_scan_done_cb_t callback);

#ifdef __cplusplus // 如果是C++编译器
}
#endif // 结束C++编译器判断

#endif // WIFI_MANAGER_H
