# XN OTA 组件

ESP32 OTA (Over-the-Air) 固件升级组件，提供固件版本管理、设备认证、远程升级等功能。

## 功能特性

- ✅ 获取本地和云端固件版本列表
- ✅ 检查固件更新
- ✅ 升级到指定版本
- ✅ 固件校验（MD5）
- ✅ 升级进度回调
- ✅ 设备认证和激活
- ✅ 自动标记固件有效
- ✅ 支持强制升级

## 目录结构

```
xn_ota/
├── CMakeLists.txt          # 组件构建配置
├── include/
│   └── xn_ota.h           # 组件头文件
├── src/
│   └── xn_ota.c           # 组件实现
└── README.md              # 本文件
```

## API 说明

### 初始化和反初始化

```c
// 初始化 OTA 组件
xn_ota_config_t config = XN_OTA_DEFAULT_CONFIG();
config.server_url = "http://your-server.com/api/v1/ota/check";
config.device_type = "purifier";
config.progress_cb = ota_progress_callback;

esp_err_t ret = xn_ota_init(&config);

// 反初始化
xn_ota_deinit();
```

### 版本管理

```c
// 获取本地版本
const char *local_version = xn_ota_get_local_version();

// 获取云端版本列表
xn_ota_version_list_t version_list;
xn_ota_get_cloud_versions(&version_list);

// 检查更新
bool has_update = false;
xn_ota_version_info_t latest_version;
xn_ota_check_update(&has_update, &latest_version);
```

### 固件升级

```c
// 升级到最新版本
xn_ota_upgrade(NULL);

// 升级到指定版本
xn_ota_upgrade("1.2.0");

// 标记当前固件为有效
xn_ota_mark_valid();
```

### 设备认证

```c
// 检查认证状态
xn_ota_auth_status_t status;
char activation_code[64];
char activation_message[256];
xn_ota_check_auth_status(&status, activation_code, activation_message);

// 获取设备信息
xn_ota_device_info_t device_info;
xn_ota_get_device_info(&device_info);

// 提交设备信息到云端
xn_ota_submit_device_info();

// 执行设备激活
xn_ota_activate_device();
```

## 使用示例

### 基本使用

```c
#include "xn_ota.h"

void ota_progress_callback(int progress, size_t speed)
{
    printf("OTA Progress: %d%%, Speed: %d B/s\n", progress, speed);
}

void app_main(void)
{
    // 初始化 OTA
    xn_ota_config_t config = XN_OTA_DEFAULT_CONFIG();
    config.server_url = "http://192.168.1.100:8000/api/v1/ota/check";
    config.device_type = "purifier";
    config.progress_cb = ota_progress_callback;
    
    xn_ota_init(&config);
    
    // 标记当前固件为有效
    xn_ota_mark_valid();
    
    // 检查更新
    bool has_update = false;
    xn_ota_version_info_t latest_version;
    
    if (xn_ota_check_update(&has_update, &latest_version) == ESP_OK) {
        if (has_update) {
            printf("New version available: %s\n", latest_version.version);
            
            // 执行升级
            if (xn_ota_upgrade(NULL) == ESP_OK) {
                printf("Upgrade successful, restarting...\n");
                esp_restart();
            }
        }
    }
}
```

### 与 OTA Manager 配合使用

```c
#include "managers/ota_manager.h"

void app_main(void)
{
    // 初始化 OTA Manager
    ota_manager_config_t config = OTA_MANAGER_DEFAULT_CONFIG();
    config.server_url = "http://192.168.1.100:8000/api/v1/ota/check";
    config.device_type = "purifier";
    config.auto_upgrade = false;  // 手动升级
    config.check_on_boot = true;  // 启动时检查
    
    ota_manager_init(&config);
    
    // 启动 OTA 流程（自动执行认证和更新检查）
    ota_manager_start();
    
    // 检查是否需要操作
    if (ota_manager_needs_action()) {
        // 获取认证状态
        xn_ota_auth_status_t status;
        char activation_code[64];
        ota_manager_get_auth_status(&status, activation_code, NULL);
        
        if (status == XN_OTA_AUTH_PENDING) {
            printf("Device needs activation, code: %s\n", activation_code);
        }
    }
}
```

## 服务端协议

### 版本检查接口

**请求：**

```
POST /api/v1/ota/check
Content-Type: application/json
```

**请求头：**

- `Device-Id`: 设备 ID
- `Content-Type`: application/json

**请求体：**

```json
{
  "device_id": "A1B2C3D4E5F6",
  "device_type": "purifier",
  "firmware_version": "1.0.0",
  "hardware_version": "v1.0",
  "chip_model": "ESP32"
}
```

**响应：**

```json
{
  "firmware": {
    "version": "1.1.0",
    "url": "https://ota.example.com/firmware/purifier_v1.1.0.bin",
    "size": 1048576,
    "md5": "5d41402abc4b2a76b9719d911017c592",
    "force": 0,
    "changelog": "修复已知问题"
  },
  "activation": {
    "code": "ABCD-1234",
    "message": "请在管理端输入激活码",
    "challenge": "random-challenge-string"
  }
}
```

### 设备激活接口

**请求：**

```
POST /api/v1/ota/check/activate
Content-Type: application/json
```

**请求体：**

```json
{
  "device_id": "A1B2C3D4E5F6",
  "challenge": "random-challenge-string"
}
```

**响应：**

- `200`: 激活成功
- `202`: 激活请求已接受，等待处理
- `401`: 激活失败

## 配置选项

| 参数 | 类型 | 说明 |
|------|------|------|
| server_url | const char* | OTA 服务器地址（必填） |
| device_type | const char* | 设备类型 |
| progress_cb | xn_ota_progress_cb_t | 升级进度回调 |
| timeout_ms | uint32_t | 超时时间（毫秒） |

## 注意事项

1. **分区表配置**：确保分区表中有足够的 OTA 分区空间
2. **网络连接**：OTA 操作前确保设备已连接网络
3. **固件校验**：建议启用 MD5 校验确保固件完整性
4. **升级后重启**：升级成功后需要调用 `esp_restart()` 重启设备
5. **标记有效**：首次启动后调用 `xn_ota_mark_valid()` 防止回滚

## 依赖组件

- `esp_http_client`: HTTP 客户端
- `esp_https_ota`: HTTPS OTA 支持
- `nvs_flash`: NVS 存储
- `json`: cJSON 库

## 许可证

Copyright (c) 2026 by xingnian, All Rights Reserved.
