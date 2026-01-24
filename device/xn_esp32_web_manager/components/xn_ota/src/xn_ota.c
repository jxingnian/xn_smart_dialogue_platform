/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24
 * @Description: OTA 组件实现 - 固件升级、版本管理、设备认证
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#include "xn_ota.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"

/* 日志TAG */
static const char *TAG = "xn_ota";

/* ========================================================================== */
/*                              内部变量                                        */
/* ========================================================================== */

static xn_ota_config_t s_config;                    // OTA 配置
static bool s_initialized = false;                  // 初始化标志
static xn_ota_version_list_t s_cloud_versions;      // 云端版本列表缓存
static xn_ota_device_info_t s_device_info;          // 设备信息
static char s_activation_code[64] = {0};            // 激活码
static char s_activation_message[256] = {0};        // 激活消息
static char s_activation_challenge[256] = {0};      // 激活挑战码

/* ========================================================================== */
/*                              内部函数                                        */
/* ========================================================================== */

/**
 * @brief 生成设备 ID（基于 MAC 地址）
 */
static void generate_device_id(char *device_id, size_t len)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, len, "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/**
 * @brief 生成设备信息
 */
static esp_err_t generate_device_info(void)
{
    // 生成设备 ID
    generate_device_id(s_device_info.device_id, sizeof(s_device_info.device_id));
    
    // 设置设备类型
    snprintf(s_device_info.device_type, sizeof(s_device_info.device_type), 
             "%s", s_config.device_type);
    
    // 获取固件版本
    const esp_app_desc_t *app_desc = esp_app_get_description();
    snprintf(s_device_info.firmware_version, sizeof(s_device_info.firmware_version),
             "%s", app_desc->version);
    
    // 设置硬件版本
    snprintf(s_device_info.hardware_version, sizeof(s_device_info.hardware_version),
             "v1.0");
    
    // 获取 MAC 地址
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_device_info.mac_address, sizeof(s_device_info.mac_address),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // 获取芯片型号
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    snprintf(s_device_info.chip_model, sizeof(s_device_info.chip_model),
             "ESP32");
    
    // 序列号（从 NVS 读取，如果有的话）
    nvs_handle_t nvs_handle;
    if (nvs_open("ota", NVS_READONLY, &nvs_handle) == ESP_OK) {
        size_t len = sizeof(s_device_info.serial_number);
        nvs_get_str(nvs_handle, "serial_number", s_device_info.serial_number, &len);
        nvs_close(nvs_handle);
    }
    
    return ESP_OK;
}

/**
 * @brief 比较版本号
 * 
 * @return 1: v1 > v2, 0: v1 == v2, -1: v1 < v2
 */
static int compare_version(const char *v1, const char *v2)
{
    int major1 = 0, minor1 = 0, patch1 = 0;
    int major2 = 0, minor2 = 0, patch2 = 0;
    
    sscanf(v1, "%d.%d.%d", &major1, &minor1, &patch1);
    sscanf(v2, "%d.%d.%d", &major2, &minor2, &patch2);
    
    if (major1 != major2) return (major1 > major2) ? 1 : -1;
    if (minor1 != minor2) return (minor1 > minor2) ? 1 : -1;
    if (patch1 != patch2) return (patch1 > patch2) ? 1 : -1;
    
    return 0;
}

/**
 * @brief HTTP 事件处理回调
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", 
                     evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

/**
 * @brief 解析云端版本列表响应
 */
static esp_err_t parse_version_list_response(const char *json_str, 
                                             xn_ota_version_list_t *version_list)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }
    
    cJSON *firmware = cJSON_GetObjectItem(root, "firmware");
    if (!cJSON_IsObject(firmware)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    
    // 解析单个版本信息
    version_list->count = 0;
    xn_ota_version_info_t *ver = &version_list->versions[0];
    
    cJSON *version = cJSON_GetObjectItem(firmware, "version");
    if (cJSON_IsString(version)) {
        strncpy(ver->version, version->valuestring, XN_OTA_MAX_VERSION_LEN - 1);
    }
    
    cJSON *url = cJSON_GetObjectItem(firmware, "url");
    if (cJSON_IsString(url)) {
        strncpy(ver->url, url->valuestring, XN_OTA_MAX_URL_LEN - 1);
    }
    
    cJSON *size = cJSON_GetObjectItem(firmware, "size");
    if (cJSON_IsNumber(size)) {
        ver->size = size->valueint;
    }
    
    cJSON *md5 = cJSON_GetObjectItem(firmware, "md5");
    if (cJSON_IsString(md5)) {
        strncpy(ver->md5, md5->valuestring, 32);
    }
    
    cJSON *force = cJSON_GetObjectItem(firmware, "force");
    ver->force = (cJSON_IsNumber(force) && force->valueint == 1);
    
    cJSON *changelog = cJSON_GetObjectItem(firmware, "changelog");
    if (cJSON_IsString(changelog)) {
        strncpy(ver->changelog, changelog->valuestring, 255);
    }
    
    version_list->count = 1;
    
    // 解析激活信息
    cJSON *activation = cJSON_GetObjectItem(root, "activation");
    if (cJSON_IsObject(activation)) {
        cJSON *code = cJSON_GetObjectItem(activation, "code");
        if (cJSON_IsString(code)) {
            strncpy(s_activation_code, code->valuestring, sizeof(s_activation_code) - 1);
        }
        
        cJSON *message = cJSON_GetObjectItem(activation, "message");
        if (cJSON_IsString(message)) {
            strncpy(s_activation_message, message->valuestring, sizeof(s_activation_message) - 1);
        }
        
        cJSON *challenge = cJSON_GetObjectItem(activation, "challenge");
        if (cJSON_IsString(challenge)) {
            strncpy(s_activation_challenge, challenge->valuestring, sizeof(s_activation_challenge) - 1);
        }
    }
    
    cJSON_Delete(root);
    return ESP_OK;
}

