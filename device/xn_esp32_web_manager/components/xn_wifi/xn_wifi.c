/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-23 12:45:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:30:02
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_wifi\xn_wifi.c
 * @Description: WiFi底层组件实现 - 封装ESP-IDF WiFi Station功能
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#include "xn_wifi.h" // 包含组件头文件
#include "esp_log.h" // 包含日志库
#include "esp_wifi.h" // 包含ESP WiFi驱动
#include "esp_event.h" // 包含事件循环库
#include "esp_netif.h" // 包含网络接口层库
#include "freertos/FreeRTOS.h" // 包含FreeRTOS核心
#include "freertos/event_groups.h" // 包含FreeRTOS事件组
#include <string.h> // 包含字符串处理库

static const char *TAG = "XN_WIFI"; // 定义日志标签

#define WIFI_CONNECTED_BIT BIT0 // 定义WiFi连接成功的事件位

// WiFi组件内部结构体定义
struct xn_wifi_s {
    EventGroupHandle_t event_group; // 事件组句柄，用于同步
    xn_wifi_status_t status; // 当前WiFi状态
    xn_wifi_scan_done_cb_t scan_callback; // 扫描完成回调
    xn_wifi_status_cb_t status_callback; // 状态变化回调
    wifi_config_t wifi_config; // WiFi配置信息
    bool is_connecting; // 是否正在连接标志
    esp_netif_t *netif; // 网络接口句柄
};

// 更新内部状态并触发回调
static void update_status(xn_wifi_t *wifi, xn_wifi_status_t new_status)
{
    if (wifi->status != new_status) { // 如果状态发生改变
        wifi->status = new_status; // 更新状态变量
        ESP_LOGI(TAG, "WiFi Status: %d -> %d", wifi->status, new_status); // 打印状态变化日志
        if (wifi->status_callback) { // 如果注册了回调函数
            wifi->status_callback(new_status); // 调用回调通知应用层
        }
    }
}

// WiFi/IP事件处理回调
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    xn_wifi_t *wifi = (xn_wifi_t *)arg; // 获取组件实例指针
    
    if (event_base == WIFI_EVENT) { // 如果是WiFi事件
        switch (event_id) { // 判断事件ID
            case WIFI_EVENT_STA_START: // Station启动事件
                ESP_LOGI(TAG, "WiFi STA Started"); // 打印日志
                break; // 结束处理
                
            case WIFI_EVENT_STA_CONNECTED: { // 连接成功事件
                wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t*)event_data; // 获取事件数据
                ESP_LOGI(TAG, "Connected to %s", event->ssid); // 打印连接的SSID
                update_status(wifi, XN_WIFI_CONNECTED); // 更新状态为已连接
                wifi->is_connecting = false; // 清除连接中标志
                break; // 结束处理
            }
            
            case WIFI_EVENT_STA_DISCONNECTED: { // 连接断开事件
                wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t*)event_data; // 获取事件数据
                ESP_LOGW(TAG, "Disconnected, reason: %d", event->reason); // 打印断开原因
                update_status(wifi, XN_WIFI_DISCONNECTED); // 更新状态为断开
                wifi->is_connecting = false; // 清除连接中标志
                xEventGroupClearBits(wifi->event_group, WIFI_CONNECTED_BIT); // 清除连接成功标志位
                break; // 结束处理
            }
            
            case WIFI_EVENT_SCAN_DONE: { // 扫描完成事件
                uint16_t ap_count = 0; // 定义AP数量变量
                esp_wifi_scan_get_ap_num(&ap_count); // 获取扫描到的AP数量
                wifi_ap_record_t *ap_list = NULL; // 定义AP列表指针
                
                if (ap_count > 0) { // 如果扫描到了AP
                    ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count); // 分配内存存储AP列表
                    if (ap_list) { // 如果内存分配成功
                        esp_wifi_scan_get_ap_records(&ap_count, ap_list); // 获取AP记录详情
                    }
                }
                
                ESP_LOGI(TAG, "Scan done, APs: %d", ap_count); // 打印扫描结果日志
                
                if (wifi->scan_callback) { // 如果注册了扫描回调
                    wifi->scan_callback(ap_count, ap_list); // 调用回调传回结果
                }
                
                if (ap_list) free(ap_list); // 释放内存
                break; // 结束处理
            }
        }
    } else if (event_base == IP_EVENT) { // 如果是IP事件
        if (event_id == IP_EVENT_STA_GOT_IP) { // 获取IP事件
            ip_event_got_ip_t *event = (ip_event_got_ip_t*)event_data; // 获取IP事件数据
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip)); // 打印获得的IP地址
            xEventGroupSetBits(wifi->event_group, WIFI_CONNECTED_BIT); // 设置事件位表示网络就绪
            update_status(wifi, XN_WIFI_GOT_IP); // 更新状态为获取IP
        }
    }
}

// 创建WiFi实例
xn_wifi_t* xn_wifi_create(void)
{
    xn_wifi_t *wifi = malloc(sizeof(xn_wifi_t)); // 分配结构体内存
    if (wifi == NULL) return NULL; // 如果分配失败返回NULL
    memset(wifi, 0, sizeof(xn_wifi_t)); // 清零内存
    return wifi; // 返回实例指针
}

