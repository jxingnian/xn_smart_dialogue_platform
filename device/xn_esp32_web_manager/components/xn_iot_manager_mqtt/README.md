# iot_manager_mqtt 组件

封装 ESP-IDF `esp-mqtt` 客户端，提供简化的 MQTT 管理能力：

- 底层 MQTT 客户端封装（连接、重连、订阅、发布）
- Web MQTT 管理器（`web_mqtt_manager`），统一管理 MQTT 连接状态

## 组件结构

- `mqtt_module`: 底层 MQTT 客户端封装
- `web_mqtt_manager`: 上层管理器，提供状态机和自动重连功能

## 使用方法

```c
web_mqtt_manager_config_t cfg = WEB_MQTT_MANAGER_DEFAULT_CONFIG();
cfg.broker_uri = "mqtt://192.168.1.10:1883";
cfg.client_id = "ESP32_DEVICE_001";
cfg.base_topic = "xn/esp";
cfg.event_cb = my_mqtt_event_callback;

web_mqtt_manager_init(&cfg);
```

