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

static const xn_fsm_state_t *find_state(const xn_fsm_t *fsm, xn_state_id_t state_id)
{
    for (int i = 0; i < fsm->state_count; i++) {
        if (fsm->states[i].id == state_id) {
            return &fsm->states[i];
        }
    }
    return NULL;
}

static const xn_fsm_transition_t *find_transition(const xn_fsm_t *fsm, xn_event_id_t event)
{
    for (int i = 0; i < fsm->transition_count; i++) {
        const xn_fsm_transition_t *t = &fsm->transitions[i];
        if (t->event == event) {
            if (t->from == XN_STATE_ANY || t->from == fsm->current_state) {
                return t;
            }
        }
    }
    return NULL;
}

static void do_transition(xn_fsm_t *fsm, const xn_fsm_transition_t *trans, xn_event_id_t event)
{
    const xn_fsm_state_t *from_state = find_state(fsm, fsm->current_state);
    const xn_fsm_state_t *to_state = find_state(fsm, trans->to);
    
    if (to_state == NULL) {
        ESP_LOGE(TAG, "[%s] Invalid target state: %d", fsm->name, trans->to);
        return;
    }
    
    ESP_LOGI(TAG, "[%s] %s -> %s (event=0x%04x)", 
             fsm->name,
             from_state ? from_state->name : "?",
             to_state->name,
             event);
    
    // 退出当前状态
    if (from_state && from_state->on_exit) {
        from_state->on_exit(fsm, fsm->user_data);
    }
    
    // 执行转换动作
    if (trans->action) {
        trans->action(fsm, event, fsm->user_data);
    }
    
    // 更新状态
    fsm->prev_state = fsm->current_state;
    fsm->current_state = trans->to;
    
    // 进入新状态
    if (to_state->on_enter) {
        to_state->on_enter(fsm, fsm->user_data);
    }
}

/*===========================================================================
 *                          公共API
 *===========================================================================*/

esp_err_t xn_fsm_init(xn_fsm_t *fsm, const xn_fsm_config_t *config)
{
    if (fsm == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->states == NULL || config->state_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(fsm, 0, sizeof(xn_fsm_t));
    
    fsm->name = config->name ? config->name : "fsm";
    fsm->states = config->states;
    fsm->state_count = config->state_count;
    fsm->transitions = config->transitions;
    fsm->transition_count = config->transition_count;
    fsm->user_data = config->user_data;
    fsm->current_state = config->initial_state;
    fsm->prev_state = XN_STATE_INVALID;
    fsm->running = false;
    
    ESP_LOGI(TAG, "[%s] Initialized with %d states, %d transitions", 
             fsm->name, fsm->state_count, fsm->transition_count);
    
    return ESP_OK;
}

esp_err_t xn_fsm_start(xn_fsm_t *fsm)
{
    if (fsm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (fsm->running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    const xn_fsm_state_t *initial = find_state(fsm, fsm->current_state);
    if (initial == NULL) {
        ESP_LOGE(TAG, "[%s] Invalid initial state: %d", fsm->name, fsm->current_state);
        return ESP_ERR_INVALID_ARG;
    }
    
    fsm->running = true;
    
    ESP_LOGI(TAG, "[%s] Started in state: %s", fsm->name, initial->name);
    
    if (initial->on_enter) {
        initial->on_enter(fsm, fsm->user_data);
    }
    
    return ESP_OK;
}

esp_err_t xn_fsm_stop(xn_fsm_t *fsm)
{
    if (fsm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!fsm->running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    const xn_fsm_state_t *current = find_state(fsm, fsm->current_state);
    if (current && current->on_exit) {
        current->on_exit(fsm, fsm->user_data);
    }
    
    fsm->running = false;
    
    ESP_LOGI(TAG, "[%s] Stopped", fsm->name);
    
    return ESP_OK;
}

esp_err_t xn_fsm_process_event(xn_fsm_t *fsm, xn_event_id_t event)
{
    if (fsm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!fsm->running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    const xn_fsm_transition_t *trans = find_transition(fsm, event);
    if (trans == NULL) {
        ESP_LOGD(TAG, "[%s] No transition for event 0x%04x in state %d", 
                 fsm->name, event, fsm->current_state);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 检查条件
    if (trans->guard && !trans->guard(fsm, event, fsm->user_data)) {
        ESP_LOGD(TAG, "[%s] Transition guard rejected event 0x%04x", 
                 fsm->name, event);
        return ESP_ERR_NOT_ALLOWED;
    }
    
    do_transition(fsm, trans, event);
    
    return ESP_OK;
}

esp_err_t xn_fsm_set_state(xn_fsm_t *fsm, xn_state_id_t state)
{
    if (fsm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const xn_fsm_state_t *to_state = find_state(fsm, state);
    if (to_state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const xn_fsm_state_t *from_state = find_state(fsm, fsm->current_state);
    
    ESP_LOGI(TAG, "[%s] Force state: %s -> %s", 
             fsm->name,
             from_state ? from_state->name : "?",
             to_state->name);
    
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

xn_state_id_t xn_fsm_get_state(const xn_fsm_t *fsm)
{
    if (fsm == NULL) {
        return XN_STATE_INVALID;
    }
    return fsm->current_state;
}

const char *xn_fsm_get_state_name(const xn_fsm_t *fsm)
{
    if (fsm == NULL) {
        return "NULL";
    }
    
    const xn_fsm_state_t *state = find_state(fsm, fsm->current_state);
    return state ? state->name : "UNKNOWN";
}

bool xn_fsm_is_in_state(const xn_fsm_t *fsm, xn_state_id_t state)
{
    if (fsm == NULL) {
        return false;
    }
    return fsm->current_state == state;
}
