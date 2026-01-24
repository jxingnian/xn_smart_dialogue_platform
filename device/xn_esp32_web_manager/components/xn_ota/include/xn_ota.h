/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24
 * @Description: OTA 组件头文件 - 提供固件升级、版本管理、设备认证功能
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#ifndef XN_OTA_H
#define XN_OTA_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                              常量定义                                        */
/* ========================================================================== */

#define XN_OTA_MAX_VERSION_LEN      32      ///< 版本号最大长度
#define XN_OTA_MAX_URL_LEN          256     ///< URL 最大长度
#define XN_OTA_MAX_VERSIONS         10      ///< 最大版本列表数量

/* ========================================================================== */
/*                              类型定义                                        */
/* ========================================================================== */

/**
 * @brief OTA 版本信息结构体
 */
typedef struct {
    char version[XN_OTA_MAX_VERSION_LEN];   ///< 版本号
    char url[XN_OTA_MAX_URL_LEN];           ///< 固件下载地址
    uint32_t size;                          ///< 固件大小（字节）
    char md5[33];                           ///< MD5 校验值
    bool force;                             ///< 是否强制升级
    char changelog[256];                    ///< 更新日志
} xn_ota_version_info_t;

/**
 * @brief OTA 版本列表结构体
 */
typedef struct {
    xn_ota_version_info_t versions[XN_OTA_MAX_VERSIONS]; ///< 版本信息数组
    uint8_t count;                          ///< 版本数量
} xn_ota_version_list_t;

/**
 * @brief 设备认证状态枚举
 */
typedef enum {
    XN_OTA_AUTH_UNKNOWN = 0,                ///< 未知状态
    XN_OTA_AUTH_NOT_ACTIVATED,              ///< 未激活
    XN_OTA_AUTH_PENDING,                    ///< 等待激活
    XN_OTA_AUTH_ACTIVATED,                  ///< 已激活
    XN_OTA_AUTH_FAILED,                     ///< 激活失败
} xn_ota_auth_status_t;

/**
 * @brief 设备信息结构体
 */
typedef struct {
    char device_id[32];                     ///< 设备 ID
    char device_type[32];                   ///< 设备类型
    char firmware_version[32];              ///< 固件版本
    char hardware_version[32];              ///< 硬件版本
    char mac_address[18];                   ///< MAC 地址
    char chip_model[32];                    ///< 芯片型号
    char serial_number[64];                 ///< 序列号（可选）
} xn_ota_device_info_t;

/**
 * @brief OTA 升级进度回调函数类型
 * 
 * @param progress 进度百分比 (0-100)
 * @param speed 当前下载速度 (字节/秒)
 */
typedef void (*xn_ota_progress_cb_t)(int progress, size_t speed);

/**
 * @brief OTA 配置结构体
 */
typedef struct {
    const char *server_url;                 ///< OTA 服务器地址
    const char *device_type;                ///< 设备类型
    xn_ota_progress_cb_t progress_cb;       ///< 升级进度回调
    uint32_t timeout_ms;                    ///< 超时时间（毫秒）
} xn_ota_config_t;

/**
 * @brief OTA 默认配置宏
 */
#define XN_OTA_DEFAULT_CONFIG() \
    (xn_ota_config_t) { \
        .server_url = NULL, \
        .device_type = "unknown", \
        .progress_cb = NULL, \
        .timeout_ms = 30000, \
    }

/* ========================================================================== */
/*                              公共API                                        */
/* ========================================================================== */

/**
 * @brief 初始化 OTA 组件
 * 
 * @param config OTA 配置，NULL 使用默认配置
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - ESP_ERR_NO_MEM: 内存不足
 */
esp_err_t xn_ota_init(const xn_ota_config_t *config);

/**
 * @brief 反初始化 OTA 组件
 * 
 * @return esp_err_t 
 *      - ESP_OK: 成功
 */
esp_err_t xn_ota_deinit(void);

/**
 * @brief 获取本地固件版本
 * 
 * @return const char* 当前固件版本号
 */
const char *xn_ota_get_local_version(void);

/**
 * @brief 获取云端版本列表
 * 
 * @param[out] version_list 版本列表输出
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_FAIL: 获取失败
 *      - ESP_ERR_TIMEOUT: 超时
 */
esp_err_t xn_ota_get_cloud_versions(xn_ota_version_list_t *version_list);

/**
 * @brief 检查是否有新版本
 * 
 * @param[out] has_update 是否有更新
 * @param[out] latest_version 最新版本信息（可选，传 NULL 忽略）
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_FAIL: 检查失败
 */
esp_err_t xn_ota_check_update(bool *has_update, xn_ota_version_info_t *latest_version);

/**
 * @brief 升级到指定版本
 * 
 * @param version 目标版本号，NULL 表示升级到最新版本
 * @return esp_err_t 
 *      - ESP_OK: 升级成功
 *      - ESP_FAIL: 升级失败
 *      - ESP_ERR_NOT_FOUND: 版本不存在
 *      - ESP_ERR_INVALID_CRC: 校验失败
 */
esp_err_t xn_ota_upgrade(const char *version);

/**
 * @brief 标记当前固件为有效
 * 
 * 在 OTA 升级后首次启动时调用，防止回滚
 * 
 * @return esp_err_t 
 *      - ESP_OK: 成功
 */
esp_err_t xn_ota_mark_valid(void);

/**
 * @brief 检查设备认证状态
 * 
 * @param[out] status 认证状态输出
 * @param[out] activation_code 激活码输出（可选，传 NULL 忽略）
 * @param[out] activation_message 激活消息输出（可选，传 NULL 忽略）
 * @return esp_err_t 
 *      - ESP_OK: 成功
 *      - ESP_FAIL: 检查失败
 */
esp_err_t xn_ota_check_auth_status(xn_ota_auth_status_t *status, 
                                    char *activation_code, 
                                    char *activation_message);

/**
 * @brief 生成设备信息
 * 
 * @param[out] device_info 设备信息输出
 * @return esp_err_t 
 *      - ESP_OK: 成功
 */
esp_err_t xn_ota_get_device_info(xn_ota_device_info_t *device_info);

/**
 * @brief 提交设备信息到云端进行认证
 * 
 * @return esp_err_t 
 *      - ESP_OK: 提交成功
 *      - ESP_FAIL: 提交失败
 *      - ESP_ERR_TIMEOUT: 超时
 */
esp_err_t xn_ota_submit_device_info(void);

/**
 * @brief 执行设备激活
 * 
 * @return esp_err_t 
 *      - ESP_OK: 激活成功
 *      - ESP_ERR_TIMEOUT: 激活超时（202）
 *      - ESP_FAIL: 激活失败
 */
esp_err_t xn_ota_activate_device(void);

#ifdef __cplusplus
}
#endif

#endif /* XN_OTA_H */
