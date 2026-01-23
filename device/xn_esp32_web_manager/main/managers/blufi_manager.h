/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\blufi_manager.h
 * @Description: BluFi配网应用管理器头文件 - 通过事件与其他模块通信
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
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
 * 
 * - 订阅相关的命令事件 (如 CMD_BLUFI_START)
 * - 准备内部状态
 * 
 * @return esp_err_t 初始化结果
 */
esp_err_t blufi_manager_init(void);

/**
 * @brief 反初始化BluFi管理器
 * 
 * - 停止运行中的服务
 * - 取消事件订阅
 * 
 * @return esp_err_t 反初始化结果
 */
esp_err_t blufi_manager_deinit(void);

/**
 * @brief 启动BluFi配网
 * 
 * - 初始化蓝牙控制器 (BLE模式)
 * - 注册 BluFi 回调函数
 * - 启动蓝牙广播
 * 
 * @return esp_err_t 启动结果
 */
esp_err_t blufi_manager_start(void);

/**
 * @brief 停止BluFi配网
 * 
 * - 停止蓝牙广播
 * - 释放蓝牙控制器资源（以节省内存）
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

#ifdef __cplusplus
}
#endif

#endif /* BLUFI_MANAGER_H */

