# ESP32 设备端代码

## 目录结构

```
esp32/
├── common/                 # 通用模块
│   ├── config.h           # 配置定义
│   ├── wifi_manager.h     # WiFi 连接管理
│   └── mqtt_client.h      # MQTT 客户端封装
├── purifier/              # 净化器固件
│   └── purifier.ino
├── fish_feeder/           # 喂鱼器固件
│   └── fish_feeder.ino
└── README.md
```

## 开发环境

- Arduino IDE 2.x 或 PlatformIO
- ESP32 开发板（推荐 ESP32-WROOM-32）
- 依赖库：
  - PubSubClient（MQTT 客户端）
  - ArduinoJson（JSON 解析）

## 配置说明

1. 修改 `common/config.h` 中的 MQTT 服务器地址
2. 在具体设备代码中修改 WiFi 配置和设备 ID
3. 根据实际硬件连接修改引脚定义

## MQTT 通信协议

### 主题格式

| 主题 | 方向 | 说明 |
|------|------|------|
| `device/{device_id}/status` | 设备 → 服务器 | 状态上报 |
| `device/{device_id}/control` | 服务器 → 设备 | 控制命令 |
| `device/{device_id}/heartbeat` | 设备 → 服务器 | 心跳包 |

### 状态上报格式

净化器：
```json
{
  "power": true,
  "mode": "auto",
  "pm25": 35,
  "filter_hours": 1200
}
```

喂鱼器：
```json
{
  "last_feed_time": 1704787200,
  "food_level": 80,
  "feed_count_today": 2,
  "schedules": [
    {"hour": 8, "minute": 0, "duration": 2000},
    {"hour": 18, "minute": 0, "duration": 2000}
  ]
}
```

### 控制命令格式

```json
{
  "command": "power_on",
  "params": {}
}
```

```json
{
  "command": "set_mode",
  "params": {
    "mode": "high"
  }
}
```

## 烧录步骤

1. 使用 USB 线连接 ESP32 开发板
2. 在 Arduino IDE 中选择正确的开发板和端口
3. 修改配置（WiFi、MQTT、设备 ID）
4. 点击上传按钮烧录固件
5. 打开串口监视器查看运行日志
