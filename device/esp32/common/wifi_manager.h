/*
 * @Author: 星年 && j_xingnian@163.com
 * @Date: 2026-01-09 10:10:17
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2026-01-09 10:11:47
 * @FilePath: \xn_smart_dialogue_platform\device\esp32\common\wifi_manager.h
 * @Description: 
 * 
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
 */
/**
 * WiFi 连接管理
 * 
 * 功能说明：
 *   管理 ESP32 的 WiFi 连接。
 *   支持自动重连和连接状态监控。
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    /**
     * 初始化 WiFi 连接
     * 
     * 参数：
     *   ssid: WiFi 名称
     *   password: WiFi 密码
     *   timeout: 连接超时时间（毫秒），默认 30 秒
     * 
     * 返回：
     *   true: 连接成功
     *   false: 连接超时
     */
    static bool begin(const char* ssid, const char* password, unsigned long timeout = 30000);
    
    /**
     * 保持连接，需要在 loop() 中调用
     * 
     * 功能：
     *   检查连接状态，断开则自动重连
     */
    static void loop();
    
    /**
     * 检查是否已连接
     */
    static bool isConnected();
    
    /**
     * 获取本机 IP 地址
     */
    static String getIP();

private:
    static const char* _ssid;
    static const char* _password;
    static unsigned long _lastReconnectAttempt;
};

#endif // WIFI_MANAGER_H
