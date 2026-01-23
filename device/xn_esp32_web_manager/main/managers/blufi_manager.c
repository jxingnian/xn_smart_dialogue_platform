/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\blufi_manager.c
 * @Description: BluFi配网应用管理器实现（精简版）- 管理配网流程
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "xn_event_bus.h"
#include "blufi_manager.h"

static const char *TAG = "blufi_manager";

/*===========================================================================
 *                          内部变量
 *===========================================================================*/

static bool s_initialized = false;          ///< 初始化标志
static bool s_running = false;              ///< 运行状态标志

/*===========================================================================
 *                          命令事件处理
 *===========================================================================*/

/**
 * @brief XN事件总线命令回调
 * 
 * 监听开始和停止BluFi的命令。
 */
static void cmd_event_handler(const xn_event_t *event, void *user_data)
{
    switch (event->id) {
        case XN_CMD_BLUFI_START:
            // 打印开始命令日志
            ESP_LOGI(TAG, "Received BLUFI_START command");
            // 执行启动BluFi
            blufi_manager_start();
            break;
            
        case XN_CMD_BLUFI_STOP:
            // 打印停止命令日志
            ESP_LOGI(TAG, "Received BLUFI_STOP command");
            // 执行停止BluFi
            blufi_manager_stop();
            break;
            
        default:
            break;
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t blufi_manager_init(void)
{
    // 检查是否已初始化
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 订阅开始命令事件
    xn_event_subscribe(XN_CMD_BLUFI_START, cmd_event_handler, NULL);
    // 订阅停止命令事件
    xn_event_subscribe(XN_CMD_BLUFI_STOP, cmd_event_handler, NULL);
    
    // 标记初始化完成
    s_initialized = true;
    // 打印初始化成功日志（提示这是stub模式，完整功能需开启BT）
    ESP_LOGI(TAG, "BluFi manager initialized (stub mode - enable BT in menuconfig for full support)");
    
    // 返回成功
    return ESP_OK;
}

esp_err_t blufi_manager_deinit(void)
{
    // 检查是否已初始化
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 如果正在运行，先停止
    if (s_running) {
        blufi_manager_stop();
    }
    
    // 取消所有命令订阅
    xn_event_unsubscribe_all(cmd_event_handler);
    
    // 重置初始化标志
    s_initialized = false;
    
    // 打印反初始化完成日志
    ESP_LOGI(TAG, "BluFi manager deinitialized");
    
    // 返回成功
    return ESP_OK;
}

esp_err_t blufi_manager_start(void)
{
    // 检查是否已初始化
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 检查是否已经在运行
    if (s_running) {
        ESP_LOGW(TAG, "BluFi already running");
        return ESP_OK; // 已经运行则视为成功
    }
    
    // TODO: 在 menuconfig 启用蓝牙后，添加以下代码：
    // 1. 初始化BT Controller
    // esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    // esp_bt_controller_init(&bt_cfg);
    // esp_bt_controller_enable(ESP_BT_MODE_BLE);
    
    // 2. 初始化Bluedroid Host
    // esp_bluedroid_init();
    // esp_bluedroid_enable();
    
    // 3. 注册回调并启动Profile
    // esp_blufi_register_callbacks(&blufi_callbacks);
    // esp_blufi_profile_init();
    
    // 打印提示日志，告知用户并未真正启动蓝牙栈
    ESP_LOGW(TAG, "BluFi start - Bluetooth not enabled in menuconfig");
    ESP_LOGW(TAG, "Please run: idf.py menuconfig -> Component config -> Bluetooth");
    
    // 标记运行状态
    s_running = true;
    
    // 模拟配网初始化完成事件（用于测试流程）
    xn_event_post(XN_EVT_BLUFI_INIT_DONE, XN_EVT_SRC_BLUFI);
    
    // 返回成功
    return ESP_OK;
}

esp_err_t blufi_manager_stop(void)
{
    // 检查运行状态
    if (!s_initialized || !s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: 释放蓝牙资源
    // esp_blufi_profile_deinit();
    // esp_bluedroid_disable();
    // esp_bluedroid_deinit();
    // esp_bt_controller_disable();
    // esp_bt_controller_deinit();
    
    // 标记停止运行
    s_running = false;
    // 打印停止日志
    ESP_LOGI(TAG, "BluFi stopped");
    
    // 返回成功
    return ESP_OK;
}

bool blufi_manager_is_running(void)
{
    // 返回当前运行状态
    return s_running;
}

