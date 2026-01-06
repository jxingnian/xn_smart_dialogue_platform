# API 参考文档

## API 概览

平台提供 RESTful API 和 WebSocket API 两种接入方式。

## 基础信息

- Base URL: `https://api.smart-dialogue.example.com/v1`
- WebSocket: `wss://ws.smart-dialogue.example.com/v1`
- 认证方式: Bearer Token

## RESTful API

### 设备管理

#### 注册设备
```http
POST /devices
Content-Type: application/json
Authorization: Bearer {token}

{
  "device_type": "humidifier",
  "device_name": "卧室加湿器",
  "capabilities": {...}
}
```

#### 获取设备列表
```http
GET /devices
Authorization: Bearer {token}
```

#### 获取设备状态
```http
GET /devices/{device_id}/status
Authorization: Bearer {token}
```

#### 控制设备
```http
POST /devices/{device_id}/commands
Content-Type: application/json
Authorization: Bearer {token}

{
  "action": "set_property",
  "properties": {
    "power": "on"
  }
}
```

### 音频处理

#### 上传音频进行处理
```http
POST /audio/process
Content-Type: multipart/form-data
Authorization: Bearer {token}

audio: (binary)
device_id: speaker_001
options: {"enable_emotion": true}
```

响应：
```json
{
  "audio_id": "uuid",
  "asr": {
    "text": "今天天气好干燥",
    "confidence": 0.95
  },
  "speaker": {
    "speaker_id": "user_001",
    "confidence": 0.92
  },
  "emotion": {
    "primary": "neutral",
    "confidence": 0.85
  }
}
```

### 意图处理

#### 处理文本意图
```http
POST /intent/process
Content-Type: application/json
Authorization: Bearer {token}

{
  "text": "把加湿器打开",
  "user_id": "user_001",
  "context": {
    "location": "bedroom",
    "previous_intents": []
  }
}
```

响应：
```json
{
  "intent_id": "uuid",
  "intent": {
    "type": "device_control",
    "action": "turn_on",
    "target_device": "humidifier"
  },
  "decision": {
    "strategy": "execute_and_inform",
    "actions": [...]
  }
}
```

### 用户管理

#### 注册声纹
```http
POST /users/{user_id}/voiceprint
Content-Type: multipart/form-data
Authorization: Bearer {token}

audio_samples: (binary[])
```

#### 获取用户偏好
```http
GET /users/{user_id}/preferences
Authorization: Bearer {token}
```

## WebSocket API

### 连接建立
```javascript
const ws = new WebSocket('wss://ws.smart-dialogue.example.com/v1');
ws.onopen = () => {
  ws.send(JSON.stringify({
    type: 'auth',
    token: 'your_token'
  }));
};
```

### 实时音频流
```javascript
// 发送音频数据
ws.send(JSON.stringify({
  type: 'audio_stream',
  device_id: 'speaker_001',
  audio: 'base64_encoded_chunk',
  sequence: 1
}));

// 接收处理结果
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  if (data.type === 'asr_result') {
    console.log('识别结果:', data.text);
  }
  if (data.type === 'intent_result') {
    console.log('意图:', data.intent);
  }
};
```

### 消息类型

| 类型 | 方向 | 描述 |
|------|------|------|
| auth | C→S | 认证 |
| audio_stream | C→S | 音频流 |
| asr_result | S→C | ASR结果 |
| intent_result | S→C | 意图结果 |
| device_command | S→C | 设备指令 |
| device_status | C→S | 设备状态 |

## 错误码

| 错误码 | 描述 |
|--------|------|
| 400 | 请求参数错误 |
| 401 | 未授权 |
| 403 | 权限不足 |
| 404 | 资源不存在 |
| 429 | 请求过于频繁 |
| 500 | 服务器内部错误 |

## 速率限制

- 普通接口: 100 次/分钟
- 音频处理: 30 次/分钟
- WebSocket: 无限制（需保持心跳）
