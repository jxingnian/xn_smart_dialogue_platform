/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24 20:00:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-24 20:00:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_display\include\xn_display_lcd.h
 * @Description: LCD 驱动接口定义
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef XN_DISPLAY_LCD_H
#define XN_DISPLAY_LCD_H

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "xn_display.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 *                          LCD 驱动接口
 *===========================================================================*/

/**
 * @brief 初始化 ST7789 LCD
 * 
 * @param config 显示配置
 * @param out_panel 输出 LCD 面板句柄
 * @param out_io 输出 LCD IO 句柄
 * @return esp_err_t 初始化结果
 */
esp_err_t lcd_st7789_init(
    const xn_display_config_t *config,
    esp_lcd_panel_handle_t *out_panel,
    esp_lcd_panel_io_handle_t *out_io
);

#ifdef __cplusplus
}
#endif

#endif /* XN_DISPLAY_LCD_H */