/* ========================================================================== */
/*                              公共API实现                                     */
/* ========================================================================== */

esp_err_t xn_ota_init(const xn_ota_config_t *config)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 加载配置
    if (config == NULL) {
        s_config = XN_OTA_DEFAULT_CONFIG();
    } else {
        s_config = *config;
    }
    
    // 检查必要参数
    if (s_config.server_url == NULL || strlen(s_config.server_url) == 0) {
        ESP_LOGE(TAG, "Server URL is required");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 生成设备信息
    generate_device_info();
    
    // 清空版本列表
    memset(&s_cloud_versions, 0, sizeof(s_cloud_versions));
    
    s_initialized = true;
    ESP_LOGI(TAG, "OTA component initialized");
    ESP_LOGI(TAG, "Device ID: %s", s_device_info.device_id);
    ESP_LOGI(TAG, "Firmware Version: %s", s_device_info.firmware_version);
    
    return ESP_OK;
}

esp_err_t xn_ota_deinit(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    s_initialized = false;
    ESP_LOGI(TAG, "OTA component deinitialized");
    
    return ESP_OK;
}

const char *xn_ota_get_local_version(void)
{
    return s_device_info.firmware_version;
}

esp_err_t xn_ota_get_cloud_versions(xn_ota_version_list_t *version_list)
{
    if (!s_initialized || version_list == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 构建请求 URL
    char url[512];
    snprintf(url, sizeof(url), "%s", s_config.server_url);
    
    // 配置 HTTP 客户端
    esp_http_client_config_t http_config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = s_config.timeout_ms,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return ESP_FAIL;
    }
    
    // 设置请求头
    esp_http_client_set_header(client, "Device-Id", s_device_info.device_id);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    // 构建请求体
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", s_device_info.device_id);
    cJSON_AddStringToObject(root, "device_type", s_device_info.device_type);
    cJSON_AddStringToObject(root, "firmware_version", s_device_info.firmware_version);
    cJSON_AddStringToObject(root, "hardware_version", s_device_info.hardware_version);
    cJSON_AddStringToObject(root, "chip_model", s_device_info.chip_model);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    // 发送 POST 请求
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, json_str, strlen(json_str));
    
    esp_err_t err = esp_http_client_perform(client);
    free(json_str);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
    
    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP status code: %d", status_code);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    // 读取响应
    int content_length = esp_http_client_get_content_length(client);
    char *response = malloc(content_length + 1);
    if (response == NULL) {
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }
    
    int read_len = esp_http_client_read(client, response, content_length);
    response[read_len] = '\0';
    
    ESP_LOGI(TAG, "Response: %s", response);
    
    // 解析响应
    err = parse_version_list_response(response, version_list);
    
    free(response);
    esp_http_client_cleanup(client);
    
    if (err == ESP_OK) {
        // 缓存版本列表
        memcpy(&s_cloud_versions, version_list, sizeof(s_cloud_versions));
    }
    
    return err;
}

