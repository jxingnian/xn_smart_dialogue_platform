# 音频处理模块

## 模块概述

音频处理模块负责对输入音频进行全方位分析，提取对话所需的各类特征信息。

## 处理流程

```
原始音频
    │
    ▼
┌─────────────────┐
│  音频预处理      │ ← 降噪、增强、分帧
└────────┬────────┘
         │
    ┌────┴────┐
    ▼         ▼
┌───────┐ ┌───────┐
│ VAD   │ │ 特征  │
│ 检测  │ │ 提取  │
└───┬───┘ └───┬───┘
    │         │
    ▼         ▼
┌─────────────────┐
│   并行处理管道   │
├─────────────────┤
│ • 语音识别 ASR  │
│ • 说话人识别    │
│ • 情绪识别      │
│ • 声纹特征      │
│ • 环境音分析    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  结果融合输出    │
└─────────────────┘
```

## 核心功能

### 1. 语音活动检测 (VAD)
- 检测音频中的语音段落
- 区分语音与背景噪音
- 支持连续对话检测

### 2. 语音识别 (ASR)
- 实时语音转文字
- 支持多语言/方言
- 流式识别与端点检测
- 支持热词定制

### 3. 说话人识别 (Speaker Recognition)
- 说话人分离 (Speaker Diarization)
- 说话人身份识别
- 声纹注册与验证
- 多人对话场景支持

### 4. 情绪识别 (Emotion Recognition)
- 语音情绪分析
- 支持情绪类型：
  - 平静 (Neutral)
  - 高兴 (Happy)
  - 悲伤 (Sad)
  - 愤怒 (Angry)
  - 惊讶 (Surprised)
  - 焦虑 (Anxious)
- 情绪强度评估

### 5. 环境音分析 (Ambient Sound Analysis)
- 背景音识别（电视、音乐、宠物叫声等）
- 环境噪音水平评估
- 异常声音检测（玻璃破碎、烟雾报警等）

### 6. 语音特征提取
- 音量/响度分析
- 语速检测
- 音调变化
- 停顿分析

## 输出数据结构

```json
{
  "audio_id": "uuid",
  "timestamp": "2024-01-01T12:00:00Z",
  "duration_ms": 3500,
  "vad": {
    "speech_segments": [
      {"start_ms": 100, "end_ms": 2800}
    ],
    "is_speech": true
  },
  "asr": {
    "text": "今天天气好干燥啊",
    "confidence": 0.95,
    "words": [
      {"word": "今天", "start_ms": 100, "end_ms": 400},
      {"word": "天气", "start_ms": 420, "end_ms": 700}
    ],
    "language": "zh-CN"
  },
  "speaker": {
    "speaker_id": "user_001",
    "speaker_name": "张三",
    "confidence": 0.92,
    "is_registered": true
  },
  "emotion": {
    "primary": "neutral",
    "confidence": 0.85,
    "valence": 0.3,
    "arousal": 0.4,
    "all_emotions": {
      "neutral": 0.85,
      "happy": 0.10,
      "sad": 0.05
    }
  },
  "ambient": {
    "noise_level_db": 35,
    "detected_sounds": ["tv_background"],
    "environment_type": "living_room"
  },
  "prosody": {
    "volume": 0.6,
    "speech_rate": 4.2,
    "pitch_mean": 180,
    "pitch_variance": 25
  }
}
```

## 技术选型建议

| 功能 | 边缘端方案 | 云端方案 |
|------|-----------|----------|
| VAD | WebRTC VAD / Silero VAD | - |
| ASR | Whisper.cpp / Vosk | Whisper API / 云厂商ASR |
| 说话人识别 | Resemblyzer | Speaker Embedding API |
| 情绪识别 | SER模型 | 情绪分析API |
| 环境音 | YAMNet | AudioSet模型 |

## 性能指标

- 端到端延迟: < 300ms (边缘) / < 500ms (云端)
- ASR准确率: > 95%
- 说话人识别准确率: > 90%
- 情绪识别准确率: > 80%
