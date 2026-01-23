/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 16:16:47
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\blufi_manager.c
 * @Description: BluFi配网应用管理器实现 - 实现回调逻辑，解耦底层
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#include <string.h> // 包含字符串库
#include "freertos/FreeRTOS.h" // 包含FreeRTOS核心
#include "freertos/task.h" // 包含FreeRTOS任务
#include "esp_log.h" // 包含日志库
#include "xn_event_bus.h" // 包含事件总线库
#include "blufi_manager.h" // 包含Blufi管理器库
#include "xn_blufi.h" // 包含Blufi组件库
#include "wifi_manager.h" // 包含WiFi管理器库

static const char *TAG = "blufi_manager";

#define BLUFI_DEVICE_NAME "XN_SMART_DEVICE"

/*===========================================================================
 *                          内部变量
 *===========================================================================*/

static bool s_initialized = false;
static bool s_running = false;
static xn_blufi_t *s_blufi_instance = NULL;

/*===========================================================================
 *                          BluFi 回调实现
 *===========================================================================*/

// 收到配网配置回调
static void on_recv_sta_config(xn_blufi_t *blufi, const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "BluFi received config: SSID=%s", ssid);
    // 调用应用层WiFi管理器进行连接（并保存）
    wifi_manager_connect(ssid, password);
}

// 收到连接请求回调
static void on_connect_request(xn_blufi_t *blufi)
{
    ESP_LOGI(TAG, "BluFi requested connect");
    // 有些手机端流程是先发配置，再发连接请求
    // 我们在recv config时已经触发连接，这里可以用来做额外确认，或者如果recv_config只保存不连接的话
    // 但通常 recv_config 后直接连接体验更好，这里仅作日志
}

// 收到断开请求回调
static void on_disconnect_request(xn_blufi_t *blufi)
{
    ESP_LOGI(TAG, "BluFi requested disconnect");
    wifi_manager_disconnect();
}

// 扫描完成中转回调
static void on_wifi_scan_done(uint16_t ap_count, wifi_ap_record_t *ap_list)
{
    ESP_LOGI(TAG, "WiFi scan done, sending to BluFi phone");
    // 将扫描结果回传给手机
    xn_blufi_send_wifi_list(ap_count, ap_list);
}

// 收到扫描请求回调
static void on_scan_request(xn_blufi_t *blufi)
{
    ESP_LOGI(TAG, "BluFi requested scan");
    // 调用WiFi管理器执行扫描
    wifi_manager_scan(on_wifi_scan_done);
}

// 收到自定义数据回调
static void on_recv_custom_data(xn_blufi_t *blufi, uint8_t *data, size_t len)
{
    ESP_LOGI(TAG, "BluFi received custom data len=%d", len);
    if (len < 1) return;

    uint8_t type = data[0];
    
    if (type == 0x01) { // GET stored configs
        ESP_LOGI(TAG, "Custom Request: Get Stored Configs");
        uint8_t count = wifi_manager_get_stored_configs_count();
        
        // 构建响应：[0x01(type), 0x00(status), count, (ssid_len, ssid, pwd_len, pwd)...]
        // 预估最大长度：3 + 10 * (1+32+1+64) = ~1000 bytes. 动态分配或使用较大buffer
        // 这里只是简单示例，注意栈溢出，用 heap
        uint8_t *resp = malloc(1024);
        if (!resp) return;

        int offset = 0;
        resp[offset++] = 0x01; // Type
        resp[offset++] = 0x00; // Status OK
        resp[offset++] = count; // Count

        char ssid[33], pwd[65];
        for (int i = 0; i < count; i++) {
            if (wifi_manager_get_stored_config(i, ssid, pwd) == ESP_OK) {
                // SSID
                size_t s_len = strlen(ssid);
                resp[offset++] = (uint8_t)s_len;
                memcpy(resp + offset, ssid, s_len);
                offset += s_len;
                
                // Password
                size_t p_len = strlen(pwd);
                resp[offset++] = (uint8_t)p_len;
                memcpy(resp + offset, pwd, p_len);
                offset += p_len;
                
                if (offset > 1000) break; // 防止溢出
            }
        }
        
        xn_blufi_send_custom_data(resp, offset);
        free(resp);

    } else if (type == 0x02) { // DEL stored config
        if (len < 2) return;
        uint8_t index = data[1];
        ESP_LOGI(TAG, "Custom Request: Delete Config Index %d", index);
        
        esp_err_t ret = wifi_manager_delete_stored_config(index);
        
        // 响应：[0x02(type), status]
        uint8_t resp[2];
        resp[0] = 0x02;
        resp[1] = (ret == ESP_OK) ? 0x00 : 0x01;
        
        xn_blufi_send_custom_data(resp, 2);
    }
}

// 收到WiFi状态请求回调
static void on_request_wifi_status(xn_blufi_t *blufi)
{
    ESP_LOGI(TAG, "BluFi requested wifi status");
    // 获取当前状态
    bool connected = wifi_manager_is_connected();
    // 理想情况下 wifi_manager 应该提供 get_current_ssid()，这里暂时传 NULL
    // 如果 connected=true, xn_blufi_send_connect_report 会发送 SUCCESS
    xn_blufi_send_connect_report(connected, NULL, 0);
}

