/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24
 * @Description: OTA 管理器头文件 - 业务逻辑管理（策略层）
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include "xn_ota.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                              类型定义                                        */
/* ========================================================================== */

/**
 * @brief OTA 管理器状态枚举
 */
typedef enum {
    OTA_MANAGER_STATE_IDLE = 0,         ///< 空闲状态
    OTA_MANAGER_STATE_CHECKING,         ///< 检查更新中
    OTA_MANAGER_STATE_UPDATE_AVAILABLE, ///< 有可用更新
    OTA_MANAGER_STATE_UPGRADING,        ///< 升级中
    OTA_MANAGER_STATE_AUTH_CHECKING,    ///< 检查认证状态
    OTA_MANAGER_STATE_AUTH_PENDING,     ///< 等待激活
    OTA_MANAGER_STATE_AUTH_ACTIVATING,  ///< 激活中
    OTA_MANAGER_STATE_COMPLETED,        ///< 完成
    OTA_MANAGER_STATE_ERROR,            ///< 错误
} ota_manager_state_t;

/**
 * @brief OTA 管理器状态回调函数类型
 */
typedef void (*ota_manager_state_cb_t)(ota_manager_state_t state);

/**
 * @brief OTA 管理器配置结构体
 */
typedef struct {
    const char *server_url;             ///< OTA 服务器地址
    const char *device_type;            ///< 设备类型
    bool auto_upgrade;                  ///< 是否自动升级
    bool check_on_boot;                 ///< 启动时检查更新
    ota_manager_state_cb_t state_cb;    ///< 状态回调
    xn_ota_progress_cb_t progress_cb;   ///< 升级进度回调
} ota_manager_config_t;

/**
 * @brief OTA 管理器默认配置宏
 */
#define OTA_MANAGER_DEFAULT_CONFIG() \
    (ota_manager_config_t) { \
        .server_url = NULL, \
        .device_type = "unknown", \
        .auto_upgrade = false, \
        .check_on_boot = true, \
        .state_cb = NULL, \
        .progress_cb = NULL, \
    }

/* ========================================================================== */
/*                              公共API                                        */
/* ========================================================================== */

/**
 * @brief 初始化 OTA 管理器
 * 
 * @param config OTA 管理器配置
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t ota_manager_init(const ota_manager_config_t *config);

/**
 * @brief 反初始化 OTA 管理器
 * 
 * @return esp_err_t 
 *      - ESP_OK: 成功
 */
esp_err_t ota_manager_deinit(void);

/**
 * @brief 启动 OTA 管理器
 * 
 * 执行以下流程：
 * 1. 标记当前固件为有效
 * 2. 检查设备认证状态
 * 3. 如果需要激活，执行激活流程
 * 4. 检查固件更新
 * 5. 如果有更新且配置了自动升级，执行升级
 * 
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_FAIL: 失败
 */
esp_err_t ota_manager_start(void);

/**
 * @brief 停止 OTA 管理器
 * 
 * @return esp_err_t 
 *      - ESP_OK: 成功
 */
esp_err_t ota_manager_stop(void);

/**
 * @brief 手动检查更新
 * 
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_FAIL: 失败
 */
esp_err_t ota_manager_check_update(void);

/**
 * @brief 手动执行升级
 * 
 * @param version 目标版本号，NULL 表示升级到最新版本
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_FAIL: 失败
 */
esp_err_t ota_manager_upgrade(const char *version);

/**
 * @brief 获取当前状态
 * 
 * @return ota_manager_state_t 当前状态
 */
ota_manager_state_t ota_manager_get_state(void);

/**
 * @brief 获取设备认证状态
 * 
 * @param[out] status 认证状态
 * @param[out] activation_code 激活码（可选）
 * @param[out] activation_message 激活消息（可选）
 * @return esp_err_t 
 *      - ESP_OK: 成功
 */
esp_err_t ota_manager_get_auth_status(xn_ota_auth_status_t *status,
                                      char *activation_code,
                                      char *activation_message);

/**
 * @brief 检查是否需要 OTA 或认证
 * 
 * @return true 需要 OTA 或认证
 * @return false 不需要
 */
bool ota_manager_needs_action(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_MANAGER_H */
