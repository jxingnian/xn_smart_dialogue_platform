/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:51:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\mqtt_manager.c
 * @Description: MQTT应用管理器实现 - 业务逻辑管理（策略层）
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#include <string.h>                                 // 字符串处理函数
#include <stdio.h>                                  // 标准输入输出
#include <stdint.h>                                 // 标准整型定义
#include "freertos/FreeRTOS.h"                      // FreeRTOS核心头文件
#include "freertos/task.h"                          // FreeRTOS任务头文件
#include "esp_log.h"                                // ESP日志模块
#include "esp_mac.h"                                // ESP MAC地址获取
#include "xn_event_bus.h"                           // 事件总线模块
#include "mqtt_manager.h"                           // 本模块头文件
#include "mqtt_module.h"                            // MQTT底层API模块

/* 日志TAG */
static const char *TAG = "mqtt_manager";            // 本模块日志TAG

/* ========================================================================== */
/*                              内部变量                                        */
/* ========================================================================== */

static mqtt_manager_config_t s_mgr_cfg;             // 上层传入的管理配置副本
static mqtt_manager_state_t  s_mgr_state = MQTT_MANAGER_STATE_IDLE; // 当前状态
static TaskHandle_t          s_mgr_task  = NULL;    // 管理任务句柄
static TickType_t            s_last_error_ts = 0;   // 最近一次错误/断开的时间戳
static bool                  s_initialized = false; // 初始化标志

/* 若上层未指定client_id，则使用该缓冲区生成一个基于MAC的默认ID */
static char s_client_id_buf[32];                    // 客户端ID缓冲区

/* ========================================================================== */
/*                              内部函数                                        */
/* ========================================================================== */

/**
 * @brief 统一更新状态并通知上层回调
 *
 * @param new_state 新状态
 */
static void mqtt_manager_notify_state(mqtt_manager_state_t new_state)
{
    // 保存新状态
    s_mgr_state = new_state;

    // 如上层配置了状态回调则通知
    if (s_mgr_cfg.state_cb) {
        s_mgr_cfg.state_cb(new_state);
    }

    // 根据状态发布事件到事件总线
    switch (new_state) {
        case MQTT_MANAGER_STATE_CONNECTED:          // 已连接状态
            xn_event_post(XN_EVT_MQTT_CONNECTED, XN_EVT_SRC_MQTT);   // 发布MQTT已连接事件
            break;
        case MQTT_MANAGER_STATE_DISCONNECTED:       // 已断开状态
            xn_event_post(XN_EVT_MQTT_DISCONNECTED, XN_EVT_SRC_MQTT); // 发布MQTT已断开事件
            break;
        case MQTT_MANAGER_STATE_ERROR:              // 错误状态
            xn_event_post(XN_EVT_MQTT_ERROR, XN_EVT_SRC_MQTT);      // 发布MQTT错误事件
            break;
        default:
            break;
    }
}

/**
 * @brief 确保管理器配置中存在有效的client_id
 *
 * 若上层未提供（NULL或空串），则基于芯片MAC生成形如
 * "ESP32_XXXXXXXXXXXX"的默认client_id，保证单设备唯一
 */
