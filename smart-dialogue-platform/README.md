<!--
 * @Author: 星年 && j_xingnian@163.com
 * @Date: 2026-01-06 10:03:43
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2026-01-06 10:03:55
 * @FilePath: \xn_ai_dialogue\smart-dialogue-platform\README.md
 * @Description: 
 * 
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
-->
# 智能对话平台 (Smart Dialogue Platform)

免唤醒智能对话处理平台，支持端侧和云端部署，实现自然语音交互与智能设备控制。

## 核心特性

- **免唤醒对话**：无需唤醒词，自然对话触发
- **场景感知**：智能识别对话场景（人机对话/人人对话/人宠互动等）
- **意图理解**：结合设备能力，智能判断用户需求
- **多端部署**：支持边缘端和云端灵活部署
- **设备生态**：开放接入协议，支持各类智能设备

## 项目结构

```
smart-dialogue-platform/
├── docs/                    # 文档
│   ├── architecture/        # 架构设计
│   ├── api/                 # API文档
│   └── device-integration/  # 设备接入指南
├── core/                    # 核心服务
│   ├── audio-processor/     # 音频处理模块
│   ├── scene-analyzer/      # 场景分析模块
│   ├── intent-engine/       # 意图识别引擎
│   └── device-manager/      # 设备管理模块
├── gateway/                 # 接入网关
├── edge/                    # 边缘端部署
├── cloud/                   # 云端部署
└── sdk/                     # 设备接入SDK
```

## 快速开始

详见 [快速开始指南](docs/getting-started.md)

## 文档索引

- [系统架构](docs/architecture/system-overview.md)
- [音频处理](docs/architecture/audio-processing.md)
- [场景识别](docs/architecture/scene-recognition.md)
- [意图引擎](docs/architecture/intent-engine.md)
- [设备接入](docs/device-integration/README.md)
- [API参考](docs/api/README.md)
