/**
 * @file xn_event_bus.c
 * @brief 事件总线实现
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

typedef struct subscriber_node {
    uint16_t event_id;
    xn_event_handler_t handler;
    void *user_data;
    struct subscriber_node *next;
} subscriber_node_t;

typedef struct {
    bool initialized;
    QueueHandle_t event_queue;
    TaskHandle_t dispatcher_task;
    SemaphoreHandle_t subscriber_mutex;
    subscriber_node_t *subscribers;
    uint32_t stats_published;
    uint32_t stats_delivered;
    uint32_t stats_dropped;
} event_bus_t;

static event_bus_t s_bus = {0};

/*===========================================================================
 *                          内部函数
 *===========================================================================*/

static uint32_t get_timestamp_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void dispatch_event(const xn_event_t *event)
{
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    
    subscriber_node_t *node = s_bus.subscribers;
    while (node != NULL) {
        if (node->event_id == XN_EVT_ANY || node->event_id == event->id) {
            if (node->handler != NULL) {
                node->handler(event, node->user_data);
                s_bus.stats_delivered++;
            }
        }
        node = node->next;
    }
    
    xSemaphoreGive(s_bus.subscriber_mutex);
    
    // 自动释放数据
    if (event->auto_free && event->data != NULL) {
        free(event->data);
    }
}

static void dispatcher_task(void *arg)
{
    xn_event_t event;
    
    ESP_LOGI(TAG, "Dispatcher task started");
    
    while (1) {
        if (xQueueReceive(s_bus.event_queue, &event, portMAX_DELAY) == pdTRUE) {
            dispatch_event(&event);
        }
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t xn_event_bus_init(void)
{
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
    
    s_bus.subscribers = NULL;
    s_bus.stats_published = 0;
    s_bus.stats_delivered = 0;
    s_bus.stats_dropped = 0;
    s_bus.initialized = true;
    
    ESP_LOGI(TAG, "Event bus initialized (queue=%d, max_subs=%d)", 
             XN_EVENT_QUEUE_SIZE, XN_EVENT_MAX_SUBSCRIBERS);
    
    return ESP_OK;
}

esp_err_t xn_event_bus_deinit(void)
{
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 停止分发任务
    if (s_bus.dispatcher_task != NULL) {
        vTaskDelete(s_bus.dispatcher_task);
        s_bus.dispatcher_task = NULL;
    }
    
    // 清理订阅者
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    subscriber_node_t *node = s_bus.subscribers;
    while (node != NULL) {
        subscriber_node_t *next = node->next;
        free(node);
        node = next;
    }
    s_bus.subscribers = NULL;
    xSemaphoreGive(s_bus.subscriber_mutex);
    
    // 清理队列中的事件
    xn_event_t event;
    while (xQueueReceive(s_bus.event_queue, &event, 0) == pdTRUE) {
        if (event.auto_free && event.data != NULL) {
            free(event.data);
        }
    }
    
    vSemaphoreDelete(s_bus.subscriber_mutex);
    vQueueDelete(s_bus.event_queue);
    
    s_bus.initialized = false;
    ESP_LOGI(TAG, "Event bus deinitialized");
    
    return ESP_OK;
}

esp_err_t xn_event_publish(const xn_event_t *event)
{
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xn_event_t evt_copy = *event;
    if (evt_copy.timestamp == 0) {
        evt_copy.timestamp = get_timestamp_ms();
    }
    
    if (xQueueSend(s_bus.event_queue, &evt_copy, 0) != pdTRUE) {
        s_bus.stats_dropped++;
        ESP_LOGW(TAG, "Event queue full, dropped event 0x%04x", event->id);
        return ESP_FAIL;
    }
    
    s_bus.stats_published++;
    ESP_LOGD(TAG, "Published event 0x%04x", event->id);
    
    return ESP_OK;
}

esp_err_t xn_event_publish_sync(const xn_event_t *event)
{
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xn_event_t evt_copy = *event;
    if (evt_copy.timestamp == 0) {
        evt_copy.timestamp = get_timestamp_ms();
    }
    
    s_bus.stats_published++;
    dispatch_event(&evt_copy);
    
    return ESP_OK;
}

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

esp_err_t xn_event_post_data(uint16_t event_id, uint16_t source, 
                              const void *data, size_t len)
{
    if (data == NULL || len == 0) {
        return xn_event_post(event_id, source);
    }
    
    void *data_copy = malloc(len);
    if (data_copy == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memcpy(data_copy, data, len);
    
    xn_event_t event = {
        .id = event_id,
        .source = source,
        .timestamp = get_timestamp_ms(),
        .data = data_copy,
        .data_len = len,
        .auto_free = true,
    };
    
    esp_err_t ret = xn_event_publish(&event);
    if (ret != ESP_OK) {
        free(data_copy);
    }
    return ret;
}

esp_err_t xn_event_subscribe(uint16_t event_id, xn_event_handler_t handler, 
                              void *user_data)
{
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    subscriber_node_t *new_node = malloc(sizeof(subscriber_node_t));
    if (new_node == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    new_node->event_id = event_id;
    new_node->handler = handler;
    new_node->user_data = user_data;
    
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    new_node->next = s_bus.subscribers;
    s_bus.subscribers = new_node;
    xSemaphoreGive(s_bus.subscriber_mutex);
    
    ESP_LOGD(TAG, "Subscribed to event 0x%04x", event_id);
    
    return ESP_OK;
}

esp_err_t xn_event_unsubscribe(uint16_t event_id, xn_event_handler_t handler)
{
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    
    subscriber_node_t *prev = NULL;
    subscriber_node_t *curr = s_bus.subscribers;
    
    while (curr != NULL) {
        if (curr->event_id == event_id && curr->handler == handler) {
            if (prev == NULL) {
                s_bus.subscribers = curr->next;
            } else {
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

esp_err_t xn_event_unsubscribe_all(xn_event_handler_t handler)
{
    if (!s_bus.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreTake(s_bus.subscriber_mutex, portMAX_DELAY);
    
    subscriber_node_t *prev = NULL;
    subscriber_node_t *curr = s_bus.subscribers;
    
    while (curr != NULL) {
        if (curr->handler == handler) {
            subscriber_node_t *to_free = curr;
            if (prev == NULL) {
                s_bus.subscribers = curr->next;
                curr = s_bus.subscribers;
            } else {
                prev->next = curr->next;
                curr = curr->next;
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

uint32_t xn_event_pending_count(void)
{
    if (!s_bus.initialized) {
        return 0;
    }
    return uxQueueMessagesWaiting(s_bus.event_queue);
}
