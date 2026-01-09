/**
 * 喂鱼器设备固件
 * 
 * 功能说明：
 *   控制自动喂鱼器的 ESP32 固件。
 *   支持的功能：
 *   - 手动投喂
 *   - 定时投喂（支持多个时间点）
 *   - 投喂量调节
 *   - 余粮检测
 * 
 * 硬件连接：
 *   - GPIO 2: 投喂电机
 *   - GPIO 34: 余粮检测传感器（模拟输入）
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../common/config.h"
#include "../common/wifi_manager.h"
#include "../common/mqtt_client.h"

// ========== 设备配置 ==========
// 设备唯一标识，每个设备必须不同
#define DEVICE_ID "fish_feeder_001"
// 设备类型
#define DEVICE_TYPE "fish_feeder"

// WiFi 配置（根据实际情况修改）
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// ========== 引脚定义 ==========
#define PIN_MOTOR 2           // 投喂电机
#define PIN_FOOD_SENSOR 34    // 余粮传感器

// ========== 投喂配置 ==========
// 每次投喂的默认时长（毫秒），控制投喂量
#define DEFAULT_FEED_DURATION 2000
// 最大投喂时长
#define MAX_FEED_DURATION 5000
// 最小投喂时长
#define MIN_FEED_DURATION 500

// ========== 设备状态 ==========
// 上次投喂时间（时间戳）
unsigned long lastFeedTime = 0;
// 余粮百分比（0-100）
int foodLevel = 100;
// 今日投喂次数
int feedCountToday = 0;
// 当前投喂量设置（毫秒）
int feedDuration = DEFAULT_FEED_DURATION;

// ========== 定时投喂配置 ==========
// 最多支持 5 个定时任务
#define MAX_SCHEDULES 5
struct FeedSchedule {
    bool enabled;       // 是否启用
    int hour;           // 小时（0-23）
    int minute;         // 分钟（0-59）
    int duration;       // 投喂时长（毫秒）
};
FeedSchedule schedules[MAX_SCHEDULES];

// ========== 全局对象 ==========
MqttClientWrapper mqtt(DEVICE_ID, DEVICE_TYPE);
unsigned long lastStatusReport = 0;
unsigned long lastScheduleCheck = 0;

/**
 * 执行投喂
 * 
 * 参数：
 *   duration: 投喂时长（毫秒），控制投喂量
 */
void feed(int duration = DEFAULT_FEED_DURATION) {
    // 限制投喂时长范围
    duration = constrain(duration, MIN_FEED_DURATION, MAX_FEED_DURATION);
    
    DEBUG_PRINTF("开始投喂，时长: %d ms\n", duration);
    
    // 启动电机
    digitalWrite(PIN_MOTOR, HIGH);
    delay(duration);
    digitalWrite(PIN_MOTOR, LOW);
    
    // 更新状态
    lastFeedTime = millis();
    feedCountToday++;
    
    DEBUG_PRINTLN("投喂完成");
    
    // 投喂后立即上报状态
    reportStatus();
}

/**
 * 读取余粮百分比
 * 
 * 返回：
 *   余粮百分比（0-100）
 */
int readFoodLevel() {
    // 读取模拟值并转换为百分比
    // 具体转换根据传感器类型调整
    int rawValue = analogRead(PIN_FOOD_SENSOR);
    int level = map(rawValue, 0, 4095, 0, 100);
    return constrain(level, 0, 100);
}

/**
 * 上报设备状态
 */
void reportStatus() {
    JsonDocument doc;
    doc["last_feed_time"] = lastFeedTime;
    doc["food_level"] = foodLevel;
    doc["feed_count_today"] = feedCountToday;
    doc["feed_duration"] = feedDuration;
    
    // 添加定时任务信息
    JsonArray schedulesArray = doc["schedules"].to<JsonArray>();
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        if (schedules[i].enabled) {
            JsonObject s = schedulesArray.add<JsonObject>();
            s["hour"] = schedules[i].hour;
            s["minute"] = schedules[i].minute;
            s["duration"] = schedules[i].duration;
        }
    }
    
    mqtt.reportStatus(doc);
    DEBUG_PRINTLN("状态已上报");
}

