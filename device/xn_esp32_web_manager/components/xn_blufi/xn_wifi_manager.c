/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_blufi\xn_wifi_manager.c
 * @Description: WiFi管理层 - 封装ESP-IDF WiFi API
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include "xn_wifi_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "XN_WIFI_MANAGER";

// WiFi连接成功标志位
#define WIFI_CONNECTED_BIT BIT0
// 最大重连次数
#define MAX_RETRY_COUNT 5

/* WiFi管理器实例结构体 */
struct xn_wifi_manager_s {
    EventGroupHandle_t event_group;         // WiFi事件组，用于等待连接成功等事件
    xn_wifi_status_t status;                // WiFi连接状态，记录当前所处阶段
    xn_wifi_scan_done_cb_t scan_callback;   // 扫描完成回调函数指针
    xn_wifi_status_cb_t status_callback;    // 状态变化回调函数指针
    wifi_config_t wifi_config;              // WiFi配置信息（SSID和密码）
    uint8_t retry_count;                    // 当前已尝试重连的次数
    bool is_connecting;                     // 标志位：是否正在进行连接过程
    esp_netif_t *netif;                     // ESP-NETIF 网络接口实例指针
};

/* 更新WiFi状态并触发回调 */
static void update_status(xn_wifi_manager_t *manager, xn_wifi_status_t new_status)
{
    // 检查状态是否真的发生改变，避免重复触发
    if (manager->status != new_status) {
        manager->status = new_status; // 更新内部状态
        ESP_LOGI(TAG, "WiFi状态变化: %d", new_status); // 打印日志
        
        // 如果注册了状态回调函数，则通知上层应用
        if (manager->status_callback) {
            manager->status_callback(new_status);
        }
    }
}

/* WiFi事件处理函数 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    // 获取传递的用户参数，这里是WiFi管理器实例
    xn_wifi_manager_t *manager = (xn_wifi_manager_t *)arg;
    
    // 处理WiFi相关事件
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                // WiFi Station模式启动完成
                ESP_LOGI(TAG, "WiFi已启动");
                break;
                
            case WIFI_EVENT_STA_CONNECTED: {
                // 已成功连接到AP
                wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t*)event_data;
                ESP_LOGI(TAG, "已连接到WiFi: %s", event->ssid);
                
                // 更新状态为已连接（但还未获取IP）
                update_status(manager, XN_WIFI_CONNECTED);
                
                // 连接成功，不再处于连接尝试中
                manager->is_connecting = false;
                // 重置重试计数器
                manager->retry_count = 0;
                break;
            }
            
            case WIFI_EVENT_STA_DISCONNECTED: {
                // WiFi连接断开
                wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t*)event_data;
                ESP_LOGW(TAG, "WiFi断开，原因: %d", event->reason);
                
                // 如果当前处于连接过程中且未超过最大重试次数，则尝试重连
                if (manager->is_connecting && manager->retry_count < MAX_RETRY_COUNT) {
                    esp_wifi_connect(); // 发起重连
                    manager->retry_count++; // 增加重试计数
                    ESP_LOGI(TAG, "重连WiFi，第%d次", manager->retry_count);
                    update_status(manager, XN_WIFI_CONNECTING); // 状态保持为连接中
                } else {
                    // 超过重试次数或非预期断开，停止自动连接
                    manager->is_connecting = false;
                    update_status(manager, XN_WIFI_DISCONNECTED); // 更新状态为断开
                }
                
                // 清除事件组中的连接标志位
                xEventGroupClearBits(manager->event_group, WIFI_CONNECTED_BIT);
                break;
            }
            
            case WIFI_EVENT_SCAN_DONE: {
                // WiFi扫描完成
                uint16_t ap_count = 0;
                esp_wifi_scan_get_ap_num(&ap_count); // 获取扫描到的AP数量
                
                if (ap_count == 0) {
                    ESP_LOGW(TAG, "未扫描到WiFi");
                    // 如果没有扫描到，回调返回空列表
                    if (manager->scan_callback) {
                        manager->scan_callback(0, NULL);
                    }
                    break;
                }
                
                // 分配内存存储AP列表
                wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
                if (ap_list == NULL) {
                    ESP_LOGE(TAG, "分配内存失败");
                    break;
                }
                
                // 获取具体的AP信息
                esp_wifi_scan_get_ap_records(&ap_count, ap_list);
                ESP_LOGI(TAG, "扫描到%d个WiFi", ap_count);
                
                // 通过回调将结果传递给上层
                if (manager->scan_callback) {
                    manager->scan_callback(ap_count, ap_list);
                }
                
                // 释放临时申请的内存
                free(ap_list);
                break;
            }
        }
    } else if (event_base == IP_EVENT) {
        // 处理IP相关事件
        if (event_id == IP_EVENT_STA_GOT_IP) {
            // DHCP成功，获取到IP地址
            ip_event_got_ip_t *event = (ip_event_got_ip_t*)event_data;
            ESP_LOGI(TAG, "获取到IP: " IPSTR, IP2STR(&event->ip_info.ip));
            
            // 设置事件组标志位，表示完全连接成功
            xEventGroupSetBits(manager->event_group, WIFI_CONNECTED_BIT);
            // 更新状态为已获取IP
            update_status(manager, XN_WIFI_GOT_IP);
        }
    }
}

/* 创建WiFi管理器实例 */
xn_wifi_manager_t* xn_wifi_manager_create(void)
{
    // 分配结构体内存
    xn_wifi_manager_t *manager = malloc(sizeof(xn_wifi_manager_t));
    if (manager == NULL) {
        ESP_LOGE(TAG, "分配内存失败");
        return NULL;
    }
    
    // 清零内存并设置初始状态
    memset(manager, 0, sizeof(xn_wifi_manager_t));
    manager->status = XN_WIFI_DISCONNECTED;
    
    ESP_LOGI(TAG, "WiFi管理器创建成功");
    return manager;
}

