/**
 * @file xn_event_bus.h
 * @brief 事件总线核心API - 通用可移植组件
 * 
 * 此组件提供了一个轻量级的发布-订阅（Pub/Sub）事件机制。
 * 支持同步和异步事件分发，支持带数据和不带数据的事件。
 * 核心功能包括：
 * - 初始化和反初始化事件总线
 * - 发布事件（Publish）
 * - 订阅事件（Subscribe）
 * - 取消订阅（Unsubscribe）
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
#define XN_EVENT_QUEUE_SIZE         32  ///< 事件队列深度，决定可以缓冲多少个未处理事件
#endif

#ifndef XN_EVENT_MAX_SUBSCRIBERS
#define XN_EVENT_MAX_SUBSCRIBERS    16  ///< 最大订阅者数量限制（注意：当前实现使用链表，此宏可能未实际限制链表长度，取决于具体实现）
#endif

#ifndef XN_EVENT_TASK_STACK_SIZE
#define XN_EVENT_TASK_STACK_SIZE    4096 ///< 事件分发任务的堆栈大小(字节)
#endif

#ifndef XN_EVENT_TASK_PRIORITY
#define XN_EVENT_TASK_PRIORITY      5    ///< 事件分发任务的优先级
#endif

/*===========================================================================
 *                          数据类型
 *===========================================================================*/

/**
 * @brief 事件结构体
 * 
 * 包含事件的所有必要信息，用于在总线上传递。
 */
typedef struct {
    uint16_t id;            ///< 事件ID，见 xn_event_types.h 定义
    uint16_t source;        ///< 事件源，标识事件产生者
    uint32_t timestamp;     ///< 时间戳(ms)，事件产生的时间
    void *data;             ///< 数据指针，指向携带的负载数据
    size_t data_len;        ///< 数据长度(字节)
    bool auto_free;         ///< 标志位：是否由事件总线自动释放data内存
} xn_event_t;

/**
 * @brief 事件处理回调函数原型
 * 
 * @param event 指向接收到的事件结构体
 * @param user_data 注册时传递的用户私有数据
 */
typedef void (*xn_event_handler_t)(const xn_event_t *event, void *user_data);

/*===========================================================================
 *                          核心API
 *===========================================================================*/

/**
 * @brief 初始化事件总线
 * 
 * 创建事件队列、互斥锁和分发任务。
 * 必须在任何其他事件操作之前调用。
 * 
 * @return esp_err_t 
 *      - ESP_OK: 初始化成功
 *      - ESP_ERR_INVALID_STATE: 已经初始化
 *      - ESP_ERR_NO_MEM: 内存不足
 */
esp_err_t xn_event_bus_init(void);

/**
 * @brief 反初始化事件总线
 * 
 * 停止分发任务，释放队列、互斥锁和所有订阅者资源。
 * 
 * @return esp_err_t 
 *      - ESP_OK: 反初始化成功
 *      - ESP_ERR_INVALID_STATE: 未初始化
 */
esp_err_t xn_event_bus_deinit(void);

/**
 * @brief 发布事件（异步）
 * 
 * 将事件放入队列，由后台任务异步分发给订阅者。
 * 如果队列已满，此函数可能会失败或阻塞（取决于实现）。
 * 
 * @param event 指向要发布的事件结构体
 * @return esp_err_t 
 *      - ESP_OK: 发布成功入队
 *      - ESP_ERR_INVALID_STATE: 总线未初始化
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - ESP_FAIL: 队列满
 */
esp_err_t xn_event_publish(const xn_event_t *event);

/**
 * @brief 发布事件（同步）
 * 
 * 直接在当前线程上下文中分发事件给所有订阅者，不经过队列。
 * 注意：这会阻塞当前线程直到所有处理程序执行完毕。
 * 
 * @param event 指向要发布的事件结构体
 * @return esp_err_t 
 *      - ESP_OK: 分发成功
 *      - ESP_ERR_INVALID_STATE: 总线未初始化
 *      - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t xn_event_publish_sync(const xn_event_t *event);

/**
 * @brief 快速发布无数据事件
 * 
 * 辅助函数，构建并发布一个不带数据的简单事件。
 * 
 * @param event_id 事件ID
 * @param source 事件源
 * @return esp_err_t 见 xn_event_publish 返回值
 */
esp_err_t xn_event_post(uint16_t event_id, uint16_t source);

/**
 * @brief 快速发布带数据事件（自动拷贝）
 * 
 * 辅助函数，会分配新内存并拷贝数据，设置 auto_free=true。
 * 
 * @param event_id 事件ID
 * @param source 事件源
 * @param data 数据指针
 * @param len 数据长度
 * @return esp_err_t 见 xn_event_publish 返回值
 */
esp_err_t xn_event_post_data(uint16_t event_id, uint16_t source, 
                              const void *data, size_t len);

/**
 * @brief 订阅事件
 * 
 * 注册一个回调函数来接收特定ID的事件。
 * 
 * @param event_id 要订阅的事件ID，使用 XN_EVT_ANY 订阅所有事件
 * @param handler 回调函数
 * @param user_data 传递给回调的用户数据
 * @return esp_err_t 
 *      - ESP_OK: 订阅成功
 *      - ESP_ERR_INVALID_STATE: 总线未初始化
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - ESP_ERR_NO_MEM: 内存不足
 */
esp_err_t xn_event_subscribe(uint16_t event_id, xn_event_handler_t handler, 
                              void *user_data);

/**
 * @brief 取消订阅
 * 
 * 移除特定的订阅记录。
 * 
 * @param event_id 事件ID
 * @param handler 回调函数
 * @return esp_err_t 
 *      - ESP_OK: 取消成功
 *      - ESP_ERR_NOT_FOUND: 未找到该订阅
 */
esp_err_t xn_event_unsubscribe(uint16_t event_id, xn_event_handler_t handler);

/**
 * @brief 取消handler的所有订阅
 * 
 * 移除指定handler注册的所有事件订阅。
 * 
 * @param handler 回调函数
 * @return esp_err_t 
 *      - ESP_OK: 操作成功
 */
esp_err_t xn_event_unsubscribe_all(xn_event_handler_t handler);

/**
 * @brief 获取待处理事件数量
 * 
 * @return uint32_t 队列中等待分发的事件数量
 */
uint32_t xn_event_pending_count(void);

#ifdef __cplusplus
}
#endif

#endif /* XN_EVENT_BUS_H */
