/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24 20:00:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-24 20:00:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_display\include\xn_display.h
 * @Description: 显示组件头文件 - LVGL + LCD 驱动适配层
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef XN_DISPLAY_H
#define XN_DISPLAY_H

#include "esp_err.h"
#include "lvgl.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 *                          类型定义
 *===========================================================================*/

/**
 * @brief LCD 类型枚举
 */
typedef enum {
    XN_DISPLAY_LCD_ST7789,      ///< ST7789 驱动（240x240/240x320）
} xn_display_lcd_type_t;

/**
 * @brief SPI 主机枚举
 */
typedef enum {
    XN_DISPLAY_SPI_HOST_2 = 1,  ///< SPI2_HOST
    XN_DISPLAY_SPI_HOST_3 = 2,  ///< SPI3_HOST
} xn_display_spi_host_t;
typedef enum {
    XN_DISPLAY_RGB_ORDER_RGB,   ///< RGB 顺序
    XN_DISPLAY_RGB_ORDER_BGR,   ///< BGR 顺序
} xn_display_rgb_order_t;

/**
 * @brief 显示配置结构
 */
typedef struct {
    // LCD 基本配置
    xn_display_lcd_type_t lcd_type;     ///< LCD 类型
    uint16_t width;                      ///< 屏幕宽度
    uint16_t height;                     ///< 屏幕高度
    
    // SPI 配置
    int spi_host;                        ///< SPI 主机 (1=SPI2_HOST, 2=SPI3_HOST)
    int pin_mosi;                        ///< MOSI 引脚
    int pin_sclk;                        ///< SCLK 引脚
    int pin_cs;                          ///< CS 引脚 (GPIO_NUM_NC 表示不使用)
    int pin_dc;                          ///< DC 引脚
    int pin_rst;                         ///< RST 引脚
    int pin_bckl;                        ///< 背光引脚
    uint32_t spi_clk_hz;                 ///< SPI 时钟频率
    uint8_t spi_mode;                    ///< SPI 模式 (0-3)
    
    // 显示配置
    bool mirror_x;                       ///< X 轴镜像
    bool mirror_y;                       ///< Y 轴镜像
    bool swap_xy;                        ///< 交换 XY 轴
    bool invert_color;                   ///< 反转颜色
    xn_display_rgb_order_t rgb_order;    ///< RGB 顺序
    uint16_t offset_x;                   ///< X 偏移
    uint16_t offset_y;                   ///< Y 偏移
    bool backlight_output_invert;        ///< 背光输出反转
    
    // LVGL 配置
    uint32_t lvgl_tick_period_ms;        ///< LVGL tick 周期（默认5ms）
    uint32_t lvgl_task_stack_size;       ///< LVGL 任务栈大小（默认4096）
    uint8_t lvgl_task_priority;          ///< LVGL 任务优先级（默认5）
    uint32_t lvgl_buffer_size;           ///< LVGL 缓冲区大小（像素数，默认width*10）
} xn_display_config_t;

/*===========================================================================
 *                          API
 *===========================================================================*/

/**
 * @brief 初始化显示系统
 * 
 * 执行流程：
 * - 初始化 SPI 总线
 * - 初始化 LCD 驱动
 * - 初始化 LVGL 库
 * - 创建 LVGL 任务
 * - 初始化背光控制
 * 
 * @param config 显示配置
 * @return esp_err_t 
 *         - ESP_OK: 初始化成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_ERR_NO_MEM: 内存不足
 *         - 其他: 初始化失败
 */
esp_err_t xn_display_init(const xn_display_config_t *config);

/**
 * @brief 反初始化显示系统
 * 
 * 执行流程：
 * - 停止 LVGL 任务
 * - 释放 LVGL 资源
 * - 释放 LCD 驱动资源
 * - 释放 SPI 总线
 * 
 * @return esp_err_t 
 *         - ESP_OK: 反初始化成功
 *         - 其他: 反初始化失败
 */
esp_err_t xn_display_deinit(void);

/**
 * @brief 设置屏幕亮度
 * 
 * 通过 PWM 控制背光亮度
 * 
 * @param brightness 亮度值 (0-100)
 * @return esp_err_t 
 *         - ESP_OK: 设置成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t xn_display_set_brightness(uint8_t brightness);

/**
 * @brief 屏幕休眠控制
 * 
 * 休眠时关闭背光和显示，唤醒时恢复
 * 
 * @param sleep true=休眠, false=唤醒
 * @return esp_err_t 
 *         - ESP_OK: 控制成功
 *         - 其他: 控制失败
 */
esp_err_t xn_display_sleep(bool sleep);

/**
 * @brief 获取 LVGL 显示对象
 * 
 * 用于直接操作 LVGL 显示对象
 * 
 * @return lv_display_t* LVGL 显示对象指针（LVGL 9.x），失败返回 NULL
 */
lv_display_t* xn_display_get_disp(void);

/**
 * @brief 锁定 LVGL（用于多线程访问）
 * 
 * 在其他任务中操作 LVGL 对象前必须先锁定
 * 
 * @param timeout_ms 超时时间（毫秒）
 * @return true 锁定成功
 * @return false 锁定失败（超时）
 */
bool xn_display_lock(uint32_t timeout_ms);

/**
 * @brief 解锁 LVGL
 * 
 * 操作完成后必须解锁
 */
void xn_display_unlock(void);

/**
 * @brief 获取默认配置
 * 
 * 返回一个预设的默认配置，用户可以在此基础上修改
 * 
 * @return xn_display_config_t 默认配置
 */
xn_display_config_t xn_display_get_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* XN_DISPLAY_H */
