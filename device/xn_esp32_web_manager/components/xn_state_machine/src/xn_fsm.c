/**
 * @file xn_fsm.c
 * @brief 通用有限状态机实现
 */

#include <string.h>
#include "esp_log.h"
#include "xn_fsm.h"

static const char *TAG = "xn_fsm";

/*===========================================================================
 *                          内部函数
 *===========================================================================*/

/**
 * @brief 根据ID查找状态定义
 */
static const xn_fsm_state_t *find_state(const xn_fsm_t *fsm, xn_state_id_t state_id)
{
    // 线性遍历查找状态
    for (int i = 0; i < fsm->state_count; i++) {
        if (fsm->states[i].id == state_id) {
            return &fsm->states[i];
        }
    }
    return NULL; // 未找到
}

/**
 * @brief 查找匹配的转换规则
 * 
 * 查找触发事件为event，且源状态匹配当前状态（或为ANY）的规则。
 * 如果有多条匹配，返回表中最先出现的一条。
 */
static const xn_fsm_transition_t *find_transition(const xn_fsm_t *fsm, xn_event_id_t event)
{
    for (int i = 0; i < fsm->transition_count; i++) {
        const xn_fsm_transition_t *t = &fsm->transitions[i];
        // 匹配事件
        if (t->event == event) {
            // 匹配源状态 (支持通配 XN_STATE_ANY)
            if (t->from == XN_STATE_ANY || t->from == fsm->current_state) {
                return t;
            }
        }
    }
    return NULL;
}

/**
 * @brief 执行状态转换
 * 
 * 流程：
 * 1. 查找源状态和目标状态定义
 * 2. 如果存在，调用源状态的 on_exit
 * 3. 如果存在，执行转换动作(action)
 * 4. 更新当前状态ID
 * 5. 如果存在，调用目标状态的 on_enter
 */