/* 销毁WiFi管理器实例 */
void xn_wifi_manager_destroy(xn_wifi_manager_t *manager)
{
    if (manager) {
        // 删除事件组
        if (manager->event_group) {
            vEventGroupDelete(manager->event_group);
        }
        // 释放结构体内存
        free(manager);
        ESP_LOGI(TAG, "WiFi管理器已销毁");
    }
}

/* 初始化WiFi管理器 */
esp_err_t xn_wifi_manager_init(xn_wifi_manager_t *manager)
{
    if (manager == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 创建事件组
    manager->event_group = xEventGroupCreate();
    if (manager->event_group == NULL) {
        ESP_LOGE(TAG, "创建事件组失败");
        return ESP_FAIL;
    }
    
    // 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 创建默认的WiFi Station网络接口
    manager->netif = esp_netif_create_default_wifi_sta();
    
    // 注册WiFi和IP事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                               &wifi_event_handler, manager));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, manager));
    
    // 初始化WiFi配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // 设置为Station模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // 启动WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi管理器初始化成功");
    return ESP_OK;
}

/* 反初始化WiFi管理器 */
esp_err_t xn_wifi_manager_deinit(xn_wifi_manager_t *manager)
{
    if (manager == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 停止WiFi
    esp_wifi_stop();
    // 反初始化WiFi驱动
    esp_wifi_deinit();
    
    // 注销事件处理函数
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
    
    ESP_LOGI(TAG, "WiFi管理器已反初始化");
    return ESP_OK;
}

/* 连接WiFi */
esp_err_t xn_wifi_manager_connect(xn_wifi_manager_t *manager, 
                                   const char *ssid, 
                                   const char *password)
{
    if (manager == NULL || ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 清空并设置新的WiFi配置
    memset(&manager->wifi_config, 0, sizeof(wifi_config_t));
    // 复制SSID
    strncpy((char*)manager->wifi_config.sta.ssid, ssid, 
           sizeof(manager->wifi_config.sta.ssid) - 1);
    // 如果有密码则复制
    if (password) {
        strncpy((char*)manager->wifi_config.sta.password, password, 
               sizeof(manager->wifi_config.sta.password) - 1);
    }
    
    // 先断开当前可能的连接
    esp_wifi_disconnect();
    
    // 设置新配置到ESP-IDF WiFi驱动
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &manager->wifi_config));
    
    // 更新内部状态
    manager->is_connecting = true;
    manager->retry_count = 0;
    update_status(manager, XN_WIFI_CONNECTING);
    
    ESP_LOGI(TAG, "开始连接WiFi: %s", ssid);
    // 调用ESP-IDF接口开始连接
    return esp_wifi_connect();
}

/* 断开WiFi */
esp_err_t xn_wifi_manager_disconnect(xn_wifi_manager_t *manager)
{
    if (manager == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 标记不再处于主动连接状态
    manager->is_connecting = false;
    ESP_LOGI(TAG, "断开WiFi连接");
    // 调用ESP-IDF接口断开连接
    return esp_wifi_disconnect();
}

/* 扫描WiFi */
esp_err_t xn_wifi_manager_scan(xn_wifi_manager_t *manager, 
                                xn_wifi_scan_done_cb_t callback)
{
    if (manager == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 保存回调函数
    manager->scan_callback = callback;
    
    // 配置扫描参数，默认扫描所有通道
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false
    };
    
    ESP_LOGI(TAG, "开始扫描WiFi");
    // 启动非阻塞扫描
    return esp_wifi_scan_start(&scan_config, false);
}

/* 获取WiFi状态 */
xn_wifi_status_t xn_wifi_manager_get_status(xn_wifi_manager_t *manager)
{
    if (manager == NULL) {
        return XN_WIFI_DISCONNECTED;
    }
    return manager->status;
}

/* 注册状态回调 */
void xn_wifi_manager_register_status_cb(xn_wifi_manager_t *manager, 
                                         xn_wifi_status_cb_t callback)
{
    if (manager) {
        manager->status_callback = callback;
    }
}

