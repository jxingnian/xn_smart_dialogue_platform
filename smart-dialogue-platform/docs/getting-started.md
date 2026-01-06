# 快速开始指南

## 平台部署

### 方式一：云端部署

```bash
# 使用 Docker Compose 部署
git clone https://github.com/your-org/smart-dialogue-platform.git
cd smart-dialogue-platform
docker-compose up -d
```

### 方式二：边缘端部署

```bash
# 在边缘设备上部署轻量版
./scripts/deploy-edge.sh --config edge-config.yaml
```

### 方式三：混合部署

```bash
# 边缘端 + 云端协同
./scripts/deploy-hybrid.sh
```

## 接入第一个设备

### 1. 获取接入凭证

```bash
curl -X POST https://api.platform.com/v1/devices/register \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "device_type": "smart_speaker",
    "device_name": "客厅音箱"
  }'
```

### 2. 配置设备 SDK

```python
from smart_dialogue_sdk import Device

device = Device(
    device_id="your_device_id",
    device_token="your_device_token"
)
device.connect("mqtt://platform.example.com")
```

### 3. 开始上报音频

```python
# 音频采集并上报
device.start_audio_stream(
    sample_rate=16000,
    channels=1
)
```

## 测试对话

```bash
# 发送测试音频
curl -X POST https://api.platform.com/v1/audio/process \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -F "audio=@test.wav" \
  -F "device_id=your_device_id"
```

## 下一步

- [系统架构](architecture/system-overview.md)
- [设备接入详解](device-integration/README.md)
- [API 文档](api/README.md)
