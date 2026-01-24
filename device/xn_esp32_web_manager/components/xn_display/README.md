# xn_display 组件

## 概述

`xn_display` 是一个基于 LVGL 的显示组件，提供 LCD 驱动适配层和 LVGL 初始化功能。

## 特性

- ✅ 支持 ST7789 LCD 驱动（SPI 接口）
- ✅ 集成 LVGL 图形库
- ✅ PWM 背光控制
- ✅ 多线程安全（互斥锁保护）
- ✅ 灵活的配置选项
- ✅ 休眠/唤醒控制

## 硬件支持

### LCD 控制器
- ST7789 (240x240 / 240x320)
- ILI9341 (240x320) - 待实现
- ST7735 (128x160) - 待实现

### 接口
- SPI (支持 SPI2_HOST 和 SPI3_HOST)

## 使用方法

### 1. 基本初始化

```c
#include "xn_display.h"

// 获取默认配置
xn_display_config_t config = xn_display_get_default_config();

// 根据需要修改配置
config.width = 320;
config.height = 240;
config.pin_mosi = GPIO_NUM_47;
config.pin_sclk = GPIO_NUM_48;
// ... 其他配置

// 初始化显示
esp_err_t ret = xn_display_init(&config);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize display");
}
```

### 2. 使用 LVGL 创建 UI

```c
// 锁定 LVGL（多线程访问时必须）
if (xn_display_lock(1000)) {
    // 创建 LVGL 对象
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello World!");
    lv_obj_center(label);
    
    // 解锁 LVGL
    xn_display_unlock();
}
```

### 3. 控制背光

```c
// 设置亮度 (0-100)
xn_display_set_brightness(80);

// 休眠显示
xn_display_sleep(true);

// 唤醒显示
xn_display_sleep(false);
```

## 配置选项

### LCD 配置
- `lcd_type`: LCD 类型（ST7789/ILI9341/ST7735）
- `width`: 屏幕宽度
- `height`: 屏幕高度

### SPI 配置
- `spi_host`: SPI 主机（SPI2_HOST/SPI3_HOST）
- `pin_mosi`: MOSI 引脚
- `pin_sclk`: SCLK 引脚
- `pin_cs`: CS 引脚（GPIO_NUM_NC 表示不使用）
- `pin_dc`: DC 引脚
- `pin_rst`: RST 引脚
- `pin_bckl`: 背光引脚
- `spi_clk_hz`: SPI 时钟频率
- `spi_mode`: SPI 模式（0-3）

### 显示配置
- `mirror_x`: X 轴镜像
- `mirror_y`: Y 轴镜像
- `swap_xy`: 交换 XY 轴
- `invert_color`: 反转颜色
- `rgb_order`: RGB 顺序
- `offset_x`: X 偏移
- `offset_y`: Y 偏移
- `backlight_output_invert`: 背光输出反转

### LVGL 配置
- `lvgl_tick_period_ms`: LVGL tick 周期（默认 5ms）
- `lvgl_task_stack_size`: LVGL 任务栈大小（默认 4096）
- `lvgl_task_priority`: LVGL 任务优先级（默认 5）
- `lvgl_buffer_size`: LVGL 缓冲区大小（默认 width * 10）

## API 参考

### 初始化和反初始化
- `xn_display_init()`: 初始化显示系统
- `xn_display_deinit()`: 反初始化显示系统
- `xn_display_get_default_config()`: 获取默认配置

### 显示控制
- `xn_display_set_brightness()`: 设置亮度
- `xn_display_sleep()`: 休眠/唤醒控制

### LVGL 访问
- `xn_display_get_disp()`: 获取 LVGL 显示对象
- `xn_display_lock()`: 锁定 LVGL（多线程访问）
- `xn_display_unlock()`: 解锁 LVGL

## 依赖

- `driver`: ESP-IDF 驱动组件
- `esp_timer`: ESP-IDF 定时器组件
- `esp_lcd`: ESP-IDF LCD 驱动组件
- `lvgl`: LVGL 图形库

## 注意事项

1. **多线程访问**: 在其他任务中操作 LVGL 对象时，必须先调用 `xn_display_lock()` 锁定，操作完成后调用 `xn_display_unlock()` 解锁。

2. **内存分配**: 显示缓冲区使用 DMA 内存，确保系统有足够的 DMA 内存。

3. **SPI 总线**: 如果 SPI 总线被其他设备共享，需要注意时钟频率和模式配置。

4. **LVGL 配置**: 需要在项目中提供 `lv_conf.h` 配置文件。

## 示例

参考 `main/managers/display_manager.c` 和 `main/ui/` 目录下的示例代码。
