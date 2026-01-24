/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-24 20:00:00
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-24 20:00:00
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\components\xn_display\src\lcd_st7789.c
 * @Description: ST7789 LCD 驱动实现
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include "xn_display_lcd.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

static const char *TAG = "lcd_st7789";

esp_err_t lcd_st7789_init(
    const xn_display_config_t *config,
    esp_lcd_panel_handle_t *out_panel,
    esp_lcd_panel_io_handle_t *out_io)
{
    if (config == NULL || out_panel == NULL || out_io == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing ST7789 LCD (SPI)...");
    ESP_LOGI(TAG, "Resolution: %dx%d", config->width, config->height);
    ESP_LOGI(TAG, "SPI: MOSI=%d, SCLK=%d, CS=%d, DC=%d, RST=%d", 
             config->pin_mosi, config->pin_sclk, config->pin_cs, 
             config->pin_dc, config->pin_rst);
    
    // 1. 初始化 SPI 总线
    spi_bus_config_t buscfg = {
        .mosi_io_num = config->pin_mosi,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = config->pin_sclk,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = config->width * config->height * sizeof(uint16_t),
    };
    
    ret = spi_bus_initialize(config->spi_host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 2. 配置 LCD IO (SPI)
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = config->pin_dc,
        .cs_gpio_num = config->pin_cs,
        .pclk_hz = config->spi_clk_hz,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = config->spi_mode,
        .trans_queue_depth = 10,
    };
    
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)config->spi_host, &io_config, out_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LCD IO: %s", esp_err_to_name(ret));
        spi_bus_free(config->spi_host);
        return ret;
    }
    
    // 3. 配置 LCD 面板
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config->pin_rst,
        .rgb_ele_order = (config->rgb_order == XN_DISPLAY_RGB_ORDER_RGB) ? 
                         LCD_RGB_ELEMENT_ORDER_RGB : LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    
    ret = esp_lcd_new_panel_st7789(*out_io, &panel_config, out_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LCD panel: %s", esp_err_to_name(ret));
        esp_lcd_panel_io_del(*out_io);
        spi_bus_free(config->spi_host);
        return ret;
    }
    
    // 4. 复位 LCD
    esp_lcd_panel_reset(*out_panel);
    
    // 5. 初始化 LCD
    esp_lcd_panel_init(*out_panel);
    
    // 6. 配置显示方向和镜像
    esp_lcd_panel_mirror(*out_panel, config->mirror_x, config->mirror_y);
    esp_lcd_panel_swap_xy(*out_panel, config->swap_xy);
    esp_lcd_panel_invert_color(*out_panel, config->invert_color);
    
    // 7. 设置偏移（如果需要）
    if (config->offset_x != 0 || config->offset_y != 0) {
        esp_lcd_panel_set_gap(*out_panel, config->offset_x, config->offset_y);
    }
    
    // 8. 打开显示
    esp_lcd_panel_disp_on_off(*out_panel, true);
    
    ESP_LOGI(TAG, "ST7789 LCD initialized successfully");
    
    return ESP_OK;
}
