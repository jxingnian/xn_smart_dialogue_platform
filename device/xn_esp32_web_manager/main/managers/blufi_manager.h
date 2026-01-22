/**
 * @file blufi_manager.h
 * @brief BluFi配网应用管理器 - 通过事件与其他模块通信
 * 
 * 负责管理 ESP32 的蓝牙配网流程。
 * 功能包括：
 * - 启动/停止蓝牙控制器
 * - 响应 BluFi 配网事件
 * - 将接收到的 WiFi 配置写入 NVS
 * - 发布配网完成事件
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