/**
 * 处理控制命令
 * 
 * 支持的命令：
 *   - feed: 手动投喂，可选参数 duration
 *   - set_duration: 设置默认投喂量
 *   - add_schedule: 添加定时任务
 *   - remove_schedule: 删除定时任务
 *   - clear_schedules: 清空所有定时任务
 */
void handleControl(const char* topic, JsonDocument& doc) {
    String command = doc["command"].as<String>();
    DEBUG_PRINTF("收到控制命令: %s\n", command.c_str());
    
    if (command == "feed") {
        // 手动投喂
        int duration = doc["params"]["duration"] | feedDuration;
        feed(duration);
        
    } else if (command == "set_duration") {
        // 设置默认投喂量
        feedDuration = doc["params"]["duration"] | DEFAULT_FEED_DURATION;
        feedDuration = constrain(feedDuration, MIN_FEED_DURATION, MAX_FEED_DURATION);
        DEBUG_PRINTF("投喂量设置为: %d ms\n", feedDuration);
        reportStatus();
        
    } else if (command == "add_schedule") {
        // 添加定时任务
        int hour = doc["params"]["hour"];
        int minute = doc["params"]["minute"];
        int duration = doc["params"]["duration"] | feedDuration;
        
        // 找到空闲槽位
        for (int i = 0; i < MAX_SCHEDULES; i++) {
            if (!schedules[i].enabled) {
                schedules[i].enabled = true;
                schedules[i].hour = hour;
                schedules[i].minute = minute;
                schedules[i].duration = duration;
                DEBUG_PRINTF("添加定时任务: %02d:%02d\n", hour, minute);
                break;
            }
        }
        reportStatus();
        
    } else if (command == "remove_schedule") {
        // 删除定时任务
        int hour = doc["params"]["hour"];
        int minute = doc["params"]["minute"];
        
        for (int i = 0; i < MAX_SCHEDULES; i++) {
            if (schedules[i].enabled && 
                schedules[i].hour == hour && 
                schedules[i].minute == minute) {
                schedules[i].enabled = false;
                DEBUG_PRINTF("删除定时任务: %02d:%02d\n", hour, minute);
                break;
            }
        }
        reportStatus();
        
    } else if (command == "clear_schedules") {
        // 清空所有定时任务
        for (int i = 0; i < MAX_SCHEDULES; i++) {
            schedules[i].enabled = false;
        }
        DEBUG_PRINTLN("已清空所有定时任务");
        reportStatus();
    }
}

/**
 * 检查定时任务
 * 
 * 注意：需要配合 NTP 时间同步使用
 * 这里简化处理，实际使用需要获取真实时间
 */
void checkSchedules() {
    // TODO: 获取当前时间并检查定时任务
    // 需要添加 NTP 时间同步功能
}

/**
 * 初始化
 */
void setup() {
    // 初始化串口
    Serial.begin(115200);
    DEBUG_PRINTLN("喂鱼器启动中...");
    
    // 初始化引脚
    pinMode(PIN_MOTOR, OUTPUT);
    pinMode(PIN_FOOD_SENSOR, INPUT);
    
    // 默认关闭电机
    digitalWrite(PIN_MOTOR, LOW);
    
    // 初始化定时任务
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        schedules[i].enabled = false;
    }
    
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
    
    DEBUG_PRINTLN("喂鱼器启动完成");
}

/**
 * 主循环
 */
void loop() {
    // 保持 WiFi 连接
    WiFiManager::loop();
    
    // 保持 MQTT 连接
    mqtt.loop();
    
    // 读取余粮
    foodLevel = readFoodLevel();
    
    // 余粮不足告警（低于 20%）
    if (foodLevel < 20) {
        DEBUG_PRINTLN("警告：余粮不足！");
        // TODO: 发送告警通知
    }
    
    // 检查定时任务（每分钟检查一次）
    if (millis() - lastScheduleCheck > 60000) {
        checkSchedules();
        lastScheduleCheck = millis();
    }
    
    // 定时上报状态
    if (millis() - lastStatusReport > STATUS_REPORT_INTERVAL) {
        reportStatus();
        lastStatusReport = millis();
    }
    
    delay(100);
}
