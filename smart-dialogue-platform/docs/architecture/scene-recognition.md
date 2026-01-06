# 场景识别模块

## 模块概述

场景识别模块负责分析当前对话场景，判断用户是否在与设备交互，以及识别具体的交互场景类型。这是免唤醒对话的核心能力。

## 场景分类体系

### 一级场景分类

```
对话场景
├── 人机对话 (Human-Device)
│   ├── 直接指令
│   ├── 询问查询
│   └── 闲聊互动
├── 人人对话 (Human-Human)
│   ├── 家人对话
│   ├── 访客对话
│   └── 电话/视频通话
├── 人宠互动 (Human-Pet)
│   └── 与宠物说话
├── 自言自语 (Self-Talk)
│   └── 独自思考/念叨
└── 背景声音 (Background)
    ├── 电视/广播
    ├── 音乐
    └── 环境噪音
```

### 二级场景细分

#### 人机对话场景
| 场景 | 描述 | 示例 |
|------|------|------|
| 设备控制 | 控制智能设备 | "把灯关了" |
| 信息查询 | 查询信息 | "今天天气怎么样" |
| 内容播放 | 播放媒体内容 | "放首歌" |
| 提醒设置 | 设置提醒/闹钟 | "明天早上7点叫我" |
| 闲聊陪伴 | 日常聊天 | "你觉得这个电影怎么样" |

#### 人人对话场景
| 场景 | 描述 | 特征 |
|------|------|------|
| 日常闲聊 | 家人日常交流 | 多人声音交替 |
| 讨论决策 | 讨论某事 | 可能涉及设备相关话题 |
| 情感交流 | 情感沟通 | 情绪特征明显 |
| 远程通话 | 电话/视频 | 单侧声音+电子音 |

#### 特殊场景
| 场景 | 描述 | 处理策略 |
|------|------|----------|
| 隐私对话 | 敏感私密内容 | 不处理/不记录 |
| 紧急情况 | 求救/异常 | 主动响应 |
| 睡眠场景 | 用户休息中 | 静默模式 |

## 场景判断逻辑

```
输入数据
    │
    ▼
┌─────────────────────────────────────────┐
│            多维度特征分析                │
├─────────────────────────────────────────┤
│ • 说话人数量与身份                       │
│ • 语音方向（朝向设备/其他方向）          │
│ • 对话内容关键词                         │
│ • 上下文历史                            │
│ • 设备状态                              │
│ • 时间/环境因素                         │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│            场景分类器                    │
├─────────────────────────────────────────┤
│  规则引擎 + ML模型 混合决策              │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│            置信度评估                    │
├─────────────────────────────────────────┤
│  高置信度 → 直接判定                     │
│  低置信度 → 保守处理/询问确认            │
└────────────────┬────────────────────────┘
                 │
                 ▼
            场景输出
```

## 判断维度

### 1. 语音特征维度
- 说话人数量
- 说话人身份（已注册/未知）
- 语音方向性
- 音量大小
- 语速变化

### 2. 内容特征维度
- 设备相关关键词
- 指令性语句结构
- 疑问句式
- 称呼词（"小爱"、"设备"等）

### 3. 上下文维度
- 前序对话内容
- 设备当前状态
- 用户历史行为模式
- 时间段特征

### 4. 环境维度
- 背景音类型
- 房间位置
- 其他设备状态

## 场景判断规则示例

```yaml
rules:
  # 高置信度人机对话
  - name: "direct_device_command"
    conditions:
      - speaker_count: 1
      - speaker_is_registered: true
      - contains_device_keyword: true
      - sentence_type: imperative
    result:
      scene: "human_device"
      sub_scene: "device_control"
      confidence: 0.95

  # 人人对话
  - name: "human_conversation"
    conditions:
      - speaker_count: ">1"
      - no_device_keyword: true
      - conversation_pattern: "dialogue"
    result:
      scene: "human_human"
      confidence: 0.85

  # 潜在需求识别（关键场景）
  - name: "implicit_need"
    conditions:
      - speaker_is_registered: true
      - contains_environment_complaint: true
      - related_device_available: true
    result:
      scene: "human_device"
      sub_scene: "implicit_request"
      confidence: 0.70
      action: "proactive_suggestion"
```

## 输出数据结构

```json
{
  "scene_id": "uuid",
  "timestamp": "2024-01-01T12:00:00Z",
  "primary_scene": {
    "type": "human_device",
    "sub_type": "implicit_request",
    "confidence": 0.75
  },
  "alternative_scenes": [
    {
      "type": "human_human",
      "confidence": 0.20
    }
  ],
  "context": {
    "speaker_count": 1,
    "speaker_ids": ["user_001"],
    "location": "living_room",
    "time_context": "evening",
    "device_states": {
      "humidifier": "off",
      "ac": "on"
    }
  },
  "flags": {
    "is_device_related": true,
    "requires_response": true,
    "is_private": false,
    "is_urgent": false
  },
  "recommended_action": "proactive_suggestion"
}
```

## 隐私保护

### 不处理场景
- 检测到隐私关键词
- 用户明确设置的隐私时段
- 低置信度且无设备相关性

### 数据处理原则
- 非人机对话不存储原始音频
- 敏感内容实时脱敏
- 用户可查看和删除历史记录
