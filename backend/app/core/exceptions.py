# -*- coding: utf-8 -*-
"""
自定义异常模块

功能说明：
    定义系统中所有的业务异常类型。
    业务异常是可预期的错误，比如设备不存在、参数错误等。
    这些异常会被统一捕获并转换为友好的错误响应返回给用户。

异常分类：
    - BusinessError: 业务异常基类，所有业务异常都继承自它
    - DeviceError: 设备相关异常
    - MemoryError: 记忆相关异常
    - AIProviderError: AI 服务相关异常
"""


class BusinessError(Exception):
    """
    业务异常基类
    
    所有可预期的业务错误都应该继承这个类。
    包含错误码和错误信息，便于前端识别和处理。
    
    属性：
        code: 错误码，用于前端判断具体错误类型
        message: 错误信息，用于展示给用户
    """
    
    def __init__(self, code: str, message: str):
        self.code = code
        self.message = message
        super().__init__(message)


# ========== 设备相关异常 ==========

class DeviceNotFoundError(BusinessError):
    """设备不存在异常，当查询的设备 ID 在数据库中找不到时抛出"""
    
    def __init__(self, device_id: str):
        super().__init__("DEVICE_NOT_FOUND", f"设备不存在: {device_id}")


class DeviceOfflineError(BusinessError):
    """设备离线异常，当尝试控制一个离线设备时抛出"""
    
    def __init__(self, device_id: str):
        super().__init__("DEVICE_OFFLINE", f"设备离线，无法执行操作: {device_id}")


class DeviceControlError(BusinessError):
    """设备控制失败异常，当发送控制指令失败时抛出"""
    
    def __init__(self, device_id: str, reason: str):
        super().__init__("DEVICE_CONTROL_FAILED", f"设备控制失败: {device_id}, 原因: {reason}")


# ========== 记忆相关异常 ==========

class MemoryNotFoundError(BusinessError):
    """记忆不存在异常，当查询的记忆记录找不到时抛出"""
    
    def __init__(self, memory_id: str):
        super().__init__("MEMORY_NOT_FOUND", f"记忆记录不存在: {memory_id}")


class MemoryStorageError(BusinessError):
    """记忆存储失败异常，当保存记忆到数据库失败时抛出"""
    
    def __init__(self, reason: str):
        super().__init__("MEMORY_STORAGE_FAILED", f"记忆存储失败: {reason}")


# ========== AI 服务相关异常 ==========

class AIProviderError(BusinessError):
    """AI 服务异常基类"""
    
    def __init__(self, provider: str, message: str):
        super().__init__("AI_PROVIDER_ERROR", f"AI 服务异常 [{provider}]: {message}")


class AIProviderUnavailableError(AIProviderError):
    """AI 服务不可用异常，当 AI 服务无法连接或响应超时时抛出"""
    
    def __init__(self, provider: str):
        super().__init__(provider, "服务不可用，请稍后重试")


class AIProviderQuotaExceededError(AIProviderError):
    """AI 服务配额超限异常，当 API 调用次数或 Token 用量超限时抛出"""
    
    def __init__(self, provider: str):
        super().__init__(provider, "调用配额已用尽")