static void mqtt_manager_ensure_client_id(void)
{
    // 检查上层是否已提供有效的client_id
    if (s_mgr_cfg.client_id != NULL && s_mgr_cfg.client_id[0] != '\0') {
        return;                                     // 直接使用上层提供的值
    }

    // 读取芯片MAC地址
    uint8_t mac[6] = {0};
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        // MAC读取失败，使用默认名称
        snprintf(s_client_id_buf, sizeof(s_client_id_buf), "ESP32_UNKNOWN");
    } else {
        // 基于MAC生成唯一client_id
        snprintf(s_client_id_buf, sizeof(s_client_id_buf),
                 "ESP32_%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // 填入默认client_id
    s_mgr_cfg.client_id = s_client_id_buf;
}

/**
 * @brief MQTT模块事件回调
 *
 * 由mqtt_module在底层连接状态变化时调用，用于驱动管理器状态机
 */
static void mqtt_manager_on_mqtt_event(mqtt_module_event_t event)
{
    switch (event) {
        case MQTT_MODULE_EVENT_CONNECTED:           // 底层已连接
            ESP_LOGI(TAG, "MQTT connected");
            mqtt_manager_notify_state(MQTT_MANAGER_STATE_CONNECTED); // 更新为已连接
            s_last_error_ts = 0;                    // 清空错误时间戳
            break;

        case MQTT_MODULE_EVENT_DISCONNECTED:        // 底层断开
            ESP_LOGW(TAG, "MQTT disconnected");
            mqtt_manager_notify_state(MQTT_MANAGER_STATE_DISCONNECTED); // 更新为断开
            s_last_error_ts = xTaskGetTickCount();  // 记录断开时间
            break;

        case MQTT_MODULE_EVENT_ERROR:               // 底层错误
        default:
            ESP_LOGE(TAG, "MQTT error");
            mqtt_manager_notify_state(MQTT_MANAGER_STATE_ERROR); // 更新为错误状态
            s_last_error_ts = xTaskGetTickCount();  // 记录错误时间
            break;
    }
}

/**
 * @brief MQTT消息接收回调
 *
 * 由底层模块在收到消息时调用
 */
static void mqtt_manager_on_message(const char *topic, int topic_len, 
                                    const uint8_t *payload, int payload_len)
{
    ESP_LOGD(TAG, "Received data: topic=%.*s", topic_len, topic);
    
    // 透传给上层配置的回调
    if (s_mgr_cfg.message_cb) {
        s_mgr_cfg.message_cb(topic, topic_len, payload, payload_len);
    }

    // 封装后发布到事件总线
    xn_evt_mqtt_data_t data = {
        .topic = (char *)topic,                     // Topic指针
        .topic_len = topic_len,                     // Topic长度
        .data = (char *)payload,                    // 负载数据指针
        .data_len = payload_len,                    // 负载数据长度
        .msg_id = 0,                                // 模块层未透传msg_id
    };
    xn_event_post_data(XN_EVT_MQTT_DATA, XN_EVT_SRC_MQTT, &data, sizeof(data));
}

/**
 * @brief 单步执行MQTT管理状态机
 *
 * 根据当前状态与配置决定是否发起连接或等待重试
 */
static void mqtt_manager_step(void)
{
    switch (s_mgr_state) {                          // 根据当前状态分类
        case MQTT_MANAGER_STATE_DISCONNECTED:       // 断开状态
        case MQTT_MANAGER_STATE_ERROR: {            // 错误状态
            // 小于0表示不自动重连
            if (s_mgr_cfg.reconnect_interval_ms < 0) {
                break;
            }

            // 获取当前时间
            TickType_t now   = xTaskGetTickCount();
            // 计算距离上次出错的时间差
            TickType_t delta = (s_last_error_ts == 0) ? 0 : (now - s_last_error_ts);

            // 计算需要等待的Tick数
            TickType_t need = pdMS_TO_TICKS(
                (s_mgr_cfg.reconnect_interval_ms <= 0)
                    ? 0
                    : s_mgr_cfg.reconnect_interval_ms);

            // 检查是否已到达重试时间
            if (delta >= need) {
                ESP_LOGI(TAG, "try connect MQTT server");
                mqtt_manager_notify_state(MQTT_MANAGER_STATE_CONNECTING); // 进入连接中
                (void)mqtt_module_start();          // 发起一次连接尝试
                
                // 如果是首次启动，也需要记录时间防止短时间内反复重连
                if (s_last_error_ts == 0) {
                    s_last_error_ts = now;
                }
            }
            break;
        }

        case MQTT_MANAGER_STATE_CONNECTED:          // 已连接状态
        case MQTT_MANAGER_STATE_IDLE:               // 空闲状态
        default:
            // 已连接状态下，通常由MQTT内部心跳维持，应用层暂不做额外动作
            break;
    }
}

/**
 * @brief MQTT管理任务：周期性驱动状态机
 */
static void mqtt_manager_task(void *arg)
{
    (void)arg;

    for (;;) {
        // 单步执行状态机
        mqtt_manager_step();

        // 获取休眠间隔
        int interval_ms = s_mgr_cfg.step_interval_ms;
        if (interval_ms <= 0) {
            // 若未配置使用默认值
            interval_ms = MQTT_MANAGER_STEP_INTERVAL_MS;
        }

        // 休眠一段时间
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

/**
 * @brief 系统事件处理回调
 * 
 * 监听WiFi状态事件，实现自动连接逻辑
 */
static void system_event_handler(const xn_event_t *event, void *user_data)
{
    (void)user_data;
    
    // 根据事件ID处理
    switch (event->id) {
        case XN_EVT_WIFI_GOT_IP:
            // WiFi连接成功
            ESP_LOGI(TAG, "WiFi Got IP, MQTT will connect automatically");
            // 重置错误时间戳，使状态机立即尝试连接
            if (s_mgr_state == MQTT_MANAGER_STATE_DISCONNECTED ||
                s_mgr_state == MQTT_MANAGER_STATE_ERROR) {
                s_last_error_ts = 0;
            }
            break;
            
        case XN_EVT_WIFI_DISCONNECTED:
            // WiFi断开
            ESP_LOGW(TAG, "WiFi Disconnected, MQTT will pause");
            // 底层MQTT会自动检测到网络错误并进入重连等待
            break;
            
        default:
            break;
    }
}

/* ========================================================================== */
/*                              公共API实现                                     */
/* ========================================================================== */

/* 初始化MQTT管理器 */
esp_err_t mqtt_manager_init(const mqtt_manager_config_t *config)
{
    // 检查是否已初始化
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // 装载配置：优先使用上层配置，否则使用默认配置
    if (config == NULL) {
        s_mgr_cfg = MQTT_MANAGER_DEFAULT_CONFIG();
    } else {
        s_mgr_cfg = *config;
    }

    // broker_uri必须提供
    if (s_mgr_cfg.broker_uri == NULL || s_mgr_cfg.broker_uri[0] == '\0') {
        ESP_LOGE(TAG, "Broker URI is required");
        return ESP_ERR_INVALID_ARG;
    }

    // 若未指定client_id，则基于MAC生成一个默认client_id
    mqtt_manager_ensure_client_id();

    // 组装MQTT模块配置
    mqtt_module_config_t mqtt_cfg = MQTT_MODULE_DEFAULT_CONFIG();
    mqtt_cfg.broker_uri    = s_mgr_cfg.broker_uri;  // 设置Broker地址
    mqtt_cfg.client_id     = s_mgr_cfg.client_id;   // 设置客户端ID
    mqtt_cfg.username      = s_mgr_cfg.username;    // 设置用户名
    mqtt_cfg.password      = s_mgr_cfg.password;    // 设置密码

    // 设置keepalive
    if (s_mgr_cfg.keepalive_sec > 0) {
        mqtt_cfg.keepalive_sec = s_mgr_cfg.keepalive_sec;
    }

    // 绑定内部回调，用于将MQTT底层事件转换为Manager状态更新
    mqtt_cfg.event_cb   = mqtt_manager_on_mqtt_event;
    mqtt_cfg.message_cb = mqtt_manager_on_message;

    // 初始化底层MQTT模块
    esp_err_t ret = mqtt_module_init(&mqtt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mqtt_module_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 初始化状态
    s_mgr_state     = MQTT_MANAGER_STATE_DISCONNECTED;
    s_last_error_ts = 0;

    // 创建管理任务（仅创建一次）
    if (s_mgr_task == NULL) {
        BaseType_t ret_task = xTaskCreate(
            mqtt_manager_task,                      // 任务函数
            "mqtt_mgr",                             // 任务名称
            4096,                                   // 栈大小
            NULL,                                   // 任务参数
            tskIDLE_PRIORITY + 1,                   // 任务优先级
            &s_mgr_task);                           // 任务句柄

        if (ret_task != pdPASS) {
            ESP_LOGE(TAG, "Create task failed");
            s_mgr_task = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    // 订阅系统事件 - 关注WiFi状态
    xn_event_subscribe(XN_EVT_WIFI_GOT_IP, system_event_handler, NULL);
    xn_event_subscribe(XN_EVT_WIFI_DISCONNECTED, system_event_handler, NULL);

    // 标记初始化完成
    s_initialized = true;

    // 通知状态变更
    mqtt_manager_notify_state(MQTT_MANAGER_STATE_DISCONNECTED);
    
    // 立即启动第一次连接尝试，不等待Task轮询
    ESP_LOGI(TAG, "Start initial connection...");
    (void)mqtt_module_start();

    ESP_LOGI(TAG, "MQTT manager initialized");
    return ESP_OK;
}

/* 反初始化MQTT管理器 */
esp_err_t mqtt_manager_deinit(void)
{
    // 检查是否已初始化
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // 取消订阅系统事件
    xn_event_unsubscribe(XN_EVT_WIFI_GOT_IP, system_event_handler);
    xn_event_unsubscribe(XN_EVT_WIFI_DISCONNECTED, system_event_handler);

    // 停止管理任务
    if (s_mgr_task != NULL) {
        vTaskDelete(s_mgr_task);
        s_mgr_task = NULL;
    }

    // 停止MQTT
    mqtt_module_stop();

    // 重置状态
    s_mgr_state = MQTT_MANAGER_STATE_IDLE;
    s_last_error_ts = 0;
    s_initialized = false;

    ESP_LOGI(TAG, "MQTT manager deinitialized");
    return ESP_OK;
}

/* 手动启动MQTT连接 */
esp_err_t mqtt_manager_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // 重置错误时间戳以触发立即连接
    s_last_error_ts = 0;
    s_mgr_state = MQTT_MANAGER_STATE_DISCONNECTED;

    return mqtt_module_start();
}

/* 手动停止MQTT连接 */
esp_err_t mqtt_manager_stop(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = mqtt_module_stop();
    if (ret == ESP_OK) {
        s_mgr_state = MQTT_MANAGER_STATE_IDLE;
    }
    return ret;
}

/* 发布消息 */
esp_err_t mqtt_manager_publish(const char *topic, const void *data, size_t len, int qos)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return mqtt_module_publish(topic, data, (int)len, qos, false);
}

/* 订阅主题 */
esp_err_t mqtt_manager_subscribe(const char *topic, int qos)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return mqtt_module_subscribe(topic, qos);
}

/* 取消订阅主题 */
esp_err_t mqtt_manager_unsubscribe(const char *topic)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return mqtt_module_unsubscribe(topic);
}

/* 获取当前状态 */
mqtt_manager_state_t mqtt_manager_get_state(void)
{
    return s_mgr_state;
}

/* 检查是否已连接 */
bool mqtt_manager_is_connected(void)
{
    return (s_mgr_state == MQTT_MANAGER_STATE_CONNECTED);
}

/* 获取客户端ID */
const char *mqtt_manager_get_client_id(void)
{
    return s_mgr_cfg.client_id;
}
