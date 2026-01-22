/*
 * @Author: 星年 jixingnian@gmail.com
 * @Date: 2025-01-15
 * @Description: Web MQTT 管理器实现（基于 mqtt_module 封装状态机与重连逻辑）
 * 
 * 职责划分：
 *  - 作为上层管理模块，负责 MQTT 连接状态机与自动重连策略；
 *  - 通过 mqtt_module 完成具体的 MQTT 客户端初始化与连接；
 *  - 通过 web_mqtt_event_cb_t 回调向上层报告抽象状态变化。
 * 
 * 不直接处理业务 Topic，只关注“是否连上 MQTT 服务器”。
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "mqtt_module.h"
#include "web_mqtt_manager.h"

/* 日志 TAG */
static const char *TAG = "web_mqtt_manager";       ///< 本模块日志 TAG

/* 管理器内部状态 */
static web_mqtt_manager_config_t s_mgr_cfg;        ///< 上层传入的管理配置副本
static web_mqtt_state_t          s_mgr_state = WEB_MQTT_STATE_DISCONNECTED; ///< 当前状态
static TaskHandle_t              s_mgr_task  = NULL; ///< 管理任务句柄
static TickType_t                s_last_error_ts = 0; ///< 最近一次错误/断开的时间戳

/* 若上层未指定 client_id，则使用该缓冲区生成一个基于 MAC 的默认 ID */
static char s_client_id_buf[32];

/**
 * @brief 统一更新状态并通知上层回调
 *
 * @param new_state 新状态
 */
static void web_mqtt_manager_notify_state(web_mqtt_state_t new_state)
{
    s_mgr_state = new_state;

    if (s_mgr_cfg.event_cb) {                      ///< 如上层配置了回调
        s_mgr_cfg.event_cb(new_state);             ///< 通知上层当前状态
    }
}

/**
 * @brief 确保管理器配置中存在有效的 client_id
 *
 * 若上层未提供（NULL 或空串），则基于芯片 MAC 生成形如
 * "ESP32_XXXXXXXXXXXX" 的默认 client_id，保证单设备唯一。
 */
