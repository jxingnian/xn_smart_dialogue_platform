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
        case WEB_MQTT_STATE_ERROR:
            // 标记已断开
            s_connected = false;
            // 发布MQTT错误事件
            xn_event_post(XN_EVT_MQTT_ERROR, XN_EVT_SRC_MQTT);
            break;
            
        default:
            break;
    }
}

/**
 * @brief 系统事件处理回调
 * 
 * 监听 WiFi 状态事件，实现自动连接逻辑。
 */
static void system_event_handler(const xn_event_t *event, void *user_data)
{
    // 根据事件ID处理
    switch (event->id) {
        case XN_EVT_WIFI_GOT_IP:
            // 打印日志
            ESP_LOGI(TAG, "WiFi Got IP, starting MQTT...");
            // WiFi连接成功，自动启动MQTT连接（底层组件已配置自动重连，这里确保它处于运行状态）
            break;
            
        case XN_EVT_WIFI_DISCONNECTED:
            // 打印日志
            ESP_LOGW(TAG, "WiFi Disconnected, MQTT will pause...");
            // WiFi断开，底层MQTT会自动检测到网络错误并进入重连等待，
            // 也可以选择显式暂停MQTT以节省资源
            s_connected = false;
            break;
            
        default:
            break;
    }
}


/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t mqtt_manager_init(void)
{
    // 检查是否已初始化
    if (s_initialized) {
        // 已初始化则返回状态错误
        return ESP_ERR_INVALID_STATE;
    }
    
    // 初始化配置，使用默认配置
    web_mqtt_manager_config_t config = WEB_MQTT_MANAGER_DEFAULT_CONFIG();
    
    // 设置MQTT Broker地址 (建议改为从NVS读取)
    config.broker_uri = "mqtt://broker.emqx.io:1883"; 
    // 设置项目基础Topic
    config.base_topic = "xn/device";
    // 注册状态回调
    config.event_cb = on_mqtt_state_changed;
    
    // 初始化Web MQTT组件
    esp_err_t ret = web_mqtt_manager_init(&config);
    // 检查初始化结果
    if (ret != ESP_OK) {
        // 打印错误日志
        ESP_LOGE(TAG, "Failed to init Web MQTT: %s", esp_err_to_name(ret));
        // 返回错误码
        return ret;
    }
    
    // 订阅系统事件 - 关注WiFi状态
    xn_event_subscribe(XN_EVT_WIFI_GOT_IP, system_event_handler, NULL);
    xn_event_subscribe(XN_EVT_WIFI_DISCONNECTED, system_event_handler, NULL);
    
    // 标记初始化完成
    s_initialized = true;
    // 打印初始化成功日志
    ESP_LOGI(TAG, "MQTT manager initialized");
    
    // 返回成功
    return ESP_OK;
}

esp_err_t mqtt_manager_deinit(void)
{
    // 检查是否已初始化
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 取消订阅系统事件
    xn_event_unsubscribe(XN_EVT_WIFI_GOT_IP, system_event_handler);
    xn_event_unsubscribe(XN_EVT_WIFI_DISCONNECTED, system_event_handler);
    
    // 注意：Web MQTT 组件暂时没有提供反初始化函数，这里仅标记
    
    // 重置初始化标志
    s_initialized = false;
    // 重置连接标志
    s_connected = false;
    
    // 打印反初始化完成日志
    ESP_LOGI(TAG, "MQTT manager deinitialized");
    
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
