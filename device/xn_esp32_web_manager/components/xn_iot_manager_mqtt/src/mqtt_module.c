/*
 * @Author: 星年 jixingnian@gmail.com
 * @Date: 2025-01-15
 * @Description: MQTT 模块实现（封装 esp-mqtt 客户端）
 * 
 * 仅负责：
 *  - 根据配置创建 MQTT 客户端；
 *  - 启动/停止客户端；
 *  - 将底层事件转换为简单的 mqtt_module_event_t 上报给上层。
 * 
 * 不直接处理业务 Topic，由上层管理器决定订阅/发布策略。
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_module.h"

/* 日志 TAG */
static const char *TAG = "mqtt_module";           ///< 本模块日志 TAG

/* MQTT 模块配置与状态 */
static mqtt_module_config_t   s_mqtt_cfg;          ///< 保存一份配置副本
static bool                   s_mqtt_inited = false; ///< 是否已初始化
static esp_mqtt_client_handle_t s_mqtt_client = NULL; ///< MQTT 客户端句柄

/**
 * @brief 内部辅助：统一分发事件到上层回调
 * @param event 要分发的事件
 */
static void mqtt_module_dispatch_event(mqtt_module_event_t event)
{
    if (s_mqtt_cfg.event_cb) {                    ///< 若上层配置了回调
        s_mqtt_cfg.event_cb(event);               ///< 则转发事件
    }
}

/**
 * @brief esp-mqtt 事件处理回调
 *
 * 由 IDF MQTT 客户端在其内部任务中调用，用于通知连接状态变化等。
 * 将底层复杂的事件转换为简化的 mqtt_module_event_t 上报。
 */
static void mqtt_module_event_handler(void           *handler_args,
                                      esp_event_base_t base,
                                      int32_t         event_id,
                                      void           *event_data)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {       ///< 根据事件 ID 分类处理
    case MQTT_EVENT_CONNECTED:                     ///< 已连接事件
        ESP_LOGI(TAG, "MQTT connected");
        mqtt_module_dispatch_event(MQTT_MODULE_EVENT_CONNECTED); ///< 上报已连接
        break;

    case MQTT_EVENT_DISCONNECTED:                  ///< 断开连接事件
        ESP_LOGW(TAG, "MQTT disconnected");
        mqtt_module_dispatch_event(MQTT_MODULE_EVENT_DISCONNECTED); ///< 上报断开
        break;

    case MQTT_EVENT_ERROR:                         ///< 错误事件
        ESP_LOGE(TAG, "MQTT error");
        mqtt_module_dispatch_event(MQTT_MODULE_EVENT_ERROR); ///< 上报错误
        break;

    case MQTT_EVENT_DATA:                          ///< 收到一条 MQTT 消息
        ESP_LOGI(TAG, "MQTT data: topic=%.*s, len=%d",
                 event->topic_len,
                 event->topic,
                 event->data_len);

        if (s_mqtt_cfg.message_cb) {                ///< 若配置了消息回调
            s_mqtt_cfg.message_cb(                  ///< 调用上层回调
                event->topic,                       ///< Topic 指针
                (int)event->topic_len,              ///< Topic 长度
                (const uint8_t *)event->data,       ///< 负载指针
                (int)event->data_len);              ///< 负载长度
        }
        break;

    default:                                       ///< 其他事件暂不关心
        break;
    }
}

