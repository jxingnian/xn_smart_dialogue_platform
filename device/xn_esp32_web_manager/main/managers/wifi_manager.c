/**
 * @file wifi_manager.c
 * @brief WiFi应用管理器实现
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

static bool s_initialized = false;
static bool s_connected = false;
static uint32_t s_ip_addr = 0;
static EventGroupHandle_t s_wifi_event_group = NULL;

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

/*===========================================================================
 *                          事件处理
 *===========================================================================*/

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA started");
                xn_event_post(XN_EVT_WIFI_STA_START, XN_EVT_SRC_WIFI);
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(TAG, "STA stopped");
                xn_event_post(XN_EVT_WIFI_STA_STOP, XN_EVT_SRC_WIFI);
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connected to AP");
                s_connected = true;
                xn_event_post(XN_EVT_WIFI_CONNECTED, XN_EVT_SRC_WIFI);
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *evt = (wifi_event_sta_disconnected_t *)event_data;
                ESP_LOGW(TAG, "Disconnected from AP, reason: %d", evt->reason);
                s_connected = false;
                s_ip_addr = 0;
                
                xn_evt_wifi_disconnected_t data = {.reason = evt->reason};
                xn_event_post_data(XN_EVT_WIFI_DISCONNECTED, XN_EVT_SRC_WIFI, 
                                   &data, sizeof(data));
                
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                
                // 自动重连
                esp_wifi_connect();
                break;
            }
            
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            s_ip_addr = event->ip_info.ip.addr;
            
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            
            xn_evt_wifi_got_ip_t data = {
                .ip = event->ip_info.ip.addr,
                .netmask = event->ip_info.netmask.addr,
                .gateway = event->ip_info.gw.addr,
            };
            xn_event_post_data(XN_EVT_WIFI_GOT_IP, XN_EVT_SRC_WIFI, 
                               &data, sizeof(data));
            
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        } else if (event_id == IP_EVENT_STA_LOST_IP) {
            ESP_LOGW(TAG, "Lost IP");
            s_ip_addr = 0;
            xn_event_post(XN_EVT_WIFI_LOST_IP, XN_EVT_SRC_WIFI);
        }
    }
}

/*===========================================================================
 *                          命令事件处理
 *===========================================================================*/

static void cmd_event_handler(const xn_event_t *event, void *user_data)
{
    switch (event->id) {
        case XN_CMD_WIFI_CONNECT:
            ESP_LOGI(TAG, "Received WIFI_CONNECT command");
            wifi_manager_start();
            break;
            
        case XN_CMD_WIFI_DISCONNECT:
            ESP_LOGI(TAG, "Received WIFI_DISCONNECT command");
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
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    // 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();
    
    // 初始化WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 注册事件处理
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                                wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                                wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, 
                                                wifi_event_handler, NULL));
    
    // 订阅命令事件
    xn_event_subscribe(XN_CMD_WIFI_CONNECT, cmd_event_handler, NULL);
    xn_event_subscribe(XN_CMD_WIFI_DISCONNECT, cmd_event_handler, NULL);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    s_initialized = true;
    ESP_LOGI(TAG, "WiFi manager initialized");
    
    return ESP_OK;
}

esp_err_t wifi_manager_deinit(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xn_event_unsubscribe(XN_CMD_WIFI_CONNECT, cmd_event_handler);
    xn_event_unsubscribe(XN_CMD_WIFI_DISCONNECT, cmd_event_handler);
    
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_LOST_IP, wifi_event_handler);
    
    esp_wifi_stop();
    esp_wifi_deinit();
    
    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    s_initialized = false;
    s_connected = false;
    s_ip_addr = 0;
    
    ESP_LOGI(TAG, "WiFi manager deinitialized");
    
    return ESP_OK;
}

esp_err_t wifi_manager_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_wifi_start();
}

esp_err_t wifi_manager_stop(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_wifi_stop();
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (password) {
        strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    return esp_wifi_start();
}

esp_err_t wifi_manager_disconnect(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_wifi_disconnect();
}

bool wifi_manager_is_connected(void)
{
    return s_connected && (s_ip_addr != 0);
}

uint32_t wifi_manager_get_ip(void)
{
    return s_ip_addr;
}
