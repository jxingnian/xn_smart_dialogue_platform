/*
 * @Author: 星年 jixingnian@gmail.com
 * @Date: 2025-01-15
 * @Description: WiFi配置存储层 - 头文件
 * 
 * 功能说明：
 * 1. 负责WiFi配置的持久化存储，使用ESP-IDF的NVS组件
 * 2. 支持存储多个WiFi配置（SSID和密码）
 * 3. 提供保存、加载、删除配置的接口
 * 4. 自动管理存储空间，支持简单的配置轮换
 */

#ifndef XN_WIFI_STORAGE_H
#define XN_WIFI_STORAGE_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi配置信息结构体 */
typedef struct {
    char ssid[32];          // WiFi SSID，最大32字节
    char password[64];      // WiFi 密码，最大64字节
} xn_wifi_config_t;

/**
 * @brief 初始化WiFi存储层
 * 
 * 初始化NVS Flash，如果NVS分区损坏或版本不匹配，会自动擦除并重新初始化。
 * 
 * @return esp_err_t 
 *      - ESP_OK: 初始化成功
 *      - 其他: NVS初始化失败代码
 */
esp_err_t xn_wifi_storage_init(void);

/**
 * @brief 保存WiFi配置到NVS
 * 
 * @param ssid WiFi名称（SSID）
 * @param password WiFi密码
 * @return esp_err_t 
 *      - ESP_OK: 保存成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 *      - 其他: NVS写入失败
 */
esp_err_t xn_wifi_storage_save(const char *ssid, const char *password);

/**
 * @brief 从NVS加载第一个WiFi配置（兼容旧接口）
 * 
 * 此函数默认加载存储中的第一个配置。
 * 
 * @param config 输出参数，用于保存加载的配置
 * @return esp_err_t 
 *      - ESP_OK: 加载成功
 *      - ESP_ERR_NVS_NOT_FOUND: 未找到配置
 *      - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t xn_wifi_storage_load(xn_wifi_config_t *config);

/**
 * @brief 从NVS加载所有WiFi配置
 * 
 * @param configs 输出参数，配置数组指针
 * @param count 输出参数，实际加载的配置数量
 * @param max_count 数组最大容量
 * @return esp_err_t 
 *      - ESP_OK: 加载成功
 *      - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t xn_wifi_storage_load_all(xn_wifi_config_t *configs, uint8_t *count, uint8_t max_count);

/**
 * @brief 删除指定索引的WiFi配置
 * 
 * @param index 配置索引（从0开始）
 * @return esp_err_t 
 *      - ESP_OK: 删除成功
 *      - ESP_ERR_INVALID_ARG: 索引超出范围
 *      - 其他: NVS操作失败
 */
esp_err_t xn_wifi_storage_delete_by_index(uint8_t index);

/**
 * @brief 从NVS删除所有WiFi配置
 * 
 * @return esp_err_t 
 *      - ESP_OK: 删除成功
 *      - 其他: NVS操作失败
 */
esp_err_t xn_wifi_storage_delete(void);

/**
 * @brief 检查是否存在WiFi配置
 * 
 * @return true 表示存在至少一个WiFi配置
 * @return false 表示不存在任何配置
 */
bool xn_wifi_storage_exists(void);

#ifdef __cplusplus
}
#endif

#endif // XN_WIFI_STORAGE_H
