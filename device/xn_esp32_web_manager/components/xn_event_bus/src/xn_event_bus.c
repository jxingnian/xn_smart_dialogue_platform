/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_event_bus\src\xn_event_bus.c
 * @Description: 事件总线实现 - 提供Pub/Sub机制
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "xn_event_bus.h"

static const char *TAG = "xn_event_bus";

/*===========================================================================
 *                          内部数据结构
 *===========================================================================*/

/**
 * @brief 订阅者节点结构体
 * 使用链表存储每个事件ID的订阅者
 */
typedef struct subscriber_node {
    uint16_t event_id;              ///< 订阅的事件ID (XN_EVT_ANY 表示所有事件)
    xn_event_handler_t handler;     ///< 回调函数指针
    void *user_data;                ///< 用户数据，调用回调时传回
    struct subscriber_node *next;   ///< 链表下一节点
} subscriber_node_t;

/**
 * @brief 事件总线管理结构体
 */
typedef struct {
    bool initialized;                   ///< 初始化标志
    QueueHandle_t event_queue;          ///< 异步事件队列句柄
    TaskHandle_t dispatcher_task;       ///< 事件分发任务句柄
    SemaphoreHandle_t subscriber_mutex; ///< 订阅者链表保护互斥锁
    subscriber_node_t *subscribers;     ///< 订阅者链表头指针
    
    // 统计信息
    uint32_t stats_published;           ///< 已发布事件计数
    uint32_t stats_delivered;           ///< 已投递(处理)事件计数
    uint32_t stats_dropped;             ///< 丢弃事件计数(队列满)
} event_bus_t;

// 全局唯一的事件总线实例
static event_bus_t s_bus = {0};

/*===========================================================================
 *                          内部函数
 *===========================================================================*/

/**
 * @brief 获取当前系统时间戳(毫秒)
 * @return uint32_t 毫秒级时间戳
 */