esp_err_t xn_ota_check_update(bool *has_update, xn_ota_version_info_t *latest_version)
{
    if (!s_initialized || has_update == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取云端版本列表
    xn_ota_version_list_t version_list;
    esp_err_t err = xn_ota_get_cloud_versions(&version_list);
    if (err != ESP_OK) {
        return err;
    }
    
    *has_update = false;
    
    if (version_list.count == 0) {
        return ESP_OK;
    }
    
    // 检查是否有新版本
    const char *current_version = xn_ota_get_local_version();
    xn_ota_version_info_t *newest = &version_list.versions[0];
    
    if (compare_version(newest->version, current_version) > 0 || newest->force) {
        *has_update = true;
        if (latest_version != NULL) {
            memcpy(latest_version, newest, sizeof(xn_ota_version_info_t));
        }
        ESP_LOGI(TAG, "New version available: %s", newest->version);
    } else {
        ESP_LOGI(TAG, "Current version is up to date");
    }
    
    return ESP_OK;
}

esp_err_t xn_ota_upgrade(const char *version)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 如果未指定版本，升级到最新版本
    xn_ota_version_info_t *target_version = NULL;
    
    if (version == NULL) {
        // 使用缓存的版本列表
        if (s_cloud_versions.count == 0) {
            ESP_LOGE(TAG, "No version available");
            return ESP_ERR_NOT_FOUND;
        }
        target_version = &s_cloud_versions.versions[0];
    } else {
        // 查找指定版本
        for (int i = 0; i < s_cloud_versions.count; i++) {
            if (strcmp(s_cloud_versions.versions[i].version, version) == 0) {
                target_version = &s_cloud_versions.versions[i];
                break;
            }
        }
        
        if (target_version == NULL) {
            ESP_LOGE(TAG, "Version %s not found", version);
            return ESP_ERR_NOT_FOUND;
        }
    }
    
    ESP_LOGI(TAG, "Starting OTA upgrade to version %s", target_version->version);
    ESP_LOGI(TAG, "Download URL: %s", target_version->url);
    
    // 配置 HTTPS OTA
    esp_http_client_config_t http_config = {
        .url = target_version->url,
        .timeout_ms = s_config.timeout_ms,
        .keep_alive_enable = true,
    };
    
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };
    
    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // 下载并写入固件
    size_t total_read = 0;
    size_t recent_read = 0;
    int64_t last_calc_time = esp_timer_get_time();
    
    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        
        // 计算进度和速度
        int image_len = esp_https_ota_get_image_len_read(ota_handle);
        recent_read = image_len - total_read;
        total_read = image_len;
        
        int64_t now = esp_timer_get_time();
        if (now - last_calc_time >= 1000000) {  // 每秒更新一次
            int progress = (total_read * 100) / target_version->size;
            size_t speed = recent_read;
            
            ESP_LOGI(TAG, "Progress: %d%% (%d/%d), Speed: %d B/s", 
                     progress, total_read, target_version->size, speed);
            
            if (s_config.progress_cb != NULL) {
                s_config.progress_cb(progress, speed);
            }
            
            last_calc_time = now;
            recent_read = 0;
        }
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA perform failed: %s", esp_err_to_name(err));
        esp_https_ota_abort(ota_handle);
        return err;
    }
    
    // 完成 OTA
    err = esp_https_ota_finish(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "OTA upgrade successful, restarting...");
    
    return ESP_OK;
}

esp_err_t xn_ota_mark_valid(void)
{
    const esp_partition_t *partition = esp_ota_get_running_partition();
    if (partition == NULL) {
        return ESP_FAIL;
    }
    
    if (strcmp(partition->label, "factory") == 0) {
        ESP_LOGI(TAG, "Running from factory partition, skipping");
        return ESP_OK;
    }
    
    esp_ota_img_states_t state;
    esp_err_t err = esp_ota_get_state_partition(partition, &state);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get partition state");
        return err;
    }
    
    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "Marking firmware as valid");
        err = esp_ota_mark_app_valid_cancel_rollback();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to mark app valid");
            return err;
        }
    }
    
    return ESP_OK;
}

esp_err_t xn_ota_check_auth_status(xn_ota_auth_status_t *status,
                                    char *activation_code,
                                    char *activation_message)
{
    if (!s_initialized || status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 检查是否有激活挑战
    if (strlen(s_activation_challenge) > 0) {
        *status = XN_OTA_AUTH_PENDING;
        
        if (activation_code != NULL && strlen(s_activation_code) > 0) {
            strcpy(activation_code, s_activation_code);
        }
        
        if (activation_message != NULL && strlen(s_activation_message) > 0) {
            strcpy(activation_message, s_activation_message);
        }
    } else {
        *status = XN_OTA_AUTH_ACTIVATED;
    }
    
    return ESP_OK;
}

esp_err_t xn_ota_get_device_info(xn_ota_device_info_t *device_info)
{
    if (!s_initialized || device_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(device_info, &s_device_info, sizeof(xn_ota_device_info_t));
    return ESP_OK;
}

esp_err_t xn_ota_submit_device_info(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 通过 xn_ota_get_cloud_versions 已经提交了设备信息
    // 这里只是一个包装函数
    xn_ota_version_list_t version_list;
    return xn_ota_get_cloud_versions(&version_list);
}

esp_err_t xn_ota_activate_device(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (strlen(s_activation_challenge) == 0) {
        ESP_LOGW(TAG, "No activation challenge");
        return ESP_OK;
    }
    
    // 构建激活 URL
    char url[512];
    snprintf(url, sizeof(url), "%s/activate", s_config.server_url);
    
    // 配置 HTTP 客户端
    esp_http_client_config_t http_config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = s_config.timeout_ms,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (client == NULL) {
        return ESP_FAIL;
    }
    
    // 设置请求头
    esp_http_client_set_header(client, "Device-Id", s_device_info.device_id);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    // 构建激活 Payload（简化版，实际应使用 HMAC 签名）
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", s_device_info.device_id);
    cJSON_AddStringToObject(root, "challenge", s_activation_challenge);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    // 发送 POST 请求
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, json_str, strlen(json_str));
    
    esp_err_t err = esp_http_client_perform(client);
    free(json_str);
    
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return err;
    }
    
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    
    if (status_code == 202) {
        return ESP_ERR_TIMEOUT;  // 激活超时
    } else if (status_code == 200) {
        ESP_LOGI(TAG, "Device activated successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Activation failed, status: %d", status_code);
        return ESP_FAIL;
    }
}