static void do_transition(xn_fsm_t *fsm, const xn_fsm_transition_t *trans, xn_event_id_t event)
{
    const xn_fsm_state_t *from_state = find_state(fsm, fsm->current_state);
    const xn_fsm_state_t *to_state = find_state(fsm, trans->to);
    
    // 目标状态必须存在
    if (to_state == NULL) {
        ESP_LOGE(TAG, "[%s] Invalid target state: %d", fsm->name, trans->to);
        return;
    }
    
    ESP_LOGI(TAG, "[%s] %s -> %s (event=0x%04x)", 
             fsm->name,
             from_state ? from_state->name : "?",
             to_state->name,
             event);
    
    // 退出当前状态(前)
    if (from_state && from_state->on_exit) {
        from_state->on_exit(fsm, fsm->user_data);
    }
    
    // 执行转换动作(中)
    if (trans->action) {
        trans->action(fsm, event, fsm->user_data);
    }
    
    // 更新状态
    fsm->prev_state = fsm->current_state;
    fsm->current_state = trans->to;
    
    // 进入新状态(后)
    if (to_state->on_enter) {
        to_state->on_enter(fsm, fsm->user_data);
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

/* 初始化状态机 */
esp_err_t xn_fsm_init(xn_fsm_t *fsm, const xn_fsm_config_t *config)
{
    if (fsm == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->states == NULL || config->state_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 指针复位
    memset(fsm, 0, sizeof(xn_fsm_t));
    
    // 加载配置
    fsm->name = config->name ? config->name : "fsm";
    fsm->states = config->states;
    fsm->state_count = config->state_count;
    fsm->transitions = config->transitions;
    fsm->transition_count = config->transition_count;
    fsm->user_data = config->user_data;
    
    // 设置初始状态，但暂不进入
    fsm->current_state = config->initial_state;
    fsm->prev_state = XN_STATE_INVALID;
    fsm->running = false;
    
    ESP_LOGI(TAG, "[%s] Initialized with %d states, %d transitions", 
             fsm->name, fsm->state_count, fsm->transition_count);
    
    return ESP_OK;
}

/* 启动状态机 */
esp_err_t xn_fsm_start(xn_fsm_t *fsm)
{
    if (fsm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (fsm->running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 验证初始状态是否存在
    const xn_fsm_state_t *initial = find_state(fsm, fsm->current_state);
    if (initial == NULL) {
        ESP_LOGE(TAG, "[%s] Invalid initial state: %d", fsm->name, fsm->current_state);
        return ESP_ERR_INVALID_ARG;
    }
    
    fsm->running = true;
    
    ESP_LOGI(TAG, "[%s] Started in state: %s", fsm->name, initial->name);
    
    // 触发初始状态的进入动作
    if (initial->on_enter) {
        initial->on_enter(fsm, fsm->user_data);
    }
    
    return ESP_OK;
}

/* 停止状态机 */
esp_err_t xn_fsm_stop(xn_fsm_t *fsm)
{
    if (fsm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!fsm->running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 退出当前状态
    const xn_fsm_state_t *current = find_state(fsm, fsm->current_state);
    if (current && current->on_exit) {
        current->on_exit(fsm, fsm->user_data);
    }
    
    fsm->running = false;
    
    ESP_LOGI(TAG, "[%s] Stopped", fsm->name);
    
    return ESP_OK;
}

/* 处理事件 */
esp_err_t xn_fsm_process_event(xn_fsm_t *fsm, xn_event_id_t event)
{
    if (fsm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!fsm->running) {
        ESP_LOGW(TAG, "[%s] Event processing rejected: FSM not running", fsm->name);
        return ESP_ERR_INVALID_STATE;
    }
    
    // 查找转换规则
    const xn_fsm_transition_t *trans = find_transition(fsm, event);
    if (trans == NULL) {
        ESP_LOGD(TAG, "[%s] No transition for event 0x%04x in state %d", 
                 fsm->name, event, fsm->current_state);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 检查 Guard 条件
    if (trans->guard && !trans->guard(fsm, event, fsm->user_data)) {
        ESP_LOGD(TAG, "[%s] Transition guard rejected event 0x%04x", 
                 fsm->name, event);
        return ESP_ERR_NOT_ALLOWED;
    }
    
    // 执行转换
    do_transition(fsm, trans, event);
    
    return ESP_OK;
}

/* 强制设置状态 */
esp_err_t xn_fsm_set_state(xn_fsm_t *fsm, xn_state_id_t state)
{
    if (fsm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const xn_fsm_state_t *to_state = find_state(fsm, state);
    if (to_state == NULL) {
        ESP_LOGE(TAG, "[%s] Invalid target state: %d", fsm->name, state);
        return ESP_ERR_INVALID_ARG;
    }
    
    const xn_fsm_state_t *from_state = find_state(fsm, fsm->current_state);
    
    ESP_LOGI(TAG, "[%s] Force state: %s -> %s", 
             fsm->name,
             from_state ? from_state->name : "?",
             to_state->name);
    
    // 如果正在运行，需要完整执行状态切换的 exit/enter
    if (fsm->running && from_state && from_state->on_exit) {
        from_state->on_exit(fsm, fsm->user_data);
    }
    
    fsm->prev_state = fsm->current_state;
    fsm->current_state = state;
    
    if (fsm->running && to_state->on_enter) {
        to_state->on_enter(fsm, fsm->user_data);
    }
    
    return ESP_OK;
}

/* 运行当前状态逻辑 */
void xn_fsm_run(xn_fsm_t *fsm)
{
    if (fsm == NULL || !fsm->running) {
        return;
    }
    
    const xn_fsm_state_t *current = find_state(fsm, fsm->current_state);
    if (current && current->on_run) {
        current->on_run(fsm, fsm->user_data);
    }
}

/* 获取当前状态 */
xn_state_id_t xn_fsm_get_state(const xn_fsm_t *fsm)
{
    if (fsm == NULL) {
        return XN_STATE_INVALID;
    }
    return fsm->current_state;
}

/* 获取当前状态名称 */
const char *xn_fsm_get_state_name(const xn_fsm_t *fsm)
{
    if (fsm == NULL) {
        return "NULL";
    }
    
    const xn_fsm_state_t *state = find_state(fsm, fsm->current_state);
    return state ? state->name : "UNKNOWN";
}

/* 状态判断 */
bool xn_fsm_is_in_state(const xn_fsm_t *fsm, xn_state_id_t state)
{
    if (fsm == NULL) {
        return false;
    }
    return fsm->current_state == state;
}
