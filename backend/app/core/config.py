# -*- coding: utf-8 -*-
"""
配置管理模块

功能说明：
    统一管理所有配置项，包括数据库连接、Redis、MQTT、AI服务等。
    支持从环境变量和配置文件读取，环境变量优先级更高。

使用方式：
    from app.core.config import settings
    print(settings.DATABASE_URL)
"""

from typing import Optional
from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    """
    应用配置类
    
    所有配置项都可以通过环境变量覆盖，环境变量名与字段名相同（大写）。
    例如：DATABASE_URL 可以通过设置环境变量 DATABASE_URL 来覆盖。
    """
    
    # ========== 应用基础配置 ==========
    # 应用名称，用于日志和监控标识
    APP_NAME: str = "知境中枢"
    # 运行环境：development（开发）、production（生产）、testing（测试）
    ENV: str = "development"
    # 是否开启调试模式，生产环境务必关闭
    DEBUG: bool = True
    # API 版本前缀
    API_V1_PREFIX: str = "/api/v1"
    
    # ========== 数据库配置 ==========
    # PostgreSQL 连接地址，格式：postgresql+asyncpg://用户名:密码@主机:端口/数据库名
    DATABASE_URL: str = "postgresql+asyncpg://postgres:postgres@localhost:5432/zhijing"
    
    # ========== Redis 配置 ==========
    # Redis 连接地址，用于缓存和会话管理
    REDIS_URL: str = "redis://localhost:6379/0"
    
    # ========== MQTT 配置 ==========
    # MQTT Broker 地址，用于设备通信
    MQTT_BROKER: str = "localhost"
    # MQTT Broker 端口
    MQTT_PORT: int = 1883
    # MQTT 用户名，留空表示匿名连接
    MQTT_USERNAME: Optional[str] = None
    # MQTT 密码
    MQTT_PASSWORD: Optional[str] = None
    
    # ========== AI 服务配置 ==========
    # 通义千问 API Key，用于调用阿里云 AI 服务
    DASHSCOPE_API_KEY: Optional[str] = None
    # Ollama 服务地址，用于本地模型推理
    OLLAMA_BASE_URL: str = "http://localhost:11434"
    
    # ========== 安全配置 ==========
    # JWT 密钥，用于生成和验证 Token，生产环境必须修改
    SECRET_KEY: str = "your-secret-key-change-in-production"
    # Token 过期时间（分钟）
    ACCESS_TOKEN_EXPIRE_MINUTES: int = 60 * 24 * 7  # 7 天
    
    class Config:
        # 环境变量文件路径
        env_file = ".env"
        # 环境变量文件编码
        env_file_encoding = "utf-8"


# 全局配置实例，其他模块直接导入使用
settings = Settings()
