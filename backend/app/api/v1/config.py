# -*- coding: utf-8 -*-
"""
配置管理 API

功能说明：
    提供系统配置的读取和保存接口。
    配置保存在 JSON 文件中，支持热更新。
"""

import os
import json
from typing import Optional
from fastapi import APIRouter
from pydantic import BaseModel, Field

router = APIRouter()

# 配置文件路径
CONFIG_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(__file__)))), "config")
CONFIG_FILE = os.path.join(CONFIG_DIR, "ai_config.json")


class AIModelConfig(BaseModel):
    """AI 模型配置"""
    # 模型来源：local（本地）或 cloud（云端）
    provider: str = Field(default="cloud", description="模型来源：local 或 cloud")
    # 云端模型名称
    cloud_model: str = Field(default="qwen-omni-turbo", description="云端模型名称")
    # 本地模型名称
    local_model: str = Field(default="", description="本地模型名称")
    # API Key（云端需要）
    api_key: str = Field(default="", description="API Key")
    # 本地服务地址
    local_endpoint: str = Field(default="http://localhost:11434", description="本地服务地址")


class AIModelConfigResponse(BaseModel):
    """AI 配置响应（API Key 脱敏）"""
    provider: str
    cloud_model: str
    local_model: str
    api_key: str  # 脱敏后的
    local_endpoint: str


def mask_api_key(key: Optional[str]) -> str:
    """脱敏 API Key"""
    if not key or len(key) < 12:
        return ""
    return f"{key[:4]}****{key[-4:]}"


def load_config() -> AIModelConfig:
    """加载配置"""
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r", encoding="utf-8") as f:
            data = json.load(f)
            return AIModelConfig(**data)
    return AIModelConfig()


def save_config(config: AIModelConfig):
    """保存配置"""
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(CONFIG_FILE, "w", encoding="utf-8") as f:
        json.dump(config.model_dump(), f, ensure_ascii=False, indent=2)


# 全局配置实例
_config: Optional[AIModelConfig] = None


def get_config() -> AIModelConfig:
    """获取当前配置"""
    global _config
    if _config is None:
        _config = load_config()
    return _config


@router.get("/ai", response_model=AIModelConfigResponse)
async def get_ai_config():
    """
    获取 AI 模型配置
    
    返回当前的 AI 模型配置，API Key 会脱敏显示。
    """
    config = get_config()
    return AIModelConfigResponse(
        provider=config.provider,
        cloud_model=config.cloud_model,
        local_model=config.local_model,
        api_key=mask_api_key(config.api_key),
        local_endpoint=config.local_endpoint,
    )


@router.put("/ai")
async def update_ai_config(config: AIModelConfig):
    """
    更新 AI 模型配置
    
    保存配置到文件，并更新运行时配置。
    """
    global _config
    
    # 如果 API Key 是脱敏的（包含 ****），保留原来的
    if "****" in config.api_key:
        old_config = get_config()
        config.api_key = old_config.api_key
    
    # 保存并更新
    save_config(config)
    _config = config
    
    return {"message": "配置已保存"}


@router.get("/ai/models")
async def get_available_models():
    """
    获取可用的模型列表
    """
    return {
        "cloud": [
            {
                "id": "qwen-omni-turbo",
                "name": "通义千问 Omni Turbo",
                "description": "多模态模型，支持文本、图像、音频",
                "multimodal": True,
            },
            {
                "id": "qwen-turbo",
                "name": "通义千问 Turbo",
                "description": "文本模型，速度快",
                "multimodal": False,
            },
            {
                "id": "qwen-plus",
                "name": "通义千问 Plus",
                "description": "文本模型，效果均衡",
                "multimodal": False,
            },
            {
                "id": "qwen-max",
                "name": "通义千问 Max",
                "description": "文本模型，效果最好",
                "multimodal": False,
            },
        ],
        "local": [
            # 本地模型后续添加
        ]
    }
