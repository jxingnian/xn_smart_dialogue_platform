/**
 * @file xn_fsm.h
 * @brief 通用有限状态机框架 - 可移植组件
 * 
 * 本组件提供了一个基于表驱动的有限状态机(FSM)实现。
 * 特点：
 * - 纯C实现，无操作系统依赖（但在多线程环境需注意同步）
 * - 支持状态入口(on_enter)、出口(on_exit)和运行(on_run)回调
 * - 支持带条件的转换(Guard)
 * - 支持转换动作(Action)
 * - 调试友好的状态名称支持
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
#define XN_FSM_MAX_STATES       16      ///< 支持的最大状态数（用于可能的静态分配场景，当前实现主要用于参考）
#endif

#ifndef XN_FSM_MAX_TRANSITIONS
#define XN_FSM_MAX_TRANSITIONS  32      ///< 支持的最大转换规则数
#endif

#ifndef XN_FSM_NAME_LEN
#define XN_FSM_NAME_LEN         16      ///< 状态机名称最大长度
#endif

/*===========================================================================
 *                          数据类型
 *===========================================================================*/

typedef uint16_t xn_state_id_t;         ///< 状态ID类型
typedef uint16_t xn_event_id_t;         ///< 事件ID类型

#define XN_STATE_INVALID    ((xn_state_id_t)0xFFFF) ///< 无效状态ID
#define XN_STATE_ANY        ((xn_state_id_t)0xFFFE) ///< 通配符状态ID，匹配任意状态

struct xn_fsm; // 前向声明

/**
 * @brief 状态回调函数类型
 * 
 * 用于状态的 entry/exit/run 动作。
 * @param fsm 状态机实例指针
 * @param user_data 用户私有数据
 */
typedef void (*xn_fsm_state_cb_t)(struct xn_fsm *fsm, void *user_data);

/**
 * @brief 转换条件函数类型(Guard)
 * 
 * 在发生事件时，决定是否允许转换发生。
 * @param fsm 状态机实例指针
 * @param event 触发的事件ID
 * @param user_data 用户私有数据
 * @return true 允许转换，false 拒绝转换
 */
typedef bool (*xn_fsm_guard_t)(struct xn_fsm *fsm, xn_event_id_t event, void *user_data);

/**
 * @brief 转换动作函数类型(Action)
 * 
 * 在状态转换过程中执行的动作（在源状态exit之后，目标状态enter之前执行）。
 * @param fsm 状态机实例指针
 * @param event 触发的事件ID
 * @param user_data 用户私有数据
 */
typedef void (*xn_fsm_action_t)(struct xn_fsm *fsm, xn_event_id_t event, void *user_data);

/**
 * @brief 状态定义结构体
 */
typedef struct {
    xn_state_id_t id;               ///< 状态ID（必须唯一）
    const char *name;               ///< 状态名称（用于调试）
    xn_fsm_state_cb_t on_enter;     ///< 进入状态回调（可选）
    xn_fsm_state_cb_t on_exit;      ///< 退出状态回调（可选）
    xn_fsm_state_cb_t on_run;       ///< 状态运行回调（可选，通常用于轮询）
} xn_fsm_state_t;

/**
 * @brief 转换规则定义结构体
 * 
 * 表示：当在From状态收到Event且Guard为真时，执行Action并跳转到To状态。
 */
typedef struct {
    xn_state_id_t from;             ///< 源状态(XN_STATE_ANY匹配任意)
    xn_event_id_t event;            ///< 触发事件
    xn_state_id_t to;               ///< 目标状态
    xn_fsm_guard_t guard;           ///< 条件函数(可选)
    xn_fsm_action_t action;         ///< 转换动作(可选)
} xn_fsm_transition_t;

/**
 * @brief 状态机实例结构体
 * 
 * 存储状态机的运行时状态。
 */
