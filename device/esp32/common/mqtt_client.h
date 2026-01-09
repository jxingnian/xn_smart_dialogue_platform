/**
 * MQTT 客户端封装
 * 
 * 功能说明：
 *   封装 MQTT 连接和消息收发功能。
 *   所有设备都通过 MQTT 与知境中枢通信。
 * 
 * 通信协议：
 *   - 设备上报主题：device/{device_id}/status
 *   - 设备控制主题：device/{device_id}/control
 *   - 心跳主题：device/{device_id}/heartbeat
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// MQTT 消息回调函数类型
// 参数：主题、消息内容（JSON 格式）
typedef void (*MqttMessageCallback)(const char* topic, JsonDocument& doc);

class MqttClientWrapper {
public:
    /**
     * 构造函数
     * 
     * 参数：
     *   deviceId: 设备唯一标识，用于构建 MQTT 主题
     *   deviceType: 设备类型，如 purifier、fish_feeder
     */
    MqttClientWrapper(const char* deviceId, const char* deviceType);
    
    /**
     * 初始化 MQTT 连接
     * 
     * 参数：
     *   broker: MQTT 服务器地址
     *   port: MQTT 服务器端口，默认 1883
     *   username: 用户名，可选
     *   password: 密码，可选
     */
    void begin(const char* broker, int port = 1883, 
               const char* username = nullptr, const char* password = nullptr);
    
    /**
     * 保持连接，需要在 loop() 中调用
     * 
     * 功能：
     *   - 检查连接状态，断开则自动重连
     *   - 处理接收到的消息
     *   - 发送心跳包
     */
    void loop();
    
    /**
     * 上报设备状态
     * 
     * 参数：
     *   status: 状态数据（JSON 格式）
     * 
     * 示例：
     *   JsonDocument doc;
     *   doc["power"] = true;
     *   doc["mode"] = "auto";
     *   mqtt.reportStatus(doc);
     */
    void reportStatus(JsonDocument& status);
    
    /**
     * 设置控制命令回调
     * 
     * 当收到控制命令时，会调用此回调函数。
     * 设备需要在回调中处理具体的控制逻辑。
     */
    void onControl(MqttMessageCallback callback);
    
    /**
     * 检查是否已连接
     */
    bool isConnected();

private:
    const char* _deviceId;      // 设备 ID
    const char* _deviceType;    // 设备类型
    WiFiClient _wifiClient;     // WiFi 客户端
    PubSubClient _mqttClient;   // MQTT 客户端
    MqttMessageCallback _controlCallback;  // 控制命令回调
    unsigned long _lastHeartbeat;  // 上次心跳时间
    
    // MQTT 主题
    char _statusTopic[64];      // 状态上报主题
    char _controlTopic[64];     // 控制命令主题
    char _heartbeatTopic[64];   // 心跳主题
    
    // 连接到 MQTT 服务器
    void connect();
    
    // 发送心跳
    void sendHeartbeat();
    
    // 静态消息回调（PubSubClient 要求）
    static void messageCallback(char* topic, byte* payload, unsigned int length);
};

#endif // MQTT_CLIENT_H
