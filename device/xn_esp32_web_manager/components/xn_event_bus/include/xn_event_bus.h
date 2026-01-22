/**
 * @file xn_event_bus.h
 * @brief 事件总线核心API - 通用可移植组件
 */

#ifndef XN_EVENT_BUS_H
#define XN_EVENT_BUS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "xn_event_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 *                          配置宏
 *===========================================================================*/

#ifndef XN_EVENT_QUEUE_SIZE
#define XN_EVENT_QUEUE_SIZE         32
#endif

#ifndef XN_EVENT_MAX_SUBSCRIBERS
#define XN_EVENT_MAX_SUBSCRIBERS    16
#endif

#ifndef XN_EVENT_TASK_STACK_SIZE
#define XN_EVENT_TASK_STACK_SIZE    4096
#endif

#ifndef XN_EVENT_TASK_PRIORITY
#define XN_EVENT_TASK_PRIORITY      5
#endif

/*===========================================================================
 *                          数据类型
 *===========================================================================*/

/**
 * @brief 事件结构体
 */
typedef struct {
    uint16_t id;            ///< 事件ID
    uint16_t source;        ///< 事件源
    uint32_t timestamp;     ///< 时间戳(ms)
    void *data;             ///< 数据指针
    size_t data_len;        ///< 数据长度
    bool auto_free;         ///< 是否自动释放data
} xn_event_t;

/**
 * @brief 事件处理回调
 */
typedef void (*xn_event_handler_t)(const xn_event_t *event, void *user_data);

/*===========================================================================
 *                          核心API
 *===========================================================================*/

/**
 * @brief 初始化事件总线
 */
esp_err_t xn_event_bus_init(void);

/**
 * @brief 反初始化事件总线
 */
esp_err_t xn_event_bus_deinit(void);

/**
 * @brief 发布事件（异步）
 */
esp_err_t xn_event_publish(const xn_event_t *event);

/**
 * @brief 发布事件（同步）
 */
esp_err_t xn_event_publish_sync(const xn_event_t *event);

/**
 * @brief 快速发布无数据事件
 */
esp_err_t xn_event_post(uint16_t event_id, uint16_t source);

/**
 * @brief 快速发布带数据事件（自动拷贝）
 */
esp_err_t xn_event_post_data(uint16_t event_id, uint16_t source, 
                              const void *data, size_t len);

/**
 * @brief 订阅事件
 */
esp_err_t xn_event_subscribe(uint16_t event_id, xn_event_handler_t handler, 
                              void *user_data);

/**
 * @brief 取消订阅
 */
esp_err_t xn_event_unsubscribe(uint16_t event_id, xn_event_handler_t handler);

/**
 * @brief 取消handler的所有订阅
 */
esp_err_t xn_event_unsubscribe_all(xn_event_handler_t handler);

/**
 * @brief 获取待处理事件数量
 */
uint32_t xn_event_pending_count(void);

#ifdef __cplusplus
}
#endif

#endif /* XN_EVENT_BUS_H */
