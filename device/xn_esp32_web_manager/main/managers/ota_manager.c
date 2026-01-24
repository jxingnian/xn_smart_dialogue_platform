/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24
 * @Description: OTA 管理器实现 - 业务逻辑管理（策略层）
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#include "ota_manager.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xn_event_bus.h"
#include "xn_ota.h"

/* 日志TAG */
static const char *TAG = "ota_manager";

/* ========================================================================== */
/*                              内部变量                                        */
/* ========================================================================== */

static ota_manager_config_t s_config;               // 配置
static ota_manager_state_t s_state = OTA_MANAGER_STATE_IDLE; // 当前状态
static bool s_initialized = false;                  // 初始化标志
static bool s_has_update = false;                   // 是否有更新
static bool s_needs_auth = false;                   // 是否需要认证
static xn_ota_version_info_t s_latest_version;      // 最新版本信息
static xn_ota_auth_status_t s_auth_status = XN_OTA_AUTH_UNKNOWN; // 认证状态
static char s_activation_code[64] = {0};            // 激活码
static char s_activation_message[256] = {0};        // 激活消息

/* ========================================================================== */
/*                              内部函数                                        */
/* ========================================================================== */

/**
 * @brief 更新状态并通知回调
 */
static void ota_manager_notify_state(ota_manager_state_t new_state)
{
    s_state = new_state;
    
    ESP_LOGI(TAG, "State changed to: %d", new_state);
    
    if (s_config.state_cb != NULL) {
        s_config.state_cb(new_state);
    }
}

/**
 * @brief 升级进度回调
 */
static void ota_progress_callback(int progress, size_t speed)
{
    ESP_LOGI(TAG, "OTA Progress: %d%%, Speed: %d B/s", progress, speed);
    
    if (s_config.progress_cb != NULL) {
        s_config.progress_cb(progress, speed);
    }
}

/**
 * @brief 执行 OTA 流程
 */
static esp_err_t ota_manager_run_flow(void)
{
    esp_err_t ret;
    
    // 1. 标记当前固件为有效
    ESP_LOGI(TAG, "Step 1: Mark current firmware as valid");
    ret = xn_ota_mark_valid();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to mark firmware valid: %s", esp_err_to_name(ret));
    }
    
    // 2. 检查设备认证状态
    ESP_LOGI(TAG, "Step 2: Check device authentication status");
    ota_manager_notify_state(OTA_MANAGER_STATE_AUTH_CHECKING);
    
    ret = xn_ota_check_auth_status(&s_auth_status, s_activation_code, s_activation_message);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to check auth status");
        ota_manager_notify_state(OTA_MANAGER_STATE_ERROR);
        return ret;
    }
    
    ESP_LOGI(TAG, "Auth status: %d", s_auth_status);
    
    // 3. 如果需要激活，执行激活流程
    if (s_auth_status == XN_OTA_AUTH_PENDING) {
        ESP_LOGI(TAG, "Step 3: Device activation required");
        ESP_LOGI(TAG, "Activation code: %s", s_activation_code);
        ESP_LOGI(TAG, "Activation message: %s", s_activation_message);
        
        ota_manager_notify_state(OTA_MANAGER_STATE_AUTH_PENDING);
        s_needs_auth = true;
        
        // 发布事件通知需要激活
        xn_event_post(XN_EVT_SYSTEM_INIT_DONE, XN_EVT_SRC_SYSTEM);
        
        // 尝试自动激活（简化版）
        ESP_LOGI(TAG, "Attempting automatic activation...");
        ota_manager_notify_state(OTA_MANAGER_STATE_AUTH_ACTIVATING);
        
        ret = xn_ota_activate_device();
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Device activated successfully");
            s_needs_auth = false;
            s_auth_status = XN_OTA_AUTH_ACTIVATED;
        } else if (ret == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Activation pending, waiting for user confirmation");
            // 保持 AUTH_PENDING 状态，等待用户在管理端确认
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Activation failed");
            ota_manager_notify_state(OTA_MANAGER_STATE_ERROR);
            return ret;
        }
    }
    
    // 4. 检查固件更新
    ESP_LOGI(TAG, "Step 4: Check firmware update");
    ota_manager_notify_state(OTA_MANAGER_STATE_CHECKING);
    
    ret = xn_ota_check_update(&s_has_update, &s_latest_version);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to check update");
        ota_manager_notify_state(OTA_MANAGER_STATE_ERROR);
        return ret;
    }
    
    if (s_has_update) {
        ESP_LOGI(TAG, "New version available: %s", s_latest_version.version);
        ota_manager_notify_state(OTA_MANAGER_STATE_UPDATE_AVAILABLE);
        
        // 5. 如果配置了自动升级，执行升级
        if (s_config.auto_upgrade || s_latest_version.force) {
            ESP_LOGI(TAG, "Step 5: Starting automatic upgrade");
            ota_manager_notify_state(OTA_MANAGER_STATE_UPGRADING);
            
            ret = xn_ota_upgrade(NULL);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Upgrade failed: %s", esp_err_to_name(ret));
                ota_manager_notify_state(OTA_MANAGER_STATE_ERROR);
                return ret;
            }
            
            ESP_LOGI(TAG, "Upgrade completed, restarting in 3 seconds...");
            ota_manager_notify_state(OTA_MANAGER_STATE_COMPLETED);
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_restart();
        }
    } else {
        ESP_LOGI(TAG, "No update available");
    }
    
    // 完成
    ota_manager_notify_state(OTA_MANAGER_STATE_COMPLETED);
    return ESP_OK;
}

