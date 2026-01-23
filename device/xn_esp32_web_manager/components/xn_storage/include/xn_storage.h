/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-23 12:40:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:25:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_storage\include\xn_storage.h
 * @Description: 通用存储组件头文件 - 封装NVS操作
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#pragma once // 防止头文件重复包含

#include "esp_err.h" // 包含ESP错误码定义
#include <stdint.h> // 包含标准整型定义
#include <stdbool.h> // 包含布尔类型定义

#ifdef __cplusplus // 如果是C++编译器
extern "C" { // 使用C链接约定
#endif // 结束C++编译器判断

/**
 * @brief 初始化存储组件 (NVS)
 * 
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_storage_init(void); // 初始化存储组件函数声明

/**
 * @brief 存储字符串
 * 
 * @param key 键名 (最大15字符)
 * @param value 字符串值
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_storage_set_str(const char *key, const char *value); // 存储字符串函数声明

/**
 * @brief 读取字符串
 * 
 * @param key 键名
 * @param out_value 输出缓冲区
 * @param length 缓冲区大小 (输入时为缓冲区大小，输出时为实际读取长度)
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_storage_get_str(const char *key, char *out_value, size_t *length); // 读取字符串函数声明

/**
 * @brief 存储uint8_t
 * 
 * @param key 键名
 * @param value 值
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_storage_set_u8(const char *key, uint8_t value); // 存储uint8_t函数声明

/**
 * @brief 读取uint8_t
 * 
 * @param key 键名
 * @param out_value 输出值指针
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_storage_get_u8(const char *key, uint8_t *out_value); // 读取uint8_t函数声明

/**
 * @brief 存储int32_t
 * 
 * @param key 键名
 * @param value 值
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_storage_set_i32(const char *key, int32_t value); // 存储int32_t函数声明

/**
 * @brief 读取int32_t
 * 
 * @param key 键名
 * @param out_value 输出值指针
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_storage_get_i32(const char *key, int32_t *out_value); // 读取int32_t函数声明

/**
 * @brief 删除指定键
 * 
 * @param key 键名
 * @return esp_err_t 返回ESP_OK表示成功，其他表示失败
 */
esp_err_t xn_storage_erase(const char *key); // 删除指定键函数声明

/* 
 * 注意：更复杂的存储需求（如Blob）可后续添加
 */

#ifdef __cplusplus // 如果是C++编译器
}
#endif // 结束C++编译器判断
