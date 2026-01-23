/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 13:55:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\blufi_manager.h
 * @Description: BluFi配网应用管理器头文件 - 通过事件与其他模块通信
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#ifndef BLUFI_MANAGER_H // 防止头文件重复包含
#define BLUFI_MANAGER_H // 防止头文件重复包含

#include "esp_err.h" // 包含ESP错误码定义
#include <stdbool.h> // 包含布尔类型定义

#ifdef __cplusplus // 如果是C++编译器
extern "C" { // 使用C链接约定
#endif // 结束C++编译器判断

/**
 * @brief 初始化BluFi管理器
 * 
 * - 创建BluFi组件实例
 * - 订阅相关的命令事件 (如 CMD_BLUFI_START)
 * - 订阅系统状态事件 (如 WIFI_GOT_IP)
 * 
 * @return esp_err_t 初始化结果
 */
esp_err_t blufi_manager_init(void);

/**
 * @brief 反初始化BluFi管理器
 * 
 * - 停止运行中的服务
 * - 取消事件订阅
 * - 销毁BluFi组件实例
 * 
 * @return esp_err_t 反初始化结果
 */
esp_err_t blufi_manager_deinit(void);

/**
 * @brief 启动BluFi配网服务
 * 
 * - 初始化底层蓝牙栈
 * - 注册回调并开启广播
 * - 通知系统配网就绪
 * 
 * @return esp_err_t 启动结果
 */
esp_err_t blufi_manager_start(void);

/**
 * @brief 停止BluFi配网服务
 * 
 * - 停止蓝牙广播
 * - 释放蓝牙资源
 * 
 * @return esp_err_t 停止结果
 */
esp_err_t blufi_manager_stop(void);

/**
 * @brief 检查BluFi是否正在运行
 * 
 * @return true 正在运行
 * @return false 未运行
 */
bool blufi_manager_is_running(void);

#ifdef __cplusplus // 如果是C++编译器
}
#endif // 结束C++编译器判断

#endif /* BLUFI_MANAGER_H */
