/**
 * 净化器设备固件
 * 
 * 功能说明：
 *   控制空气净化器的 ESP32 固件。
 *   支持的功能：
 *   - 开关控制
 *   - 档位调节（低/中/高/自动）
 *   - PM2.5 数据采集和上报
 *   - 滤芯状态监控
 * 
 * 硬件连接：
 *   - GPIO 2: 电源继电器
 *   - GPIO 4: 低档继电器
 *   - GPIO 5: 中档继电器
 *   - GPIO 18: 高档继电器
 *   - GPIO 34: PM2.5 传感器（模拟输入）
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../common/config.h"
#include "../common/wifi_manager.h"
#include "../common/mqtt_client.h"

// ========== 设备配置 ==========
// 设备唯一标识，每个设备必须不同
#define DEVICE_ID "purifier_001"
// 设备类型
#define DEVICE_TYPE "purifier"

// WiFi 配置（根据实际情况修改）
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// ========== 引脚定义 ==========
#define PIN_POWER 2       // 电源继电器
#define PIN_LOW_SPEED 4   // 低档
#define PIN_MID_SPEED 5   // 中档
#define PIN_HIGH_SPEED 18 // 高档
#define PIN_PM25_SENSOR 34 // PM2.5 传感器

// ========== 设备状态 ==========
// 电源状态：true=开启，false=关闭
bool powerOn = false;
// 当前档位：off/low/mid/high/auto
String currentMode = "off";
// PM2.5 数值
int pm25Value = 0;
// 滤芯使用时长（小时）
int filterHours = 0;

// ========== 全局对象 ==========
MqttClientWrapper mqtt(DEVICE_ID, DEVICE_TYPE);
unsigned long lastStatusReport = 0;

/**
 * 设置档位
 * 
 * 参数：
 *   mode: 档位名称（off/low/mid/high）
 */
void setMode(const String& mode) {
    // 先关闭所有档位
    digitalWrite(PIN_LOW_SPEED, LOW);
    digitalWrite(PIN_MID_SPEED, LOW);
    digitalWrite(PIN_HIGH_SPEED, LOW);
    
    // 设置对应档位
    if (mode == "low") {
        digitalWrite(PIN_LOW_SPEED, HIGH);
    } else if (mode == "mid") {
        digitalWrite(PIN_MID_SPEED, HIGH);
    } else if (mode == "high") {
        digitalWrite(PIN_HIGH_SPEED, HIGH);
    }
    
    currentMode = mode;
    DEBUG_PRINTF("档位设置为: %s\n", mode.c_str());
}

/**
 * 开机
 */
void powerOnDevice() {
    digitalWrite(PIN_POWER, HIGH);
    powerOn = true;
    setMode("low");  // 默认低档
    DEBUG_PRINTLN("净化器已开启");
}

/**
 * 关机
 */
void powerOffDevice() {
    setMode("off");
    digitalWrite(PIN_POWER, LOW);
    powerOn = false;
    DEBUG_PRINTLN("净化器已关闭");
}

/**
 * 读取 PM2.5 数值
 * 
 * 返回：
 *   PM2.5 浓度（μg/m³）
 */
int readPM25() {
    // 读取模拟值并转换为 PM2.5 浓度
    // 具体转换公式根据传感器型号调整
    int rawValue = analogRead(PIN_PM25_SENSOR);
    int pm25 = map(rawValue, 0, 4095, 0, 500);
    return pm25;
}

/**
 * 自动档位调节
 * 
 * 根据 PM2.5 数值自动调节档位：
 *   - PM2.5 < 35: 低档
 *   - PM2.5 35-75: 中档
 *   - PM2.5 > 75: 高档
 */
void autoAdjustMode() {
    if (!powerOn || currentMode != "auto") return;
    
    if (pm25Value < 35) {
        setMode("low");
    } else if (pm25Value < 75) {
        setMode("mid");
    } else {
        setMode("high");
    }
}

/**
 * 上报设备状态
 */
void reportStatus() {
    JsonDocument doc;
    doc["power"] = powerOn;
    doc["mode"] = currentMode;
    doc["pm25"] = pm25Value;
    doc["filter_hours"] = filterHours;
    
    mqtt.reportStatus(doc);
    DEBUG_PRINTLN("状态已上报");
}

/**
 * 处理控制命令
 * 
 * 支持的命令：
 *   - power_on: 开机
 *   - power_off: 关机
 *   - set_mode: 设置档位，参数 mode=low/mid/high/auto
 */
void handleControl(const char* topic, JsonDocument& doc) {
    String command = doc["command"].as<String>();
    DEBUG_PRINTF("收到控制命令: %s\n", command.c_str());
    
    if (command == "power_on") {
        powerOnDevice();
    } else if (command == "power_off") {
        powerOffDevice();
    } else if (command == "set_mode") {
        String mode = doc["params"]["mode"].as<String>();
        if (powerOn) {
            setMode(mode);
        }
    }
    
    // 命令执行后立即上报状态
    reportStatus();
}

/**
 * 初始化
 */
void setup() {
    // 初始化串口
    Serial.begin(115200);
    DEBUG_PRINTLN("净化器启动中...");
    
    // 初始化引脚
    pinMode(PIN_POWER, OUTPUT);
    pinMode(PIN_LOW_SPEED, OUTPUT);
    pinMode(PIN_MID_SPEED, OUTPUT);
    pinMode(PIN_HIGH_SPEED, OUTPUT);
    pinMode(PIN_PM25_SENSOR, INPUT);
    
    // 默认关闭所有输出
    digitalWrite(PIN_POWER, LOW);
    digitalWrite(PIN_LOW_SPEED, LOW);
    digitalWrite(PIN_MID_SPEED, LOW);
    digitalWrite(PIN_HIGH_SPEED, LOW);
    
    // 连接 WiFi
    DEBUG_PRINTLN("连接 WiFi...");
    if (WiFiManager::begin(WIFI_SSID, WIFI_PASSWORD)) {
        DEBUG_PRINTF("WiFi 已连接，IP: %s\n", WiFiManager::getIP().c_str());
    } else {
        DEBUG_PRINTLN("WiFi 连接失败！");
    }
    
    // 初始化 MQTT
    mqtt.begin(MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);
    mqtt.onControl(handleControl);
    
    DEBUG_PRINTLN("净化器启动完成");
}

/**
 * 主循环
 */
void loop() {
    // 保持 WiFi 连接
    WiFiManager::loop();
    
    // 保持 MQTT 连接
    mqtt.loop();
    
    // 读取 PM2.5
    pm25Value = readPM25();
    
    // 自动档位调节
    autoAdjustMode();
    
    // 定时上报状态
    if (millis() - lastStatusReport > STATUS_REPORT_INTERVAL) {
        reportStatus();
        lastStatusReport = millis();
    }
    
    delay(100);
}