// 销毁WiFi实例
void xn_wifi_destroy(xn_wifi_t *wifi)
{
    if (wifi) { // 如果指针有效
        if (wifi->event_group) vEventGroupDelete(wifi->event_group); // 删除事件组
        free(wifi); // 释放内存
    }
}

// 初始化WiFi组件
esp_err_t xn_wifi_init(xn_wifi_t *wifi)
{
    if (wifi == NULL) return ESP_ERR_INVALID_ARG; // 参数检查
    
    wifi->event_group = xEventGroupCreate(); // 创建事件组
    
    // 初始化Netif (如果已初始化会返回错误，但通常在app_main里只init一次，这里为了鲁棒性可以检查)
    esp_err_t ret = esp_netif_init(); // 初始化TCP/IP堆栈
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret; // 检查返回值

    ret = esp_event_loop_create_default(); // 创建默认事件循环
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret; // 检查返回值

    wifi->netif = esp_netif_create_default_wifi_sta(); // 创建默认Station网卡
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, wifi)); // 注册WiFi事件处理
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, wifi)); // 注册IP事件处理
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // 获取默认WiFi配置
    ret = esp_wifi_init(&cfg); // 初始化WiFi驱动
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret; // 检查返回值

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // 设置模式为Station
    ESP_ERROR_CHECK(esp_wifi_start()); // 启动WiFi驱动
    
    return ESP_OK; // 返回成功
}

// 反初始化WiFi组件
esp_err_t xn_wifi_deinit(xn_wifi_t *wifi)
{
    if (wifi == NULL) return ESP_ERR_INVALID_ARG; // 参数检查
    
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler); // 注销WiFi事件处理
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler); // 注销IP事件处理
    
    esp_wifi_stop(); // 停止WiFi驱动
    // esp_wifi_deinit(); // 慎重反初始化，通常系统生命周期内不反初始化
    
    return ESP_OK; // 返回成功
}

// 连接WiFi
esp_err_t xn_wifi_connect(xn_wifi_t *wifi, const char *ssid, const char *password)
{
    if (wifi == NULL || ssid == NULL) return ESP_ERR_INVALID_ARG; // 参数检查
    
    memset(&wifi->wifi_config, 0, sizeof(wifi_config_t)); // 清空配置结构体
    strlcpy((char*)wifi->wifi_config.sta.ssid, ssid, sizeof(wifi->wifi_config.sta.ssid)); // 拷贝SSID
    if (password) { // 如果有密码
        strlcpy((char*)wifi->wifi_config.sta.password, password, sizeof(wifi->wifi_config.sta.password)); // 拷贝密码
    }
    
    esp_wifi_disconnect(); // 先断开当前连接
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi->wifi_config)); // 设置WiFi连接配置
    
    wifi->is_connecting = true; // 设置连接标志
    update_status(wifi, XN_WIFI_CONNECTING); // 更新状态为连接中
    
    return esp_wifi_connect(); // 发起连接
}

// 断开WiFi
esp_err_t xn_wifi_disconnect(xn_wifi_t *wifi)
{
    if (wifi == NULL) return ESP_ERR_INVALID_ARG; // 参数检查
    wifi->is_connecting = false; // 清除连接标志
    return esp_wifi_disconnect(); // 发起断开
}

// 扫描WiFi
esp_err_t xn_wifi_scan(xn_wifi_t *wifi, xn_wifi_scan_done_cb_t callback)
{
    if (wifi == NULL) return ESP_ERR_INVALID_ARG; // 参数检查
    wifi->scan_callback = callback; // 保存回调函数
    
    wifi_scan_config_t scan_config = { // 配置扫描参数
        .show_hidden = false, // 不显示隐藏SSID
        .scan_type = WIFI_SCAN_TYPE_ACTIVE, // 主动扫描
        .scan_time.active.min = 0, // 默认时间
        .scan_time.active.max = 0 // 默认时间
    };
    
    return esp_wifi_scan_start(&scan_config, false); // 启动扫描
}

// 获取当前状态
xn_wifi_status_t xn_wifi_get_status(xn_wifi_t *wifi)
{
    return wifi ? wifi->status : XN_WIFI_DISCONNECTED; // 返回当前状态或断开
}

// 注册状态回调
void xn_wifi_register_status_cb(xn_wifi_t *wifi, xn_wifi_status_cb_t callback)
{
    if (wifi) wifi->status_callback = callback; // 保存状态回调
}

// 获取当前连接的SSID
esp_err_t xn_wifi_get_current_ssid(xn_wifi_t *wifi, char *ssid)
{
    if (wifi == NULL || ssid == NULL) return ESP_ERR_INVALID_ARG;
    
    // 如果没有IP或未连接，返回错误
    if (wifi->status != XN_WIFI_GOT_IP && wifi->status != XN_WIFI_CONNECTED) {
        return ESP_ERR_INVALID_STATE;
    }

    wifi_config_t conf;
    esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &conf);
    if (ret == ESP_OK) {
        // 直接从驱动配置中读取
        strncpy(ssid, (char*)conf.sta.ssid, 32);
        ssid[32] = '\0';
    }
    return ret;
}