typedef struct xn_fsm {
    const char *name;                       ///< 状态机名称
    xn_state_id_t current_state;            ///< 当前状态ID
    xn_state_id_t prev_state;               ///< 上一个状态ID
    const xn_fsm_state_t *states;           ///< 状态表指针
    uint8_t state_count;                    ///< 状态数量
    const xn_fsm_transition_t *transitions; ///< 转换表指针
    uint8_t transition_count;               ///< 转换数量
    void *user_data;                        ///< 用户私有数据指针
    bool running;                           ///< 运行标志
} xn_fsm_t;

/**
 * @brief 状态机初始化配置结构体
 */
typedef struct {
    const char *name;                       ///< 状态机名称
    xn_state_id_t initial_state;            ///< 初始状态ID
    const xn_fsm_state_t *states;           ///< 状态表
    uint8_t state_count;                    ///< 状态表大小
    const xn_fsm_transition_t *transitions; ///< 转换表
    uint8_t transition_count;               ///< 转换表大小
    void *user_data;                        ///< 用户私有数据
} xn_fsm_config_t;

/*===========================================================================
 *                          API
 *===========================================================================*/

/**
 * @brief 初始化状态机
 * 
 * 加载配置，但不启动状态机（不进入初始状态）。
 * 
 * @param fsm 状态机实例指针
 * @param config 配置结构体指针
 * @return esp_err_t ESP_OK 或 错误码
 */
esp_err_t xn_fsm_init(xn_fsm_t *fsm, const xn_fsm_config_t *config);

/**
 * @brief 启动状态机
 * 
 * 设置运行标志，并强制进入初始状态（触发初始状态的 on_enter）。
 * 
 * @param fsm 状态机实例指针
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_ERR_INVALID_STATE: 已经在运行
 */
esp_err_t xn_fsm_start(xn_fsm_t *fsm);

/**
 * @brief 停止状态机
 * 
 * 触发当前状态的 on_exit，并清除运行标志。
 * 
 * @param fsm 状态机实例指针
 * @return esp_err_t 
 */
esp_err_t xn_fsm_stop(xn_fsm_t *fsm);

/**
 * @brief 处理事件
 * 
 * 查找匹配的转换规则并执行转换。
 * 
 * @param fsm 状态机实例指针
 * @param event 事件ID
 * @return esp_err_t 
 *      - ESP_OK: 转换成功
 *      - ESP_ERR_NOT_FOUND: 无匹配转换（忽略）
 *      - ESP_ERR_NOT_ALLOWED: Guard条件拒绝
 *      - ESP_ERR_INVALID_STATE: 状态机未运行
 */
esp_err_t xn_fsm_process_event(xn_fsm_t *fsm, xn_event_id_t event);

/**
 * @brief 强制转换到指定状态
 * 
 * 手动切换状态，会触发原状态 on_exit 和新状态 on_enter。
 * 通常不建议直接使用，应优先通过事件驱动。
 * 
 * @param fsm 状态机实例指针
 * @param state 目标状态ID
 * @return esp_err_t 
 */
esp_err_t xn_fsm_set_state(xn_fsm_t *fsm, xn_state_id_t state);

/**
 * @brief 运行当前状态的on_run回调
 * 
 * 应在主循环中周期性调用。
 * 
 * @param fsm 状态机实例指针
 */
void xn_fsm_run(xn_fsm_t *fsm);

/**
 * @brief 获取当前状态ID
 * 
 * @param fsm 状态机实例指针
 * @return xn_state_id_t 当前状态ID，未运行时可能返回 XN_STATE_INVALID
 */
xn_state_id_t xn_fsm_get_state(const xn_fsm_t *fsm);

/**
 * @brief 获取当前状态名称
 * 
 * @param fsm 状态机实例指针
 * @return const char* 状态名称字符串
 */
const char *xn_fsm_get_state_name(const xn_fsm_t *fsm);

/**
 * @brief 检查是否处于指定状态
 * 
 * @param fsm 状态机实例指针
 * @param state 待检查的状态ID
 * @return true 处于该状态，false 不处于
 */
bool xn_fsm_is_in_state(const xn_fsm_t *fsm, xn_state_id_t state);

#ifdef __cplusplus
}
#endif

#endif /* XN_FSM_H */
