/**
 * @file mqtt_manager.c
 * @brief MQTT应用管理器实现
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "xn_event_bus.h"
#include "mqtt_manager.h"

static const char *TAG = "mqtt_manager";

/*===========================================================================
 *                          配置
 *===========================================================================*/

#ifndef MQTT_BROKER_URI
// 默认使用的 MQTT Broker (公共测试服务器)
#define MQTT_BROKER_URI     "mqtt://broker.emqx.io:1883"
#endif

/*===========================================================================
 *                          内部变量
 *===========================================================================*/

static bool s_initialized = false;              ///< 初始化标志
static bool s_connected = false;                ///< 连接状态标志
static esp_mqtt_client_handle_t s_client = NULL; ///< MQTT客户端句柄

/*===========================================================================
 *                          事件处理
 *===========================================================================*/

/**
 * @brief ESP-IDF MQTT底层事件回调
 * 
 * 处理底层 MQTT Client 的各种事件（连接、断开、数据到达等），
 * 并将其封装为 XN_EVENT 广播到事件总线。
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "Connecting to MQTT broker...");
            xn_event_post(XN_EVT_MQTT_CONNECTING, XN_EVT_SRC_MQTT);
            break;
            
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to MQTT broker");
            s_connected = true;
            xn_event_post(XN_EVT_MQTT_CONNECTED, XN_EVT_SRC_MQTT);
            
            // 连接成功后，可以根据需要自动订阅一些主题
            // mqtt_manager_subscribe("device/+/command", 1);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            s_connected = false;
            xn_event_post(XN_EVT_MQTT_DISCONNECTED, XN_EVT_SRC_MQTT);
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed, msg_id=%d", event->msg_id);
            xn_event_post(XN_EVT_MQTT_SUBSCRIBED, XN_EVT_SRC_MQTT);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "Published, msg_id=%d", event->msg_id);
            xn_event_post(XN_EVT_MQTT_PUBLISHED, XN_EVT_SRC_MQTT);
            break;
            
        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "Received data: topic=%.*s", event->topic_len, event->topic);
            
            // 收到消息，封装后发布到事件总线
            // 注意：event->data 不一定是 NULL 结尾的字符串，需要结合 data_len 使用
            xn_evt_mqtt_data_t data = {
                .topic = event->topic,
                .topic_len = event->topic_len,
                .data = event->data,
                .data_len = event->data_len,
                .msg_id = event->msg_id,
            };
            xn_event_post_data(XN_EVT_MQTT_DATA, XN_EVT_SRC_MQTT, &data, sizeof(data));
            break;
        }
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno));
            }
            xn_event_post(XN_EVT_MQTT_ERROR, XN_EVT_SRC_MQTT);
            break;
            
        default:
            ESP_LOGD(TAG, "Unhandled event id: %d", (int)event_id);
            break;
    }
}

/*===========================================================================
 *                          命令事件处理
 *===========================================================================*/

/**
 * @brief XN事件总线命令回调
 */
static void cmd_event_handler(const xn_event_t *event, void *user_data)
{
    switch (event->id) {
        case XN_CMD_MQTT_CONNECT:
            ESP_LOGI(TAG, "Received MQTT_CONNECT command");
            mqtt_manager_connect();
            break;
            
        case XN_CMD_MQTT_DISCONNECT:
            ESP_LOGI(TAG, "Received MQTT_DISCONNECT command");
            mqtt_manager_disconnect();
            break;
            
        default:
            break;
    }
}

/**
 * @brief WiFi事件回调
 * 
 * 监听 WiFi 获取IP事件，以实现自动连接 MQTT。
 */
static void wifi_event_handler(const xn_event_t *event, void *user_data)
{
    if (event->id == XN_EVT_WIFI_GOT_IP) {
        ESP_LOGI(TAG, "WiFi connected, starting MQTT...");
        // WiFi就绪，启动MQTT连接
        mqtt_manager_connect();
    } else if (event->id == XN_EVT_WIFI_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected");
        // WiFi断开，MQTT连接也会由于网络中断而断开
        // 此处可选：主动 stop client
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t mqtt_manager_init(void)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 配置MQTT客户端
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        // .session.keepalive = 60,
    };
    
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "Failed to init MQTT client");
        return ESP_FAIL;
    }
    
    // 注册底层MQTT事件
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, 
                                   mqtt_event_handler, NULL);
    
    // 订阅内部命令
    xn_event_subscribe(XN_CMD_MQTT_CONNECT, cmd_event_handler, NULL);
    xn_event_subscribe(XN_CMD_MQTT_DISCONNECT, cmd_event_handler, NULL);
    
    // 订阅WiFi事件(用于自动连接)
    xn_event_subscribe(XN_EVT_WIFI_GOT_IP, wifi_event_handler, NULL);
    xn_event_subscribe(XN_EVT_WIFI_DISCONNECTED, wifi_event_handler, NULL);
    
    s_initialized = true;
    ESP_LOGI(TAG, "MQTT manager initialized");
    
    return ESP_OK;
}

esp_err_t mqtt_manager_deinit(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xn_event_unsubscribe_all(cmd_event_handler);
    xn_event_unsubscribe_all(wifi_event_handler);
    
    if (s_client) {
        esp_mqtt_client_stop(s_client);
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
    }
    
    s_initialized = false;
    s_connected = false;
    
    ESP_LOGI(TAG, "MQTT manager deinitialized");
    
    return ESP_OK;
}

esp_err_t mqtt_manager_connect(void)
{
    if (!s_initialized || s_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_mqtt_client_start(s_client);
}

esp_err_t mqtt_manager_disconnect(void)
{
    if (!s_initialized || s_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return esp_mqtt_client_stop(s_client);
}

esp_err_t mqtt_manager_publish(const char *topic, const char *data, int qos)
{
    if (!s_initialized || s_client == NULL || !s_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (topic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_publish(s_client, topic, data, 
                                          data ? strlen(data) : 0, qos, 0);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t mqtt_manager_subscribe(const char *topic, int qos)
{
    if (!s_initialized || s_client == NULL || !s_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (topic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_subscribe(s_client, topic, qos);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

bool mqtt_manager_is_connected(void)
{
    return s_connected;
}
