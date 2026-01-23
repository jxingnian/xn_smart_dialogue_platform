/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:27:33
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\wifi_manager.c
 * @Description: WiFi应用管理器实现 - 封装ESP-IDF WiFi Station功能
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "xn_event_bus.h"
#include "wifi_manager.h"

static const char *TAG = "wifi_manager";

/*===========================================================================
 *                          内部变量
 *===========================================================================*/

static bool s_initialized = false;          ///< 初始化标志
static bool s_connected = false;            ///< 物理层连接状态
static uint32_t s_ip_addr = 0;              ///< 当前IP地址(0表示未获取)
static EventGroupHandle_t s_wifi_event_group = NULL; ///< WiFi事件组(用于同步等待)

#define WIFI_CONNECTED_BIT  BIT0            ///< 事件位：已连接并获取IP
#define WIFI_FAIL_BIT       BIT1            ///< 事件位：连接失败

/*===========================================================================
 *                          事件处理
 *===========================================================================*/

/**
 * @brief ESP-IDF 系统事件回调
 * 
 * 处理 WIFI_EVENT 和 IP_EVENT，并将其转换为 XN_EVENT 广播出去。
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    // 处理WiFi底层事件
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                // 打印STA启动日志
                ESP_LOGI(TAG, "STA started");
                // 发布STA启动事件
                xn_event_post(XN_EVT_WIFI_STA_START, XN_EVT_SRC_WIFI);
                // 启动后立即尝试连接（使用保存的配置）
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_STA_STOP:
                // 打印STA停止日志
                ESP_LOGI(TAG, "STA stopped");
                // 发布STA停止事件
                xn_event_post(XN_EVT_WIFI_STA_STOP, XN_EVT_SRC_WIFI);
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                // 打印连接成功日志
                ESP_LOGI(TAG, "Connected to AP");
                // 标记已连接状态
                s_connected = true;
                // 发布WiFi连接成功(物理层)事件
                xn_event_post(XN_EVT_WIFI_CONNECTED, XN_EVT_SRC_WIFI);
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED: {
                // 获取断开原因
                wifi_event_sta_disconnected_t *evt = (wifi_event_sta_disconnected_t *)event_data;
                // 打印断开警告日志
                ESP_LOGW(TAG, "Disconnected from AP, reason: %d", evt->reason);
                // 标记断开状态
                s_connected = false;
                // 清除IP地址
                s_ip_addr = 0;
                
                // 准备断开事件数据
                xn_evt_wifi_disconnected_t data = {.reason = evt->reason};
                // 发布WiFi断开事件
                xn_event_post_data(XN_EVT_WIFI_DISCONNECTED, XN_EVT_SRC_WIFI, 
                                   &data, sizeof(data));
                
                // 设置事件组失败位
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                
                // 收到断开事件后，自动尝试重连
                // 注意：在实际产品中可能需要退避策略，避免频繁重连
                esp_wifi_connect();
                break;
            }
            
            default:
                break;
        }
    } else if (event_base == IP_EVENT) { // 处理IP层事件
        if (event_id == IP_EVENT_STA_GOT_IP) {
            // 获取IP事件数据
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            // 保存IP地址
            s_ip_addr = event->ip_info.ip.addr;
            
            // 打印获取IP日志
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            
            // 准备获取IP事件数据
            xn_evt_wifi_got_ip_t data = {
                .ip = event->ip_info.ip.addr,
                .netmask = event->ip_info.netmask.addr,
                .gateway = event->ip_info.gw.addr,
            };
            // 发布获取IP事件
            xn_event_post_data(XN_EVT_WIFI_GOT_IP, XN_EVT_SRC_WIFI, 
                               &data, sizeof(data));
            
            // 设置事件组连接成功位
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        } else if (event_id == IP_EVENT_STA_LOST_IP) {
            // 打印丢失IP警告
            ESP_LOGW(TAG, "Lost IP");
            // 清除IP地址
            s_ip_addr = 0;
            // 发布丢失IP事件
            xn_event_post(XN_EVT_WIFI_LOST_IP, XN_EVT_SRC_WIFI);
        }
    }
}

/*===========================================================================
 *                          命令事件处理
 *===========================================================================*/

/**
 * @brief XN事件总线命令回调
 * 
 * 响应来自其他模块的控制命令（如CLI、BlueFi等发出的连接请求）。
 */
