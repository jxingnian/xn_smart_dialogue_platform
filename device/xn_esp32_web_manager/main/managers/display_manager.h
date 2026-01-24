/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24 20:00:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-24 20:00:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\display_manager.h
 * @Description: 显示管理器头文件 - 负责 UI 业务逻辑和事件响应
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "esp_err.h"
#include "app_state_machine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 *                          类型定义
 *===========================================================================*/

/**
 * @brief UI 页面枚举
 */
typedef enum {
    UI_PAGE_HOME,           ///< 主页面（仪表盘）
    UI_PAGE_WIFI,           ///< WiFi 页面
    UI_PAGE_STATUS,         ///< 状态页面
    UI_PAGE_SETTINGS,       ///< 设置页面
    UI_PAGE_OTA,            ///< OTA 页面
    UI_PAGE_ERROR,          ///< 错误页面
    UI_PAGE_MAX,            ///< 页面数量
} ui_page_t;

/*===========================================================================
 *                          API
 *===========================================================================*/

/**
 * @brief 初始化显示管理器
 * 
 * 执行流程：
 * - 初始化 xn_display 组件
 * - 创建所有 UI 页面
 * - 订阅事件总线（监听系统状态变化）
 * - 显示初始页面
 * 
 * @return esp_err_t 
 *         - ESP_OK: 初始化成功
 *         - 其他: 初始化失败
 */
esp_err_t display_manager_init(void);

/**
 * @brief 反初始化显示管理器
 * 
 * 执行流程：
 * - 取消订阅事件总线
 * - 销毁所有 UI 页面
 * - 反初始化 xn_display 组件
 * 
 * @return esp_err_t 
 *         - ESP_OK: 反初始化成功
 *         - 其他: 反初始化失败
 */
esp_err_t display_manager_deinit(void);

/**
 * @brief 切换到指定页面
 * 
 * @param page 目标页面
 * @return esp_err_t 
 *         - ESP_OK: 切换成功
 *         - ESP_ERR_INVALID_ARG: 页面参数无效
 */
esp_err_t display_manager_show_page(ui_page_t page);

/**
 * @brief 更新主页面数据
 * 
 * 更新主页面显示的系统状态信息
 * 
 * @param state 当前系统状态
 * @param wifi_ssid WiFi SSID（可为 NULL）
 * @param wifi_rssi WiFi 信号强度（dBm）
 * @param ip_addr IP 地址（网络字节序）
 * @param mqtt_connected MQTT 连接状态
 * @return esp_err_t 
 *         - ESP_OK: 更新成功
 *         - 其他: 更新失败
 */
esp_err_t display_manager_update_home(
    app_state_t state,
    const char *wifi_ssid,
    int8_t wifi_rssi,
    uint32_t ip_addr,
    bool mqtt_connected
);

/**
 * @brief 更新 WiFi 页面数据
 * 
 * @param ssid WiFi SSID
 * @param rssi 信号强度（dBm）
 * @param status 连接状态文本
 * @return esp_err_t 
 *         - ESP_OK: 更新成功
 *         - 其他: 更新失败
 */
esp_err_t display_manager_update_wifi(
    const char *ssid,
    int8_t rssi,
    const char *status
);

/**
 * @brief 更新 OTA 页面进度
 * 
 * @param progress 进度百分比 (0-100)
 * @param status 状态文本
 * @return esp_err_t 
 *         - ESP_OK: 更新成功
 *         - 其他: 更新失败
 */
esp_err_t display_manager_update_ota(
    uint8_t progress,
    const char *status
);

/**
 * @brief 显示错误信息
 * 
 * 切换到错误页面并显示错误消息
 * 
 * @param error_msg 错误消息
 * @return esp_err_t 
 *         - ESP_OK: 显示成功
 *         - 其他: 显示失败
 */
esp_err_t display_manager_show_error(const char *error_msg);

/**
 * @brief 显示通知消息（Toast）
 * 
 * 在当前页面上方显示一个临时通知消息
 * 
 * @param msg 消息内容
 * @param duration_ms 显示时长（毫秒）
 * @return esp_err_t 
 *         - ESP_OK: 显示成功
 *         - 其他: 显示失败
 */
esp_err_t display_manager_show_toast(const char *msg, uint32_t duration_ms);

/**
 * @brief 设置屏幕亮度
 * 
 * @param brightness 亮度值 (0-100)
 * @return esp_err_t 
 *         - ESP_OK: 设置成功
 *         - 其他: 设置失败
 */
esp_err_t display_manager_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_MANAGER_H */