static uint32_t get_timestamp_ms(void)
{
    // esp_timer_get_time() 返回微秒，转换为毫秒
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/**
 * @brief 执行事件分发
 * 遍历订阅者链表，找到匹配的订阅者并调用回调
 * @param event 待分发的事件
 */
static void dispatch_event(const xn_event_t *event)
{
    // 锁定订阅者链表，防止分发过程中链表被修改
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    
    subscriber_node_t *node = s_bus.subscribers;
    // 遍历所有订阅者
    while (node != NULL) {
        // 匹配规则：订阅了 XN_EVT_ANY 或 事件ID完全匹配
        if (node->event_id == XN_EVT_ANY || node->event_id == event->id) {
            if (node->handler != NULL) {
                // 调用回调函数
                node->handler(event, node->user_data);
                s_bus.stats_delivered++;
            }
        }
        node = node->next;
    }
    
    // 释放锁
    xSemaphoreGive(s_bus.subscriber_mutex);
    
    // 如果设置了自动释放且数据指针不为空，则释放数据内存
    if (event->auto_free && event->data != NULL) {
        free(event->data);
    }
}

/**
 * @brief 事件分发任务函数
 * 持续从队列中取出事件并分发
 * @param arg 任务参数（未使用）
 */
static void dispatcher_task(void *arg)
{
    xn_event_t event;
    
    // 打印任务启动日志
    ESP_LOGI(TAG, "Dispatcher task started");
    
    while (1) {
        // 阻塞等待队列中有新事件
        if (xQueueReceive(s_bus.event_queue, &event, portMAX_DELAY) == pdTRUE) {
            // 分发事件
            dispatch_event(&event);
        }
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

/* 初始化事件总线 */
esp_err_t xn_event_bus_init(void)
{
    // 检查是否已初始化
    if (s_bus.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 创建事件队列
    s_bus.event_queue = xQueueCreate(XN_EVENT_QUEUE_SIZE, sizeof(xn_event_t));
    if (s_bus.event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }
    
    // 创建互斥锁
    s_bus.subscriber_mutex = xSemaphoreCreateMutex();
    if (s_bus.subscriber_mutex == NULL) {
        vQueueDelete(s_bus.event_queue);
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // 创建分发任务
    BaseType_t ret = xTaskCreate(
        dispatcher_task,
        "event_dispatcher",
        XN_EVENT_TASK_STACK_SIZE,
        NULL,
        XN_EVENT_TASK_PRIORITY,
        &s_bus.dispatcher_task
    );
    
    if (ret != pdPASS) {
        vSemaphoreDelete(s_bus.subscriber_mutex);
        vQueueDelete(s_bus.event_queue);
        ESP_LOGE(TAG, "Failed to create dispatcher task");
        return ESP_ERR_NO_MEM;
    }
    
    // 初始化状态
    s_bus.subscribers = NULL;
    s_bus.stats_published = 0;
    s_bus.stats_delivered = 0;
    s_bus.stats_dropped = 0;
    s_bus.initialized = true;
    
    ESP_LOGI(TAG, "Event bus initialized (queue=%d, max_subs=%d)", 
             XN_EVENT_QUEUE_SIZE, XN_EVENT_MAX_SUBSCRIBERS);
    
    return ESP_OK;
}

/* 反初始化事件总线 */
esp_err_t xn_event_bus_deinit(void)
{
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 停止并删除分发任务
    if (s_bus.dispatcher_task != NULL) {
        vTaskDelete(s_bus.dispatcher_task);
        s_bus.dispatcher_task = NULL;
    }
    
    // 清理所有订阅者节点
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    subscriber_node_t *node = s_bus.subscribers;
    while (node != NULL) {
        subscriber_node_t *next = node->next;
        free(node); // 释放节点内存
        node = next;
    }
    s_bus.subscribers = NULL;
    xSemaphoreGive(s_bus.subscriber_mutex);
    
    // 清理队列中剩余的未处理事件，如果是auto_free的需要释放数据
    xn_event_t event;
    while (xQueueReceive(s_bus.event_queue, &event, 0) == pdTRUE) {
        if (event.auto_free && event.data != NULL) {
            free(event.data);
        }
    }
    
    // 删除同步原语
    vSemaphoreDelete(s_bus.subscriber_mutex);
    vQueueDelete(s_bus.event_queue);
    
    s_bus.initialized = false;
    ESP_LOGI(TAG, "Event bus deinitialized");
    
    return ESP_OK;
}

/* 发布事件（异步） */
esp_err_t xn_event_publish(const xn_event_t *event)
{
    // 检查初始化状态
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    // 检查参数
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 复制事件结构体，避免外部修改或栈变量失效
    xn_event_t evt_copy = *event;
    // 如果未设置时间戳，自动填充当前时间
    if (evt_copy.timestamp == 0) {
        evt_copy.timestamp = get_timestamp_ms();
    }
    
    // 发送到队列，如果队列满则丢弃（非阻塞发送）
    if (xQueueSend(s_bus.event_queue, &evt_copy, 0) != pdTRUE) {
        s_bus.stats_dropped++;
        ESP_LOGW(TAG, "Event queue full, dropped event 0x%04x", event->id);
        // 如果发送失败且需要自动释放，这里必须释放，否则内存泄漏
        if (evt_copy.auto_free && evt_copy.data) {
             free(evt_copy.data);
        }
        return ESP_FAIL;
    }
    
    s_bus.stats_published++;
    ESP_LOGD(TAG, "Published event 0x%04x", event->id);
    
    return ESP_OK;
}

/* 发布事件（同步） */
esp_err_t xn_event_publish_sync(const xn_event_t *event)
{
    // 检查初始化状态
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    // 检查参数
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xn_event_t evt_copy = *event;
    if (evt_copy.timestamp == 0) {
        evt_copy.timestamp = get_timestamp_ms();
    }
    
    s_bus.stats_published++;
    // 直接在当前上下文分发
    dispatch_event(&evt_copy);
    
    return ESP_OK;
}

/* 快速发布无数据事件 */
esp_err_t xn_event_post(uint16_t event_id, uint16_t source)
{
    xn_event_t event = {
        .id = event_id,
        .source = source,
        .timestamp = get_timestamp_ms(),
        .data = NULL,
        .data_len = 0,
        .auto_free = false,
    };
    return xn_event_publish(&event);
}

/* 快速发布带数据事件（自动拷贝） */
esp_err_t xn_event_post_data(uint16_t event_id, uint16_t source, 
                              const void *data, size_t len)
{
    // 如果没有数据，退化为普通post
    if (data == NULL || len == 0) {
        return xn_event_post(event_id, source);
    }
    
    // 分配新内存并拷贝数据
    void *data_copy = malloc(len);
    if (data_copy == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memcpy(data_copy, data, len);
    
    // 构建事件，设置自动释放标志
    xn_event_t event = {
        .id = event_id,
        .source = source,
        .timestamp = get_timestamp_ms(),
        .data = data_copy,
        .data_len = len,
        .auto_free = true, // 重要：让总线处理完后自动free
    };
    
    // 发布事件
    esp_err_t ret = xn_event_publish(&event);
    if (ret != ESP_OK) {
        // 如果发布失败（例如队列满），需要手动释放刚才申请的内存
        // 注意：xn_event_publish内部已有释放逻辑，这里不需要重复
    }
    return ret;
}

/* 订阅事件 */
esp_err_t xn_event_subscribe(uint16_t event_id, xn_event_handler_t handler, 
                              void *user_data)
{
    // 检查初始化状态
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    // 检查参数
    if (handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 分配新的订阅者节点
    subscriber_node_t *new_node = malloc(sizeof(subscriber_node_t));
    if (new_node == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    new_node->event_id = event_id;
    new_node->handler = handler;
    new_node->user_data = user_data;
    
    // 头插法插入链表，需加锁保护
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    new_node->next = s_bus.subscribers;
    s_bus.subscribers = new_node;
    xSemaphoreGive(s_bus.subscriber_mutex);
    
    ESP_LOGD(TAG, "Subscribed to event 0x%04x", event_id);
    
    return ESP_OK;
}

/* 取消订阅 */
esp_err_t xn_event_unsubscribe(uint16_t event_id, xn_event_handler_t handler)
{
    // 检查初始化状态
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    
    subscriber_node_t *prev = NULL;
    subscriber_node_t *curr = s_bus.subscribers;
    
    while (curr != NULL) {
        // 找到匹配的节点（ID和Handler都必须匹配）
        if (curr->event_id == event_id && curr->handler == handler) {
            if (prev == NULL) {
                // 删除头节点
                s_bus.subscribers = curr->next;
            } else {
                // 删除中间节点
                prev->next = curr->next;
            }
            free(curr);
            xSemaphoreGive(s_bus.subscriber_mutex);
            return ESP_OK;
        }
        prev = curr;
        curr = curr->next;
    }
    
    xSemaphoreGive(s_bus.subscriber_mutex);
    return ESP_ERR_NOT_FOUND;
}

/* 取消handler的所有订阅 */
esp_err_t xn_event_unsubscribe_all(xn_event_handler_t handler)
{
    // 检查初始化状态
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    
    subscriber_node_t *prev = NULL;
    subscriber_node_t *curr = s_bus.subscribers;
    
    while (curr != NULL) {
        // 只要handler匹配就删除
        if (curr->handler == handler) {
            subscriber_node_t *to_free = curr;
            if (prev == NULL) {
                s_bus.subscribers = curr->next;
                curr = s_bus.subscribers; // 重置curr
            } else {
                prev->next = curr->next;
                curr = curr->next; // 移动curr
            }
            free(to_free);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
    
    xSemaphoreGive(s_bus.subscriber_mutex);
    return ESP_OK;
}

/* 获取待处理事件数量 */
uint32_t xn_event_pending_count(void)
{
    if (!s_bus.initialized) {
        return 0;
    }
    
    // 返回队列中消息数量
    return uxQueueMessagesWaiting(s_bus.event_queue);
}

