# 设备接入指南

## 概述

本文档描述智能设备如何接入对话平台，包括设备注册、能力声明、数据上报和指令接收。

## 设备分类体系

### 按功能分类

```
智能设备
├── 环境控制类
│   ├── 空调 (AC)
│   ├── 加湿器 (Humidifier)
│   ├── 除湿机 (Dehumidifier)
│   ├── 空气净化器 (Air Purifier)
│   ├── 新风系统 (Fresh Air System)
│   └── 暖风机 (Heater)
│
├── 照明类
│   ├── 智能灯泡 (Smart Bulb)
│   ├── 灯带 (Light Strip)
│   ├── 吸顶灯 (Ceiling Light)
│   └── 台灯 (Desk Lamp)
│
├── 影音娱乐类
│   ├── 智能音箱 (Smart Speaker)
│   ├── 智能电视 (Smart TV)
│   ├── 投影仪 (Projector)
│   └── 音响系统 (Audio System)
│
├── 安防类
│   ├── 摄像头 (Camera)
│   ├── 门锁 (Door Lock)
│   ├── 门窗传感器 (Door/Window Sensor)
│   ├── 烟雾报警器 (Smoke Detector)
│   └── 人体传感器 (Motion Sensor)
│
├── 家电类
│   ├── 扫地机器人 (Robot Vacuum)
│   ├── 洗衣机 (Washing Machine)
│   ├── 冰箱 (Refrigerator)
│   ├── 微波炉 (Microwave)
│   └── 电饭煲 (Rice Cooker)
│
├── 窗帘/遮阳类
│   ├── 电动窗帘 (Smart Curtain)
│   └── 智能百叶窗 (Smart Blinds)
│
└── 传感器类
    ├── 温湿度传感器 (Temp/Humidity Sensor)
    ├── 光照传感器 (Light Sensor)
    ├── 空气质量传感器 (Air Quality Sensor)
    └── 噪音传感器 (Noise Sensor)
```

### 按交互能力分类

| 类型 | 描述 | 示例 |
|------|------|------|
| 音频输入设备 | 可采集音频 | 音箱、电视、专用麦克风 |
| 音频输出设备 | 可播放音频 | 音箱、电视、耳机 |
| 执行设备 | 可执行控制指令 | 空调、灯、窗帘 |
| 传感设备 | 提供环境数据 | 温湿度传感器 |
| 复合设备 | 多种能力组合 | 带屏音箱 |

## 设备能力模型

### 能力定义规范

```json
{
  "device_id": "device_unique_id",
  "device_type": "humidifier",
  "device_name": "卧室加湿器",
  "manufacturer": "brand_name",
  "model": "model_number",
  "location": {
    "room": "bedroom",
    "zone": "bedside"
  },
  "capabilities": {
    "power": {
      "type": "switch",
      "actions": ["turn_on", "turn_off", "toggle"],
      "readable": true,
      "writable": true
    },
    "humidity_target": {
      "type": "range",
      "min": 30,
      "max": 80,
      "step": 5,
      "unit": "%",
      "readable": true,
      "writable": true
    },
    "mode": {
      "type": "enum",
      "values": ["auto", "sleep", "continuous"],
      "readable": true,
      "writable": true
    },
    "water_level": {
      "type": "range",
      "min": 0,
      "max": 100,
      "unit": "%",
      "readable": true,
      "writable": false
    }
  },
  "sensors": {
    "current_humidity": {
      "type": "number",
      "unit": "%",
      "report_interval": 60
    }
  },
  "audio_capabilities": {
    "input": false,
    "output": false
  }
}
```

### 标准能力类型

| 能力类型 | 描述 | 参数 |
|----------|------|------|
| switch | 开关控制 | on/off |
| range | 范围调节 | min, max, step, unit |
| enum | 枚举选择 | values[] |
| color | 颜色控制 | rgb/hsv/temperature |
| position | 位置控制 | 0-100% |
| timer | 定时控制 | duration/schedule |

## 接入协议

### 设备注册流程

