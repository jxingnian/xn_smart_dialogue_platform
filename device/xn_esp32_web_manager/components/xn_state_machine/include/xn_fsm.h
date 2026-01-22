/**
 * @file xn_fsm.h
 * @brief 通用有限状态机框架 - 可移植组件
 */

#ifndef XN_FSM_H
#define XN_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 *                          配置宏
 *===========================================================================*/

#ifndef XN_FSM_MAX_STATES
#define XN_FSM_MAX_STATES       16
#endif

#ifndef XN_FSM_MAX_TRANSITIONS
#define XN_FSM_MAX_TRANSITIONS  32
#endif

#ifndef XN_FSM_NAME_LEN
#define XN_FSM_NAME_LEN         16
#endif

/*===========================================================================
 *                          数据类型
 *===========================================================================*/

typedef uint16_t xn_state_id_t;
typedef uint16_t xn_event_id_t;

#define XN_STATE_INVALID    ((xn_state_id_t)0xFFFF)
#define XN_STATE_ANY        ((xn_state_id_t)0xFFFE)

struct xn_fsm;

/**
 * @brief 状态回调函数类型
 */
typedef void (*xn_fsm_state_cb_t)(struct xn_fsm *fsm, void *user_data);

/**
 * @brief 转换条件函数类型
 * @return true 允许转换，false 拒绝
 */
typedef bool (*xn_fsm_guard_t)(struct xn_fsm *fsm, xn_event_id_t event, void *user_data);

/**
 * @brief 转换动作函数类型
 */
typedef void (*xn_fsm_action_t)(struct xn_fsm *fsm, xn_event_id_t event, void *user_data);

/**
 * @brief 状态定义
 */
typedef struct {
    xn_state_id_t id;
    const char *name;
    xn_fsm_state_cb_t on_enter;     ///< 进入状态回调
    xn_fsm_state_cb_t on_exit;      ///< 退出状态回调
    xn_fsm_state_cb_t on_run;       ///< 状态运行回调（可选）
} xn_fsm_state_t;

/**
 * @brief 转换定义
 */
typedef struct {
    xn_state_id_t from;             ///< 源状态(XN_STATE_ANY匹配任意)
    xn_event_id_t event;            ///< 触发事件
    xn_state_id_t to;               ///< 目标状态
    xn_fsm_guard_t guard;           ///< 条件函数(可选)
    xn_fsm_action_t action;         ///< 转换动作(可选)
} xn_fsm_transition_t;

/**
 * @brief 状态机实例
 */
typedef struct xn_fsm {
    const char *name;
    xn_state_id_t current_state;
    xn_state_id_t prev_state;
    const xn_fsm_state_t *states;
    uint8_t state_count;
    const xn_fsm_transition_t *transitions;
    uint8_t transition_count;
    void *user_data;
    bool running;
} xn_fsm_t;

/**
 * @brief 状态机配置
 */
typedef struct {
    const char *name;
    xn_state_id_t initial_state;
    const xn_fsm_state_t *states;
    uint8_t state_count;
    const xn_fsm_transition_t *transitions;
    uint8_t transition_count;
    void *user_data;
} xn_fsm_config_t;

/*===========================================================================
 *                          API
 *===========================================================================*/

/**
 * @brief 初始化状态机
 */
esp_err_t xn_fsm_init(xn_fsm_t *fsm, const xn_fsm_config_t *config);

/**
 * @brief 启动状态机（进入初始状态）
 */
esp_err_t xn_fsm_start(xn_fsm_t *fsm);

/**
 * @brief 停止状态机
 */
esp_err_t xn_fsm_stop(xn_fsm_t *fsm);

/**
 * @brief 处理事件
 * @return ESP_OK 发生转换, ESP_ERR_NOT_FOUND 无匹配转换
 */
esp_err_t xn_fsm_process_event(xn_fsm_t *fsm, xn_event_id_t event);

/**
 * @brief 强制转换到指定状态
 */
esp_err_t xn_fsm_set_state(xn_fsm_t *fsm, xn_state_id_t state);

/**
 * @brief 运行当前状态的on_run回调
 */
void xn_fsm_run(xn_fsm_t *fsm);

/**
 * @brief 获取当前状态ID
 */
xn_state_id_t xn_fsm_get_state(const xn_fsm_t *fsm);

/**
 * @brief 获取当前状态名称
 */
const char *xn_fsm_get_state_name(const xn_fsm_t *fsm);

/**
 * @brief 检查是否处于指定状态
 */
bool xn_fsm_is_in_state(const xn_fsm_t *fsm, xn_state_id_t state);

#ifdef __cplusplus
}
#endif

#endif /* XN_FSM_H */
