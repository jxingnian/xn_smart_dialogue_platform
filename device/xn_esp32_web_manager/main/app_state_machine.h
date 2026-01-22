/**
 * @file app_state_machine.h
 * @brief 应用顶层状态机 - 管理系统整体状态和流程
 * 
 * 顶层状态机负责协调各个子Manager（WiFi, MQTT, BluFi）的工作顺序。
 * 它定义了系统的高级状态（如WIFI连接中，MQTT连接中，系统就绪等），
 * 并根据底层事件（如WiFi已连接，MQTT断开等）进行状态流转。
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

/**
 * @brief 应用程序状态枚举
 */
typedef enum {
    APP_STATE_INIT = 0,         ///< 初始化中：系统启动，各种初始化
    APP_STATE_WIFI_CONNECTING,  ///< WiFi连接中：尝试连接WiFi AP
    APP_STATE_WIFI_CONNECTED,   ///< WiFi已连接：物理层/链路层已连上，等待DHCP获取IP
    APP_STATE_MQTT_CONNECTING,  ///< MQTT连接中：已获取IP，正在尝试连接MQTT Broker
    APP_STATE_READY,            ///< 系统就绪：MQTT已连接，可以正常收发业务数据
    APP_STATE_BLUFI_CONFIG,     ///< BluFi配网模式：启动蓝牙配网服务，等待用户配置WiFi信息
    APP_STATE_ERROR,            ///< 错误状态：系统遇到严重错误
    APP_STATE_MAX,              ///< 状态最大值（辅助计数）
} app_state_t;

/*===========================================================================
 *                          API
 *===========================================================================*/

/**
 * @brief 初始化应用状态机
 * 
 * - 创建 FSM 实例
 * - 配置状态表和转换表
 * - 注册事件总线监听器，监听所有系统事件
 * 
 * @return esp_err_t 初始化结果
 */
esp_err_t app_state_machine_init(void);

/**
 * @brief 启动应用状态机
 * 
 * - 状态机进入初始状态(APP_STATE_INIT)
 * - 触发初始状态的 on_enter 回调
 * 
 * @return esp_err_t 启动结果
 */
esp_err_t app_state_machine_start(void);

/**
 * @brief 停止应用状态机
 * 
 * - 停止 FSM 运行
 * - 取消订阅事件总线
 * 
 * @return esp_err_t 停止结果
 */
esp_err_t app_state_machine_stop(void);

/**
 * @brief 获取当前应用状态
 * 
 * @return app_state_t 当前状态枚举值
 */
app_state_t app_state_machine_get_state(void);

/**
 * @brief 获取当前状态名称字符串
 * 
 * @return const char* 状态名称（用于日志打印）
 */
const char *app_state_machine_get_state_name(void);

/**
 * @brief 请求进入BluFi配网模式
 * 
 * 发送一条命令事件(XN_CMD_BLUFI_START)，触发状态机跳转到 APP_STATE_BLUFI_CONFIG 状态。
 * 
 * @return esp_err_t 请求发送结果
 */
esp_err_t app_state_machine_enter_blufi(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_MACHINE_H */