/* 初始化 MQTT 模块 */
esp_err_t mqtt_module_init(const mqtt_module_config_t *config)
{
    /* 若已初始化则直接返回 */
    if (s_mqtt_inited) {
        return ESP_OK;
    }

    /* 装载配置：优先使用外部配置，否则使用默认配置 */
    if (config == NULL) {
        s_mqtt_cfg = MQTT_MODULE_DEFAULT_CONFIG();
    } else {
        s_mqtt_cfg = *config;
    }

    /* broker_uri 为必填项 */
    if (s_mqtt_cfg.broker_uri == NULL || s_mqtt_cfg.broker_uri[0] == '\0') {
        ESP_LOGE(TAG, "Broker URI is required");
        return ESP_ERR_INVALID_ARG;
    }

    /* 构造 esp-mqtt 配置 */
    esp_mqtt_client_config_t mqtt_cfg = {0};

    // 配置broker地址
    mqtt_cfg.broker.address.uri = s_mqtt_cfg.broker_uri;

    // 配置认证信息
    if (s_mqtt_cfg.client_id != NULL) {
        mqtt_cfg.credentials.client_id = s_mqtt_cfg.client_id;
    }
    if (s_mqtt_cfg.username != NULL) {
        mqtt_cfg.credentials.username = s_mqtt_cfg.username;
    }
    if (s_mqtt_cfg.password != NULL) {
        mqtt_cfg.credentials.authentication.password = s_mqtt_cfg.password;
    }

    // 配置连接保持时间
    if (s_mqtt_cfg.keepalive_sec > 0) {
        mqtt_cfg.session.keepalive = (uint16_t)s_mqtt_cfg.keepalive_sec;
    } else {
        mqtt_cfg.session.keepalive = 60; // 默认60s
    }

    /* 创建 MQTT 客户端实例 */
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return ESP_ERR_NO_MEM;
    }

    /* 注册事件回调 */
    esp_err_t ret = esp_mqtt_client_register_event(
        s_mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_module_event_handler,
        NULL);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_register_event failed: %s", esp_err_to_name(ret));
        esp_mqtt_client_destroy(s_mqtt_client);
        s_mqtt_client = NULL;
        return ret;
    }

    s_mqtt_inited = true;
    ESP_LOGI(TAG, "MQTT module initialized");
    return ESP_OK;
}

/* 启动 MQTT 客户端 */
esp_err_t mqtt_module_start(void)
{
    if (!s_mqtt_inited || s_mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 启动 MQTT 客户端（内部将自动重连等） */
    esp_err_t ret = esp_mqtt_client_start(s_mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MQTT module started");
    return ESP_OK;
}

/* 停止 MQTT 客户端 */
esp_err_t mqtt_module_stop(void)
{
    if (!s_mqtt_inited || s_mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 停止客户端，不销毁句柄，便于后续再次启动 */
    esp_err_t ret = esp_mqtt_client_stop(s_mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_stop failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MQTT module stopped");
    return ESP_OK;
}

/* 发布消息 */
esp_err_t mqtt_module_publish(const char *topic,
                              const void *payload,
                              int         len,
                              int         qos,
                              bool        retain)
{
    if (!s_mqtt_inited || s_mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (topic == NULL || topic[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    if (len < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *data = (const char *)payload;

    // 调用底层API发送消息，返回msg_id或者-1
    int msg_id = esp_mqtt_client_publish(
        s_mqtt_client,
        topic,
        data,
        len,
        qos,
        retain);

    if (msg_id < 0) {
        ESP_LOGE(TAG, "esp_mqtt_client_publish failed, ret=%d", msg_id);
        return ESP_FAIL;
    }

    return ESP_OK; // 成功加入发送队列
}

/* 订阅主题 */
esp_err_t mqtt_module_subscribe(const char *topic, int qos)
{
    if (!s_mqtt_inited || s_mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (topic == NULL || topic[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    if (qos < 0 || qos > 2) {
        return ESP_ERR_INVALID_ARG;
    }

    // 发起订阅请求
    int msg_id = esp_mqtt_client_subscribe(
        s_mqtt_client,
        topic,
        qos);

    if (msg_id < 0) {
        ESP_LOGE(TAG, "esp_mqtt_client_subscribe failed, ret=%d", msg_id);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Subscribed to %s (msg_id=%d)", topic, msg_id);
    return ESP_OK;
}

/* 取消订阅 */
esp_err_t mqtt_module_unsubscribe(const char *topic)
{
    if (!s_mqtt_inited || s_mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (topic == NULL || topic[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    // 发起取消订阅请求
    int msg_id = esp_mqtt_client_unsubscribe(
        s_mqtt_client,
        topic);

    if (msg_id < 0) {
        ESP_LOGE(TAG, "esp_mqtt_client_unsubscribe failed, ret=%d", msg_id);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Unsubscribed from %s (msg_id=%d)", topic, msg_id);
    return ESP_OK;
}