// 蓝牙断开回调
static void on_ble_disconnect(xn_blufi_t *blufi)
{
    ESP_LOGI(TAG, "BluFi BLE disconnected");
    // 检查WiFi是否已连接
    if (wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "WiFi connected, exiting BluFi mode");
        // 发送配网成功事件，退出配网模式
        xn_event_post(XN_EVT_BLUFI_CONFIG_DONE, XN_EVT_SRC_BLUFI);
    } else {
        ESP_LOGI(TAG, "WiFi not connected, staying in BluFi mode (or restarting adv by component)");
    }
}

// BluFi组件回调结构体
static xn_blufi_callbacks_t s_blufi_callbacks = {
    .on_recv_sta_config = on_recv_sta_config,
    .on_connect_request = on_connect_request,
    .on_disconnect_request = on_disconnect_request,
    .on_scan_request = on_scan_request,
    .on_recv_custom_data = on_recv_custom_data,
    .on_request_wifi_status = on_request_wifi_status,
    .on_ble_disconnect = on_ble_disconnect,
};

/*===========================================================================
 *                          命令事件处理
 *===========================================================================*/

/**
 * @brief 系统事件处理回调
 * 
 * 监听 WiFi 状态事件，实现配网成功后的自动反馈和停止。
 */
static void system_event_handler(const xn_event_t *event, void *user_data)
{
    // 如果没有运行，就不处理
    if (!s_running) return;

    if (event->id == XN_EVT_WIFI_GOT_IP) {
        ESP_LOGI(TAG, "WiFi Connected (Got IP), reporting to BluFi...");
        
        // 发送报告给手机：连接成功
        // 尝试获取当前实际连接的SSID
        char ssid[33] = {0};
        if (wifi_manager_get_current_ssid(ssid) != ESP_OK) {
            // 如果获取失败（理论上不应发生，因为已Got IP），可以保持空或尝试存储
            // 这里留空，让 xn_blufi 决定（xn_blufi 已修改为允许 NULL 发送 success）
            ssid[0] = '\0';
        }

        // 发送报告给手机：连接成功
        // 带上真实的SSID
        xn_blufi_send_connect_report(true, ssid[0] ? ssid : NULL, 0);
        
        // 延时一会后停止配网，给手机端一点时间接收报告
        // 实际工程中可能需要定时器，这里简单发个事件自我停止，或者让FSM控制
        // 这里的逻辑是：配网成功 -> Report -> Stop
                 
         // NEW LOGIC: 等待蓝牙断开回调 (on_ble_disconnect) 再发送 DONE 事件
         ESP_LOGI(TAG, "Waiting for BLE disconnect to exit BluFi mode...");

    } else if (event->id == XN_EVT_WIFI_DISCONNECTED) {
        // 如果正在连接中失败
         xn_blufi_send_connect_report(false, NULL, 0);
    }
}

// 命令事件处理回调
static void cmd_event_handler(const xn_event_t *event, void *user_data)
{
    switch (event->id) {
        case XN_CMD_BLUFI_START:
            blufi_manager_start();
            break;
        case XN_CMD_BLUFI_STOP:
            blufi_manager_stop();
            break;
        default: break;
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

// 初始化管理器
esp_err_t blufi_manager_init(void)
{
    if (s_initialized) return ESP_ERR_INVALID_STATE;
    
    // 创建底层 BluFi 实例
    s_blufi_instance = xn_blufi_create(BLUFI_DEVICE_NAME);
    if (!s_blufi_instance) {
        ESP_LOGE(TAG, "Failed to create BluFi instance");
        return ESP_FAIL;
    }

    // 订阅事件
    xn_event_subscribe(XN_CMD_BLUFI_START, cmd_event_handler, NULL);
    xn_event_subscribe(XN_CMD_BLUFI_STOP, cmd_event_handler, NULL);
    xn_event_subscribe(XN_EVT_WIFI_GOT_IP, system_event_handler, NULL);
    xn_event_subscribe(XN_EVT_WIFI_DISCONNECTED, system_event_handler, NULL);
    
    s_initialized = true;
    ESP_LOGI(TAG, "BluFi Manager Initialized");
    return ESP_OK;
}

// 反初始化管理器
esp_err_t blufi_manager_deinit(void)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    
    if (s_running) {
        blufi_manager_stop();
    }
    
    xn_blufi_destroy(s_blufi_instance);
    s_blufi_instance = NULL;
    
    xn_event_unsubscribe_all(cmd_event_handler);
    xn_event_unsubscribe_all(system_event_handler);
    
    s_initialized = false;
    return ESP_OK;
}

// 启动管理器
esp_err_t blufi_manager_start(void)
{
    if (!s_initialized || !s_blufi_instance) return ESP_ERR_INVALID_STATE;
    if (s_running) {
        ESP_LOGW(TAG, "BluFi already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting BluFi...");
    
    // 初始化并启动底层组件
    esp_err_t ret = xn_blufi_init(s_blufi_instance, &s_blufi_callbacks);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start xn_blufi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_running = true;
    // 通知系统配网准备好
    xn_event_post(XN_EVT_BLUFI_INIT_DONE, XN_EVT_SRC_BLUFI);
    
    return ESP_OK;
}

// 停止管理器
esp_err_t blufi_manager_stop(void)
{
    if (!s_initialized || !s_running) return ESP_ERR_INVALID_STATE;
    
    ESP_LOGI(TAG, "Stopping BluFi...");
    // 底层deinit会停止蓝牙
    xn_blufi_deinit(s_blufi_instance);
    s_running = false;
    
    return ESP_OK;
}

// 检查是否运行接口
bool blufi_manager_is_running(void)
{
    return s_running;
}
