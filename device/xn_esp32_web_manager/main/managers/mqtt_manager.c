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
#include "web_mqtt_manager.h"
#include "mqtt_module.h"

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

/*===========================================================================
 *                          事件处理
 *===========================================================================*/

/**
 * @brief MQTT状态变更回调
 */
static void on_mqtt_state_changed(web_mqtt_state_t state)
{
    switch (state) {
        case WEB_MQTT_STATE_CONNECTED:
        case WEB_MQTT_STATE_READY:
            ESP_LOGI(TAG, "Connected to MQTT broker");
            s_connected = true;
            xn_event_post(XN_EVT_MQTT_CONNECTED, XN_EVT_SRC_MQTT);
            break;
            
        case WEB_MQTT_STATE_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            s_connected = false;
            xn_event_post(XN_EVT_MQTT_DISCONNECTED, XN_EVT_SRC_MQTT);
            break;
            
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
 * @brief MQTT消息接收回调
 */
static void on_mqtt_message(const char *topic, int topic_len, const uint8_t *payload, int payload_len)
{
    ESP_LOGD(TAG, "Received data: topic=%.*s", topic_len, topic);
    
    // 收到消息，封装后发布到事件总线
    xn_evt_mqtt_data_t data = {
        .topic = (char *)topic,
        .topic_len = topic_len,
        .data = (char *)payload,
        .data_len = payload_len,
        .msg_id = 0, // 模块层未透传msg_id
    };
    xn_event_post_data(XN_EVT_MQTT_DATA, XN_EVT_SRC_MQTT, &data, sizeof(data));
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
    // 注册消息回调
    config.message_cb = on_mqtt_message;
    
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
    
    // 停止MQTT
    mqtt_module_stop();
    
    // 重置初始化标志
    s_initialized = false;
    // 重置连接标志
    s_connected = false;
    
    // 打印反初始化完成日志
    ESP_LOGI(TAG, "MQTT manager deinitialized");
    return ESP_OK;
}

esp_err_t mqtt_manager_publish(const char *topic, const void *data, size_t len, int qos)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return mqtt_module_publish(topic, data, (int)len, qos, 0);
}

esp_err_t mqtt_manager_subscribe(const char *topic, int qos)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return mqtt_module_subscribe(topic, qos);
}

bool mqtt_manager_is_connected(void)
{
    return s_connected;
}