```
设备 ──────────────────────────────────────────────► 平台
  │                                                    │
  │  1. 发送注册请求 (设备信息 + 能力声明)              │
  │ ─────────────────────────────────────────────────► │
  │                                                    │
  │  2. 返回注册结果 (device_token + config)           │
  │ ◄───────────────────────────────────────────────── │
  │                                                    │
  │  3. 建立长连接 (WebSocket/MQTT)                    │
  │ ◄────────────────────────────────────────────────► │
  │                                                    │
  │  4. 定期心跳 + 状态上报                            │
  │ ─────────────────────────────────────────────────► │
```

### 通信协议

支持以下协议接入：
- MQTT (推荐，适合IoT设备)
- WebSocket (适合富客户端)
- HTTP/REST (适合简单场景)

### MQTT Topic 规范

```
# 设备上报
devices/{device_id}/status      # 状态上报
devices/{device_id}/audio       # 音频数据上报
devices/{device_id}/events      # 事件上报

# 平台下发
devices/{device_id}/commands    # 控制指令
devices/{device_id}/config      # 配置更新
```

## 数据上报

### 音频数据上报

```json
{
  "device_id": "speaker_001",
  "timestamp": "2024-01-01T12:00:00Z",
  "audio": {
    "format": "pcm",
    "sample_rate": 16000,
    "channels": 1,
    "bits_per_sample": 16,
    "data": "base64_encoded_audio_data"
  },
  "metadata": {
    "vad_detected": true,
    "local_asr_text": "今天天气好干燥",
    "local_emotion": "neutral"
  }
}
```

### 状态数据上报

```json
{
  "device_id": "humidifier_001",
  "timestamp": "2024-01-01T12:00:00Z",
  "status": {
    "power": "on",
    "mode": "auto",
    "humidity_target": 50,
    "water_level": 75,
    "current_humidity": 45
  },
  "health": {
    "online": true,
    "signal_strength": -45,
    "error_code": null
  }
}
```

## 指令接收

### 控制指令格式

```json
{
  "command_id": "cmd_uuid",
  "timestamp": "2024-01-01T12:00:00Z",
  "device_id": "humidifier_001",
  "action": "set_property",
  "properties": {
    "power": "on",
    "humidity_target": 55
  },
  "source": {
    "type": "voice_command",
    "user_id": "user_001",
    "intent_id": "intent_uuid"
  },
  "options": {
    "timeout_ms": 5000,
    "retry_count": 3
  }
}
```

### 指令响应格式

```json
{
  "command_id": "cmd_uuid",
  "device_id": "humidifier_001",
  "status": "success",
  "result": {
    "power": "on",
    "humidity_target": 55
  },
  "execution_time_ms": 120
}
```

## SDK 支持

### 支持的平台

| 平台 | SDK | 状态 |
|------|-----|------|
| Linux/ARM | C SDK | 可用 |
| Android | Java/Kotlin SDK | 可用 |
| iOS | Swift SDK | 可用 |
| ESP32 | C SDK | 可用 |
| Web | JavaScript SDK | 可用 |

### SDK 快速开始

```python
# Python 示例
from smart_dialogue_sdk import Device, Capability

# 定义设备能力
capabilities = {
    "power": Capability.Switch(),
    "brightness": Capability.Range(0, 100, unit="%"),
    "color_temp": Capability.Range(2700, 6500, unit="K")
}

# 创建设备实例
device = Device(
    device_type="smart_bulb",
    device_name="客厅主灯",
    capabilities=capabilities
)

# 注册到平台
device.register(
    platform_url="mqtt://platform.example.com",
    device_token="your_device_token"
)

# 处理控制指令
@device.on_command
def handle_command(cmd):
    if cmd.action == "set_property":
        # 执行设备控制
        result = execute_control(cmd.properties)
        return {"status": "success", "result": result}

# 上报状态
device.report_status({
    "power": "on",
    "brightness": 80,
    "color_temp": 4000
})
```

## 设备认证

### 认证方式

1. 设备证书认证 (推荐)
2. Token 认证
3. OAuth 2.0 (第三方设备)

### 安全要求

- 所有通信必须使用 TLS 加密
- 设备 Token 定期轮换
- 敏感操作需要用户确认
