/**
 * @file app_state_machine.h
 * @brief 应用顶层状态机 - 管理系统整体状态和流程
 */

#ifndef APP_STATE_MACHINE_H
#define APP_STATE_MACHINE_H

#include "esp_err.h"
#include "xn_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 *                          状态定义
 *===========================================================================*/

typedef enum {
    APP_STATE_INIT = 0,         ///< 初始化中
    APP_STATE_WIFI_CONNECTING,  ///< WiFi连接中
    APP_STATE_WIFI_CONNECTED,   ///< WiFi已连接，等待IP
    APP_STATE_MQTT_CONNECTING,  ///< MQTT连接中
    APP_STATE_READY,            ///< 系统就绪
    APP_STATE_BLUFI_CONFIG,     ///< BluFi配网模式
    APP_STATE_ERROR,            ///< 错误状态
    APP_STATE_MAX,
} app_state_t;

/*===========================================================================
 *                          API
 *===========================================================================*/

/**
 * @brief 初始化应用状态机
 */
esp_err_t app_state_machine_init(void);

/**
 * @brief 启动应用状态机
 */
esp_err_t app_state_machine_start(void);

/**
 * @brief 停止应用状态机
 */
esp_err_t app_state_machine_stop(void);

/**
 * @brief 获取当前状态
 */
app_state_t app_state_machine_get_state(void);

/**
 * @brief 获取当前状态名称
 */
const char *app_state_machine_get_state_name(void);

/**
 * @brief 进入BluFi配网模式
 */
esp_err_t app_state_machine_enter_blufi(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_MACHINE_H */
