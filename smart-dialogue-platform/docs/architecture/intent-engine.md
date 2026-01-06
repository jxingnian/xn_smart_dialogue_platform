# 意图识别引擎

## 模块概述

意图识别引擎负责理解用户真实需求，结合设备能力做出智能决策，决定是主动执行、询问确认还是提供建议。

## 处理流程

```
场景分析结果 + ASR文本 + 用户上下文
                │
                ▼
┌───────────────────────────────────────┐
│           意图理解层                   │
├───────────────────────────────────────┤
│  • 显式意图识别                        │
│  • 隐式需求挖掘                        │
│  • 多轮对话理解                        │
└─────────────────┬─────────────────────┘
                  │
                  ▼
┌───────────────────────────────────────┐
│           设备能力匹配                 │
├───────────────────────────────────────┤
│  • 可用设备查询                        │
│  • 功能能力匹配                        │
│  • 设备状态检查                        │
└─────────────────┬─────────────────────┘
                  │
                  ▼
┌───────────────────────────────────────┐
│           决策引擎                     │
├───────────────────────────────────────┤
│  • 执行策略选择                        │
│  • 响应内容生成                        │
│  • 多设备协同编排                      │
└─────────────────┬─────────────────────┘
                  │
                  ▼
            执行/响应输出
```

## 意图分类体系

### 显式意图 (Explicit Intent)
用户明确表达的需求

| 意图类型 | 描述 | 示例 |
|----------|------|------|
| 设备控制 | 控制设备开关/调节 | "把空调调到26度" |
| 信息查询 | 查询信息 | "现在几点了" |
| 内容播放 | 播放音视频 | "播放周杰伦的歌" |
| 定时任务 | 设置提醒/定时 | "10分钟后提醒我" |
| 场景执行 | 执行预设场景 | "开启回家模式" |

### 隐式意图 (Implicit Intent)
从对话中推断的潜在需求

| 意图类型 | 触发条件 | 示例 |
|----------|----------|------|
| 环境调节 | 提及环境不适 | "好干燥啊" → 加湿器 |
| 氛围营造 | 提及心情/场景 | "想放松一下" → 灯光+音乐 |
| 安全提醒 | 检测到风险 | "我要出门了" → 检查门窗 |
| 健康关怀 | 异常行为模式 | 深夜活动 → 关心提醒 |

## 决策策略

### 决策矩阵

```
                    意图明确度
                 低 ←────────→ 高
              ┌─────────┬─────────┐
         高   │  询问   │  执行   │
    置信度    │  确认   │  并告知 │
              ├─────────┼─────────┤
         低   │  忽略   │  建议   │
              │  或观察 │  询问   │
              └─────────┴─────────┘
```

### 决策类型

#### 1. 直接执行 (Direct Execute)
- 条件：高置信度 + 明确意图 + 低风险操作
- 示例："把灯关了" → 直接关灯

#### 2. 执行并告知 (Execute & Inform)
- 条件：高置信度 + 需要反馈
- 示例："空调调到26度" → 执行并语音确认

#### 3. 询问确认 (Ask Confirmation)
- 条件：中等置信度 或 高风险操作
- 示例："检测到您提到天气干燥，需要我打开加湿器吗？"

#### 4. 主动建议 (Proactive Suggestion)
- 条件：隐式意图 + 相关设备可用
- 示例："您好像有点累了，要不要调暗灯光？"

#### 5. 静默观察 (Silent Observe)
- 条件：低置信度 + 非人机对话
- 行为：记录上下文，不做响应

## 核心场景示例

### 场景1：环境调节

```yaml
input:
  text: "今天天气好干燥啊"
  scene: "human_human"  # 用户在和家人聊天
  speaker: "user_001"

analysis:
  explicit_intent: null
  implicit_intent: 
    type: "environment_adjustment"
    target: "humidity"
    sentiment: "complaint"

device_check:
  humidifier:
    available: true
    status: "off"
    location: "living_room"
  environment:
    humidity: 35%
    temperature: 24°C

decision:
  strategy: "proactive_suggestion"
  reason: "隐式需求 + 设备可用 + 环境数据支持"
  
response:
  type: "voice"
  content: "检测到室内湿度只有35%，需要我帮您打开加湿器吗？"
  wait_for_confirmation: true
```

### 场景2：多设备协同

```yaml
input:
  text: "我要睡觉了"
  scene: "human_device"
  speaker: "user_001"

analysis:
  explicit_intent:
    type: "scene_trigger"
    scene: "sleep_mode"

device_orchestration:
  - device: "lights"
    action: "turn_off"
    rooms: ["living_room", "kitchen"]
  - device: "bedroom_light"
    action: "dim"
    value: 10%
  - device: "curtains"
    action: "close"
  - device: "ac"
    action: "set_mode"
    value: "sleep"

decision:
  strategy: "execute_and_inform"
  
response:
  content: "好的，已为您开启睡眠模式，晚安~"
```

### 场景3：上下文理解

```yaml
# 第一轮对话
input_1:
  text: "明天早上有什么安排"
  
response_1:
  content: "明天早上8点有一个会议提醒"

# 第二轮对话
input_2:
  text: "那帮我定个7点的闹钟"
  
analysis:
  context_reference: "明天早上"
  resolved_time: "明天 07:00"
  
response_2:
  content: "好的，已为您设置明天早上7点的闹钟"
```

## 输出数据结构

```json
{
  "intent_id": "uuid",
  "timestamp": "2024-01-01T12:00:00Z",
  "input": {
    "text": "今天天气好干燥啊",
    "audio_id": "audio_uuid",
    "scene_id": "scene_uuid"
  },
  "intent": {
    "type": "implicit",
    "category": "environment_adjustment",
    "target": "humidity",
    "confidence": 0.75,
    "entities": {
      "condition": "dry",
      "time": "today"
    }
  },
  "device_match": {
    "matched_devices": [
      {
        "device_id": "humidifier_001",
        "device_type": "humidifier",
        "capability": "humidity_control",
        "current_state": "off",
        "match_score": 0.9
      }
    ],
    "environment_data": {
      "humidity": 35,
      "temperature": 24
    }
  },
  "decision": {
    "strategy": "proactive_suggestion",
    "actions": [
      {
        "type": "voice_response",
        "content": "检测到室内湿度只有35%，需要我帮您打开加湿器吗？",
        "wait_confirmation": true
      }
    ],
    "pending_actions": [
      {
        "device_id": "humidifier_001",
        "action": "turn_on",
        "params": {"target_humidity": 50},
        "requires_confirmation": true
      }
    ]
  }
}
```

## 用户偏好学习

### 学习维度
- 常用设备和功能
- 时间段行为模式
- 环境偏好（温度、亮度等）
- 响应方式偏好（主动/被动）
- 确认阈值偏好

### 个性化配置

```json
{
  "user_id": "user_001",
  "preferences": {
    "proactive_level": "medium",
    "confirmation_threshold": 0.7,
    "quiet_hours": ["23:00", "07:00"],
    "auto_execute_categories": ["lighting", "media"],
    "always_confirm_categories": ["security", "appliance"]
  }
}
```
