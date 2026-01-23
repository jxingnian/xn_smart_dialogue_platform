/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-23 12:40:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:30:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_storage\xn_storage.c
 * @Description: 通用存储组件实现 - 封装NVS操作
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#include "xn_storage.h" // 包含组件头文件
#include "esp_log.h" // 包含日志库
#include "nvs_flash.h" // 包含NVS Flash库
#include "nvs.h" // 包含NVS操作库

static const char *TAG = "XN_STORAGE"; // 定义日志标签
#define DEFAULT_NAMESPACE "xn_config" // 定义默认NVS命名空间

// 初始化存储组件
esp_err_t xn_storage_init(void)
{
    // NVS初始化通常在main中调用nvs_flash_init()，这里作为组件可以不做全局init，
    // 或者为了保险起见，仅仅是LOG一下。实际NVS Flash Init应该由App层负责。
    // 但为了方便作为一个独立模块使用，我们可以检查并初始化。
    esp_err_t ret = nvs_flash_init(); // 初始化默认NVS分区
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { // 如果NVS页面满或版本不同
        ESP_LOGW(TAG, "NVS flash erase and init..."); // 打印擦除日志
        ESP_ERROR_CHECK(nvs_flash_erase()); // 擦除NVS Flash
        ret = nvs_flash_init(); // 重新初始化
    }
    return ret; // 返回结果
}

// 打开NVS句柄
static esp_err_t open_nvs(nvs_handle_t *handle, nvs_open_mode_t mode)
{
    esp_err_t ret = nvs_open(DEFAULT_NAMESPACE, mode, handle); // 打开NVS命名空间
    if (ret != ESP_OK) { // 如果打开失败
        ESP_LOGE(TAG, "Failed to open NVS namespace '%s': %s", DEFAULT_NAMESPACE, esp_err_to_name(ret)); // 打印错误日志
    }
    return ret; // 返回结果
}

// 存储字符串
esp_err_t xn_storage_set_str(const char *key, const char *value)
{
    nvs_handle_t handle; // NVS句柄
    esp_err_t ret = open_nvs(&handle, NVS_READWRITE); // 打开NVS，读写模式
    if (ret != ESP_OK) return ret; // 如果失败直接返回

    ret = nvs_set_str(handle, key, value); // 写入字符串，键为key
    if (ret == ESP_OK) { // 如果写入成功
        ret = nvs_commit(handle); // 提交更改
    } else {
        ESP_LOGE(TAG, "Failed to set str key '%s': %s", key, esp_err_to_name(ret)); // 打印写入失败日志
    }
    nvs_close(handle); // 关闭NVS句柄
    return ret; // 返回结果
}

// 读取字符串
esp_err_t xn_storage_get_str(const char *key, char *out_value, size_t *length)
{
    nvs_handle_t handle; // NVS句柄
    esp_err_t ret = open_nvs(&handle, NVS_READONLY); // 打开NVS，只读模式
    if (ret != ESP_OK) return ret; // 如果失败直接返回

    ret = nvs_get_str(handle, key, out_value, length); // 读取字符串
    nvs_close(handle); // 关闭NVS句柄
    return ret; // 返回结果
}

// 存储uint8_t
esp_err_t xn_storage_set_u8(const char *key, uint8_t value)
{
    nvs_handle_t handle; // NVS句柄
    esp_err_t ret = open_nvs(&handle, NVS_READWRITE); // 打开NVS，读写模式
    if (ret != ESP_OK) return ret; // 如果失败直接返回 

    ret = nvs_set_u8(handle, key, value); // 写入uint8_t值
    if (ret == ESP_OK) { // 如果写入成功
        ret = nvs_commit(handle); // 提交更改
    }
    nvs_close(handle); // 关闭NVS句柄
    return ret; // 返回结果
}

// 读取uint8_t
esp_err_t xn_storage_get_u8(const char *key, uint8_t *out_value)
{
    nvs_handle_t handle; // NVS句柄
    esp_err_t ret = open_nvs(&handle, NVS_READONLY); // 打开NVS，只读模式
    if (ret != ESP_OK) return ret; // 如果失败直接返回

    ret = nvs_get_u8(handle, key, out_value); // 读取uint8_t值
    nvs_close(handle); // 关闭NVS句柄
    return ret; // 返回结果
}

// 存储int32_t
esp_err_t xn_storage_set_i32(const char *key, int32_t value)
{
    nvs_handle_t handle; // NVS句柄
    esp_err_t ret = open_nvs(&handle, NVS_READWRITE); // 打开NVS，读写模式
    if (ret != ESP_OK) return ret; // 如果失败直接返回

    ret = nvs_set_i32(handle, key, value); // 写入int32_t值
    if (ret == ESP_OK) { // 如果写入成功
        ret = nvs_commit(handle); // 提交更改
    }
    nvs_close(handle); // 关闭NVS句柄
    return ret; // 返回结果
}

// 读取int32_t
esp_err_t xn_storage_get_i32(const char *key, int32_t *out_value)
{
    nvs_handle_t handle; // NVS句柄
    esp_err_t ret = open_nvs(&handle, NVS_READONLY); // 打开NVS，只读模式
    if (ret != ESP_OK) return ret; // 如果失败直接返回

    ret = nvs_get_i32(handle, key, out_value); // 读取int32_t值
    nvs_close(handle); // 关闭NVS句柄
    return ret; // 返回结果
}

// 删除指定键
esp_err_t xn_storage_erase(const char *key)
{
    nvs_handle_t handle; // NVS句柄
    esp_err_t ret = open_nvs(&handle, NVS_READWRITE); // 打开NVS，读写模式
    if (ret != ESP_OK) return ret; // 如果失败直接返回

    ret = nvs_erase_key(handle, key); // 删除指定键
    if (ret == ESP_OK) { // 如果删除成功
        ret = nvs_commit(handle); // 提交更改
    }
    nvs_close(handle); // 关闭NVS句柄
    return ret; // 返回结果
}