static void web_mqtt_manager_ensure_client_id(void)
{
    if (s_mgr_cfg.client_id != NULL && s_mgr_cfg.client_id[0] != '\0') {
        return; // 直接使用上层提供的值
    }

    uint8_t mac[6] = {0};
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        snprintf(s_client_id_buf, sizeof(s_client_id_buf), "ESP32_UNKNOWN");
    } else {
        snprintf(s_client_id_buf, sizeof(s_client_id_buf),
                 "ESP32_%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    s_mgr_cfg.client_id = s_client_id_buf;        ///< 填入默认 client_id
}

/**
 * @brief MQTT 模块事件回调
 *
 * 由 mqtt_module 在底层连接状态变化时调用，用于驱动管理器状态机。
 */
static void web_mqtt_manager_on_mqtt_event(mqtt_module_event_t event)
{
    switch (event) {
    case MQTT_MODULE_EVENT_CONNECTED:              ///< 底层已连接
        ESP_LOGI(TAG, "MQTT connected");
        web_mqtt_manager_notify_state(WEB_MQTT_STATE_CONNECTED); ///< 更新为已连接
        s_last_error_ts = 0;                       ///< 清空错误时间戳
        break;

    case MQTT_MODULE_EVENT_DISCONNECTED:           ///< 底层断开
        ESP_LOGW(TAG, "MQTT disconnected");
        web_mqtt_manager_notify_state(WEB_MQTT_STATE_DISCONNECTED); ///< 更新为断开
        s_last_error_ts = xTaskGetTickCount();     ///< 记录断开时间
        break;

    case MQTT_MODULE_EVENT_ERROR:                  ///< 底层错误
    default:
        ESP_LOGE(TAG, "MQTT error");
        web_mqtt_manager_notify_state(WEB_MQTT_STATE_ERROR); ///< 更新为错误状态
        s_last_error_ts = xTaskGetTickCount();     ///< 记录错误时间
        break;
    }
}

/**
 * @brief 单步执行 Web MQTT 管理状态机
 *
 * 根据当前状态与配置决定是否发起连接或等待重试。
 */
static void web_mqtt_manager_step(void)
{
    switch (s_mgr_state) {                         ///< 根据当前状态分类
    case WEB_MQTT_STATE_DISCONNECTED:              ///< 断开状态
    case WEB_MQTT_STATE_ERROR: {                   ///< 错误状态
        if (s_mgr_cfg.reconnect_interval_ms < 0) { ///< 小于 0 表示不自动重连
            break;
        }

        TickType_t now   = xTaskGetTickCount();
        // 计算距离上次出错的时间差
        TickType_t delta = (s_last_error_ts == 0) ? 0 : (now - s_last_error_ts);

        // 计算需要等待的Tick数
        TickType_t need = pdMS_TO_TICKS(
            (s_mgr_cfg.reconnect_interval_ms <= 0)
                ? 0
                : s_mgr_cfg.reconnect_interval_ms);

        if (delta >= need) {                       ///< 已到达重试时间
            ESP_LOGI(TAG, "try connect MQTT server");
            web_mqtt_manager_notify_state(WEB_MQTT_STATE_CONNECTING); ///< 进入连接中
            (void)mqtt_module_start();             ///< 发起一次连接尝试
            
            // 如果是首次启动（s_last_error_ts == 0），也需要记录一下时间，防止短时间内反复重连
            if (s_last_error_ts == 0) {
                s_last_error_ts = now;
            }
        }
        break;
    }

    case WEB_MQTT_STATE_CONNECTED:                 ///< 已连接状态
    case WEB_MQTT_STATE_READY:                     ///< 业务准备就绪
    default:
        // 已连接状态下，通常由 MQTT 内部心跳维持，应用层暂不做额外动作
        break;
    }
}

/**
 * @brief Web MQTT 管理任务：周期性驱动状态机
 */
static void web_mqtt_manager_task(void *arg)
{
    (void)arg;

    for (;;) {
        web_mqtt_manager_step();                   ///< 单步执行状态机

        int interval_ms = s_mgr_cfg.step_interval_ms;
        if (interval_ms <= 0) {                    ///< 若未配置有可能为0，使用默认
            interval_ms = WEB_MQTT_MANAGER_STEP_INTERVAL_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(interval_ms));    ///< 休眠一段时间
    }
}

/* 初始化 Web MQTT 管理器 */
esp_err_t web_mqtt_manager_init(const web_mqtt_manager_config_t *config)
{
    /* 装载配置：优先使用上层配置，否则使用默认配置 */
    if (config == NULL) {
        s_mgr_cfg = WEB_MQTT_MANAGER_DEFAULT_CONFIG();
    } else {
        s_mgr_cfg = *config;
    }

    /* broker_uri 必须提供 */
    if (s_mgr_cfg.broker_uri == NULL || s_mgr_cfg.broker_uri[0] == '\0') {
        ESP_LOGE(TAG, "Broker URI is required");
        return ESP_ERR_INVALID_ARG;
    }

    /* 若未指定 client_id，则基于 MAC 生成一个默认 client_id */
    web_mqtt_manager_ensure_client_id();

    /* 组装 MQTT 模块配置 */
    mqtt_module_config_t mqtt_cfg = MQTT_MODULE_DEFAULT_CONFIG();

    mqtt_cfg.broker_uri    = s_mgr_cfg.broker_uri;
    mqtt_cfg.client_id     = s_mgr_cfg.client_id;
    mqtt_cfg.username      = s_mgr_cfg.username;
    mqtt_cfg.password      = s_mgr_cfg.password;

    if (s_mgr_cfg.keepalive_sec > 0) {
        mqtt_cfg.keepalive_sec = s_mgr_cfg.keepalive_sec;
    }

    // 绑定内部回调，用于将 MQTT 底层事件转换为 Manager 状态更新
    mqtt_cfg.event_cb      = web_mqtt_manager_on_mqtt_event;
    mqtt_cfg.message_cb    = NULL; // 当前 Manager 层不直接处理消息，可由上层通过 mqtt_module_register_... 扩展

    /* 初始化底层 MQTT 模块 */
    esp_err_t ret = mqtt_module_init(&mqtt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mqtt_module_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 初始化状态 */
    s_mgr_state     = WEB_MQTT_STATE_DISCONNECTED;
    s_last_error_ts = 0;

    /* 创建管理任务（仅创建一次） */
    if (s_mgr_task == NULL) {
        BaseType_t ret_task = xTaskCreate(
            web_mqtt_manager_task,
            "web_mqtt_mgr",
            4096,
            NULL,
            tskIDLE_PRIORITY + 1,
            &s_mgr_task);

        if (ret_task != pdPASS) {
            ESP_LOGE(TAG, "Create task failed");
            s_mgr_task = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    /* 初始化完成后，可立即触发一次连接尝试 */
    web_mqtt_manager_notify_state(WEB_MQTT_STATE_DISCONNECTED);
    
    // 立即启动第一次连接尝试，不等待Task轮询
    ESP_LOGI(TAG, "Start initial connection...");
    (void)mqtt_module_start();

    return ESP_OK;
}

const char *web_mqtt_manager_get_client_id(void)
{
    return s_mgr_cfg.client_id;
}

const char *web_mqtt_manager_get_base_topic(void)
{
    return s_mgr_cfg.base_topic;
}
