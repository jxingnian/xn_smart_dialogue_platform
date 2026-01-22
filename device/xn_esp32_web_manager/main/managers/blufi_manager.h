/**
 * @file blufi_manager.h
 * @brief BluFi配网应用管理器 - 通过事件与其他模块通信
 */

#ifndef BLUFI_MANAGER_H
#define BLUFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化BluFi管理器
 */
esp_err_t blufi_manager_init(void);

/**
 * @brief 反初始化BluFi管理器
 */
esp_err_t blufi_manager_deinit(void);

/**
 * @brief 启动BluFi配网
 */
esp_err_t blufi_manager_start(void);

/**
 * @brief 停止BluFi配网
 */
esp_err_t blufi_manager_stop(void);

/**
 * @brief 检查BluFi是否正在运行
 */
bool blufi_manager_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUFI_MANAGER_H */