static void cmd_event_handler(const xn_event_t *event, void *user_data)
{
    switch (event->id) {
        case XN_CMD_WIFI_CONNECT:
            // 打印收到连接命令日志
            ESP_LOGI(TAG, "Received WIFI_CONNECT command");
            // 执行WiFi启动连接
            wifi_manager_start();
            break;
            
        case XN_CMD_WIFI_DISCONNECT:
            // 打印收到断开命令日志
            ESP_LOGI(TAG, "Received WIFI_DISCONNECT command");
            // 执行WiFi断开
            wifi_manager_disconnect();
            break;
            
        default:
            break;
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t wifi_manager_init(void)
{
    // 检查是否已初始化
    if (s_initialized) {
        // 已初始化则返回状态错误
        return ESP_ERR_INVALID_STATE;
    }
    
    // 创建事件组
    s_wifi_event_group = xEventGroupCreate();
    // 检查事件组创建是否成功
    if (s_wifi_event_group == NULL) {
        // 内存不足
        return ESP_ERR_NO_MEM;
    }
    
    // 初始化网络接口层(Netif)
    ESP_ERROR_CHECK(esp_netif_init());
    // 创建默认的WiFi Station网络接口
    esp_netif_create_default_wifi_sta();
    
    // 初始化WiFi驱动，使用默认配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 注册系统事件处理回调 - 监听所有WiFi事件
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                                wifi_event_handler, NULL));
    // 注册系统事件处理回调 - 监听获取IP事件
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                                wifi_event_handler, NULL));
    // 注册系统事件处理回调 - 监听丢失IP事件
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, 
                                                wifi_event_handler, NULL));
    
    // 订阅内部命令事件 - 连接WiFi
    xn_event_subscribe(XN_CMD_WIFI_CONNECT, cmd_event_handler, NULL);
    // 订阅内部命令事件 - 断开WiFi
    xn_event_subscribe(XN_CMD_WIFI_DISCONNECT, cmd_event_handler, NULL);
    
    // 设置为STA模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // 标记初始化完成
    s_initialized = true;
    // 打印初始化成功日志
    ESP_LOGI(TAG, "WiFi manager initialized");
    
    // 返回成功
    return ESP_OK;
}

esp_err_t wifi_manager_deinit(void)
{
    // 检查是否已初始化
    if (!s_initialized) {
        // 未初始化则返回状态错误
        return ESP_ERR_INVALID_STATE;
    }
    
    // 取消订阅命令事件
    xn_event_unsubscribe(XN_CMD_WIFI_CONNECT, cmd_event_handler);
    xn_event_unsubscribe(XN_CMD_WIFI_DISCONNECT, cmd_event_handler);
    
    // 注销系统事件
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_LOST_IP, wifi_event_handler);
    
    // 停止WiFi
    esp_wifi_stop();
    // 反初始化WiFi驱动
    esp_wifi_deinit();
    
    // 删除事件组
    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    // 重置状态
    s_initialized = false;
    s_connected = false;
    s_ip_addr = 0;
    
    // 打印反初始化完成日志
    ESP_LOGI(TAG, "WiFi manager deinitialized");
    
    // 返回成功
    return ESP_OK;
}

esp_err_t wifi_manager_start(void)
{
    // 检查是否已初始化
    if (!s_initialized) {
        // 未初始化则返回状态错误
        return ESP_ERR_INVALID_STATE;
    }
    
    // 启动WiFi，底层驱动会自动加载NVS中的配置并尝试自动连接（如果在STA_START回调中调用了connect）
    return esp_wifi_start();
}

esp_err_t wifi_manager_stop(void)
{
    // 检查是否已初始化
    if (!s_initialized) {
        // 未初始化则返回状态错误
        return ESP_ERR_INVALID_STATE;
    }
    
    // 停止WiFi
    return esp_wifi_stop();
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
    // 检查是否已初始化
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 检查参数有效性
    if (ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 准备WiFi配置结构体
    wifi_config_t wifi_config = {0};
    // 复制SSID
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    // 如果有密码，则复制密码
    if (password) {
        strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }
    
    // 设置WiFi配置，这会自动保存到NVS
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // 确保WiFi已启动
    return esp_wifi_start();
}

esp_err_t wifi_manager_disconnect(void)
{
    // 检查是否已初始化
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 执行断开连接
    return esp_wifi_disconnect();
}

bool wifi_manager_is_connected(void)
{
    // 返回是否物理连接且已获取IP
    return s_connected && (s_ip_addr != 0);
}

uint32_t wifi_manager_get_ip(void)
{
    // 返回当前IP地址
    return s_ip_addr;
}

