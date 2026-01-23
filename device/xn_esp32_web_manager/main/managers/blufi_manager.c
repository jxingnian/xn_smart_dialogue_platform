/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 11:16:16
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\blufi_manager.c
 * @Description: BluFi配网应用管理器实现 - 管理配网流程
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "xn_event_bus.h"
#include "blufi_manager.h"
#include "xn_blufi.h"
#include "esp_wifi.h"
#include "xn_wifi_storage.h"

static const char *TAG = "blufi_manager";

#define BLUFI_DEVICE_NAME "XN_SMART_DEVICE"

/*===========================================================================
 *                          内部变量
 *===========================================================================*/

static bool s_initialized = false;          ///< 初始化标志
static bool s_running = false;              ///< 运行状态标志
static xn_blufi_t *s_blufi_instance = NULL; ///< 底层BluFi组件实例

/*===========================================================================
 *                          命令事件处理
 *===========================================================================*/

/**
 * @brief 系统事件处理回调
 * 
 * 监听 WiFi 状态事件，实现配网成功后的自动保存和退出逻辑。
 */
static void system_event_handler(const xn_event_t *event, void *user_data)
{
    if (event->id == XN_EVT_WIFI_GOT_IP) {
        // 仅在BluFi运行时处理，避免正常连接时的误触发
        if (s_running) {
            ESP_LOGI(TAG, "WiFi已连接，正在保存配置并停止BluFi...");
            
            // 获取当前WiFi配置
            wifi_config_t conf;
            esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &conf);
            if (ret == ESP_OK) {
                // 保存到NVS
                xn_wifi_storage_save((const char *)conf.sta.ssid, (const char *)conf.sta.password);
                ESP_LOGI(TAG, "WiFi配置已保存: %s", conf.sta.ssid);
            } else {
                ESP_LOGE(TAG, "获取WiFi配置失败");
            }
            
            // 停止BluFi服务
            blufi_manager_stop();
            
            // 通知系统配网完成（如果需要更细粒度的控制，可以定义专门的事件）
             xn_event_post(XN_EVT_BLUFI_CONFIG_DONE, XN_EVT_SRC_BLUFI);
        }
    }
}

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
    
    // 创建底层BluFi组件实例
    // 注意：这里只分配内存和基础资源，不初始化蓝牙栈，节省资源
    s_blufi_instance = xn_blufi_create(BLUFI_DEVICE_NAME);
    if (s_blufi_instance == NULL) {
        ESP_LOGE(TAG, "Failed to create BluFi instance");
        return ESP_FAIL;
    }

    // 订阅开始命令事件
    xn_event_subscribe(XN_CMD_BLUFI_START, cmd_event_handler, NULL);
    // 订阅停止命令事件
    xn_event_subscribe(XN_CMD_BLUFI_STOP, cmd_event_handler, NULL);
    // 订阅WiFi连接成功事件，用于自动保存配置
    xn_event_subscribe(XN_EVT_WIFI_GOT_IP, system_event_handler, NULL);
    
    // 标记初始化完成
    s_initialized = true;
    // 打印初始化成功日志
    ESP_LOGI(TAG, "BluFi manager initialized");
    
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
    
    // 销毁底层实例
    if (s_blufi_instance) {
        xn_blufi_destroy(s_blufi_instance);
        s_blufi_instance = NULL;
    }
    
    // 取消所有命令订阅
    xn_event_unsubscribe_all(cmd_event_handler);
    xn_event_unsubscribe(XN_EVT_WIFI_GOT_IP, system_event_handler);
    
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
    if (!s_initialized || s_blufi_instance == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 检查是否已经在运行
    if (s_running) {
        ESP_LOGW(TAG, "BluFi already running");
        return ESP_OK; // 已经运行则视为成功
    }
    
    ESP_LOGI(TAG, "Starting BluFi service...");
    
    // 调用底层组件初始化：启动蓝牙协议栈、GATT服务、开启广播
    esp_err_t ret = xn_blufi_init(s_blufi_instance);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start xn_blufi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 标记运行状态
    s_running = true;
    
    // 发送配网初始化完成/开始事件
    // 注意：这里发送 INIT_DONE 可能意味着 FSM 会认为配网完成了，需要根据 FSM 逻辑确认
    // 根据状态机逻辑，BLUFI_CONFIG 状态下等待 XN_EVT_BLUFI_CONFIG_DONE
    // 这里的 BLUFI_INIT_DONE 更多是通知系统蓝牙准备好了，可以暂时不处理或仅作日志
    xn_event_post(XN_EVT_BLUFI_INIT_DONE, XN_EVT_SRC_BLUFI);
    
    ESP_LOGI(TAG, "BluFi started successfully");
    
    // 返回成功
    return ESP_OK;
}

esp_err_t blufi_manager_stop(void)
{
    // 检查运行状态
    if (!s_initialized || !s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Stopping BluFi service...");
    
    // 调用底层组件反初始化：停止蓝牙协议栈
    // 注意：这将释放蓝牙 Controller 内存，如果之后要重新开启，需要确保 xn_blufi_init 能重新初始化
    esp_err_t ret = xn_blufi_deinit(s_blufi_instance);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop xn_blufi: %s", esp_err_to_name(ret));
        // 即使出错，我们也尝试标记为停止，以免状态卡死
    }
    
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