/* ========================================================================== */
/*                              公共API实现                                     */
/* ========================================================================== */

esp_err_t ota_manager_init(const ota_manager_config_t *config)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 加载配置
    if (config == NULL) {
        s_config = OTA_MANAGER_DEFAULT_CONFIG();
    } else {
        s_config = *config;
    }
    
    // 检查必要参数
    if (s_config.server_url == NULL || strlen(s_config.server_url) == 0) {
        ESP_LOGE(TAG, "Server URL is required");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 初始化 OTA 组件
    xn_ota_config_t ota_config = XN_OTA_DEFAULT_CONFIG();
    ota_config.server_url = s_config.server_url;
    ota_config.device_type = s_config.device_type;
    ota_config.progress_cb = ota_progress_callback;
    
    esp_err_t ret = xn_ota_init(&ota_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init OTA component");
        return ret;
    }
    
    // 初始化状态
    s_state = OTA_MANAGER_STATE_IDLE;
    s_has_update = false;
    s_needs_auth = false;
    
    s_initialized = true;
    ESP_LOGI(TAG, "OTA manager initialized");
    
    return ESP_OK;
}

esp_err_t ota_manager_deinit(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xn_ota_deinit();
    
    s_initialized = false;
    s_state = OTA_MANAGER_STATE_IDLE;
    
    ESP_LOGI(TAG, "OTA manager deinitialized");
    
    return ESP_OK;
}

esp_err_t ota_manager_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting OTA manager flow");
    
    // 执行 OTA 流程
    esp_err_t ret = ota_manager_run_flow();
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA flow failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "OTA manager flow completed");
    
    return ESP_OK;
}

esp_err_t ota_manager_stop(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    s_state = OTA_MANAGER_STATE_IDLE;
    
    return ESP_OK;
}

esp_err_t ota_manager_check_update(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ota_manager_notify_state(OTA_MANAGER_STATE_CHECKING);
    
    esp_err_t ret = xn_ota_check_update(&s_has_update, &s_latest_version);
    if (ret != ESP_OK) {
        ota_manager_notify_state(OTA_MANAGER_STATE_ERROR);
        return ret;
    }
    
    if (s_has_update) {
        ota_manager_notify_state(OTA_MANAGER_STATE_UPDATE_AVAILABLE);
    } else {
        ota_manager_notify_state(OTA_MANAGER_STATE_COMPLETED);
    }
    
    return ESP_OK;
}

esp_err_t ota_manager_upgrade(const char *version)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ota_manager_notify_state(OTA_MANAGER_STATE_UPGRADING);
    
    esp_err_t ret = xn_ota_upgrade(version);
    if (ret != ESP_OK) {
        ota_manager_notify_state(OTA_MANAGER_STATE_ERROR);
        return ret;
    }
    
    ota_manager_notify_state(OTA_MANAGER_STATE_COMPLETED);
    
    ESP_LOGI(TAG, "Upgrade completed, restarting in 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
    
    return ESP_OK;
}

ota_manager_state_t ota_manager_get_state(void)
{
    return s_state;
}

esp_err_t ota_manager_get_auth_status(xn_ota_auth_status_t *status,
                                      char *activation_code,
                                      char *activation_message)
{
    if (!s_initialized || status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *status = s_auth_status;
    
    if (activation_code != NULL && strlen(s_activation_code) > 0) {
        strcpy(activation_code, s_activation_code);
    }
    
    if (activation_message != NULL && strlen(s_activation_message) > 0) {
        strcpy(activation_message, s_activation_message);
    }
    
    return ESP_OK;
}

bool ota_manager_needs_action(void)
{
    return s_has_update || s_needs_auth;
}
