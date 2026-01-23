/*
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 12:19:09
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-22 20:06:08
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\main\main.c
 * @Description: AI语音对话项目 esp32设备端 应用程序入口
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "xn_event_bus.h"
#include "app_state_machine.h"
#include "managers/wifi_manager.h"
#include "managers/mqtt_manager.h"
#include "managers/mqtt_manager.h"
#include "managers/blufi_manager.h"
#include "managers/button_manager.h"

// 模块日志标签
static const char *TAG = "main";

/**
 * @brief 初始化非易失性存储(NVS)
 * 
 * NVS用于存储WiFi配置、MQTT配置等掉电保存参数。
 * 如果NVS分区满或版本不匹配，会自动擦除重置。
 * 
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
static esp_err_t init_nvs(void)
{
    // 尝试初始化NVS Flash
    esp_err_t ret = nvs_flash_init();
    
    // 如果没有空闲页或发现新版本，需要擦除NVS分区后重试
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 打印警告日志，提示正在擦除NVS
        ESP_LOGW(TAG, "NVS flash erase and init...");
        // 擦除NVS分区
        ESP_ERROR_CHECK(nvs_flash_erase());
        // 再次尝试初始化NVS
        ret = nvs_flash_init();
    }
    // 返回初始化结果
    return ret;
}

/**
 * @brief 应用程序主入口
 */
void app_main(void)
{
    // 打印系统启动分割线日志
    ESP_LOGI(TAG, "========================================");
    // 打印系统启动信息日志
    ESP_LOGI(TAG, "  XN Smart Dialogue Platform Starting");
    // 打印系统启动分割线日志
    ESP_LOGI(TAG, "========================================");
    
    // 初始化NVS子系统
    ESP_ERROR_CHECK(init_nvs());
    // 打印NVS初始化成功日志
    ESP_LOGI(TAG, "NVS initialized");
    
    // 创建系统默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 打印事件循环创建成功日志
    ESP_LOGI(TAG, "Event loop created");
    
    // 初始化自定义事件总线
    ESP_ERROR_CHECK(xn_event_bus_init());
    // 打印事件总线初始化成功日志
    ESP_LOGI(TAG, "Event bus initialized");
    
    // 初始化WiFi管理器
    ESP_ERROR_CHECK(wifi_manager_init());
    // 打印WiFi管理器初始化成功日志
    ESP_LOGI(TAG, "WiFi manager initialized");
    
    // 初始化MQTT管理器
    ESP_ERROR_CHECK(mqtt_manager_init());
    // 打印MQTT管理器初始化成功日志
    ESP_LOGI(TAG, "MQTT manager initialized");
    
    // 初始化BluFi配网管理器
    ESP_ERROR_CHECK(blufi_manager_init());
    // 打印BluFi管理器初始化成功日志
    ESP_LOGI(TAG, "BluFi manager initialized");

    // 初始化按键管理器
    ESP_ERROR_CHECK(button_manager_init());
    
    // 初始化应用状态机
    ESP_ERROR_CHECK(app_state_machine_init());
    // 启动应用状态机
    ESP_ERROR_CHECK(app_state_machine_start());
    // 打印状态机启动成功日志
    ESP_LOGI(TAG, "App state machine started");
    
    // 打印系统初始化完成分割线日志
    ESP_LOGI(TAG, "========================================");
    // 打印系统初始化完成日志
    ESP_LOGI(TAG, "  System initialization complete!");
    // 打印系统初始化完成分割线日志
    ESP_LOGI(TAG, "========================================");
    
    // 进入主循环
    while (1) {
        // 延时10秒
        vTaskDelay(pdMS_TO_TICKS(10000));
        // 获取并打印当前应用状态机所处状态
        ESP_LOGI(TAG, "System state: %s", app_state_machine_get_state_name());
    }
}

