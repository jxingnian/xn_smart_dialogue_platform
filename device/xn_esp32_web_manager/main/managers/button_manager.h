/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-23 09:48:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 09:48:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\managers\button_manager.h
 * @Description: 按键管理器 - 处理板载按键事件
 * VX:Jxingnian
 * Copyright (c) 2026 by xingnian, All Rights Reserved. 
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化按键管理器
 * 
 * - 配置 GPIO 0 (BOOT键) 为输入模式
 * - 创建按键扫描任务
 * 
 * @return esp_err_t 初始化结果
 */
esp_err_t button_manager_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_MANAGER_H */
