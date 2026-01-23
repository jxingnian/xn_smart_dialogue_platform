/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_blufi\include\xn_blufi_internal.h
 * @Description: BluFi内部头文件 - NimBLE相关函数声明
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef XN_BLUFI_INTERNAL_H
#define XN_BLUFI_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "host/ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NimBLE协议栈重置回调
 * 
 * 当NimBLE协议栈重置时调用，通常是因为底层硬件问题或系统请求。
 * 
 * @param reason 重置原因代码
 */
void xn_blufi_on_reset(int reason);

/**
 * @brief NimBLE协议栈同步回调
 * 
 * 当NimBLE协议栈完成与控制器的同步时调用，此时可以开始从应用层进行蓝牙操作。
 */
void xn_blufi_on_sync(void);

/**
 * @brief NimBLE主机任务函数
 * 
 * 在FreeRTOS任务中运行的NimBLE主循环。
 * 此函数不会返回，直到nimble_port_stop()被调用。
 * 
 * @param param 任务参数（通常为NULL）
 */
void xn_blufi_host_task(void *param);

#ifdef __cplusplus
}
#endif

#endif // XN_BLUFI_INTERNAL_H
