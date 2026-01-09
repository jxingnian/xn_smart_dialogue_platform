/**
 * 设备通用配置
 * 
 * 功能说明：
 *   定义所有设备共用的配置项。
 *   具体设备可以覆盖这些默认值。
 */

#ifndef CONFIG_H
#define CONFIG_H

// ========== WiFi 配置 ==========
// 在具体设备代码中定义，或使用配网功能
// #define WIFI_SSID "your-wifi-ssid"
// #define WIFI_PASSWORD "your-wifi-password"

// ========== MQTT 配置 ==========
// MQTT 服务器地址（知境中枢的地址）
#define MQTT_BROKER "192.168.1.100"
// MQTT 服务器端口
#define MQTT_PORT 1883
// MQTT 用户名（可选）
#define MQTT_USERNAME ""
// MQTT 密码（可选）
#define MQTT_PASSWORD ""

// ========== 心跳配置 ==========
// 心跳间隔（毫秒），默认 30 秒
#define HEARTBEAT_INTERVAL 30000

// ========== 状态上报配置 ==========
// 状态上报间隔（毫秒），默认 60 秒
#define STATUS_REPORT_INTERVAL 60000

// ========== 调试配置 ==========
// 是否开启串口调试输出
#define DEBUG_ENABLED true

// 调试输出宏
#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
