# -*- coding: utf-8 -*-
"""
配置管理 API

功能说明：
    提供系统配置的读取和保存接口。
    配置保存在 JSON 文件中，支持热更新。
    支持从 DashScope API 动态获取可用模型列表，并保存到本地。
"""

import os
import json
import httpx
from typing import Optional, List
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, Field

router = APIRouter()

# 配置文件路径
CONFIG_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(__file__)))), "config")
CONFIG_FILE = os.path.join(CONFIG_DIR, "ai_config.json")
MODELS_FILE = os.path.join(CONFIG_DIR, "cloud_models.json")  # 保存获取到的模型列表


class AIModelConfig(BaseModel):
    """AI 模型配置"""
    provider: str = Field(default="cloud", description="模型来源：local 或 cloud")
    cloud_model: str = Field(default="qwen-omni-turbo", description="云端模型名称")
    local_model: str = Field(default="", description="本地模型名称")
    api_key: str = Field(default="", description="API Key")
    local_endpoint: str = Field(default="http://localhost:11434", description="本地服务地址")


class AIModelConfigResponse(BaseModel):
    """AI 配置响应（API Key 脱敏）"""
    provider: str
    cloud_model: str
    local_model: str
    api_key: str
    local_endpoint: str


# 默认模型列表（未获取时使用）
DEFAULT_CLOUD_MODELS = [
    {"id": "qwen-omni-turbo", "name": "通义千问 Omni Turbo"},
    {"id": "qwen-turbo", "name": "通义千问 Turbo"},
    {"id": "qwen-plus", "name": "通义千问 Plus"},
    {"id": "qwen-max", "name": "通义千问 Max"},
]


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


def load_cloud_models() -> List[dict]:
    """加载已保存的云端模型列表"""
    if os.path.exists(MODELS_FILE):
        with open(MODELS_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    return DEFAULT_CLOUD_MODELS


def save_cloud_models(models: List[dict]):
    """保存云端模型列表到文件"""
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(MODELS_FILE, "w", encoding="utf-8") as f:
        json.dump(models, f, ensure_ascii=False, indent=2)


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
    """获取 AI 模型配置，API Key 脱敏显示"""
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
    """更新 AI 模型配置"""
    global _config
    
    # 如果 API Key 是脱敏的，保留原来的
    if "****" in config.api_key:
        old_config = get_config()
        config.api_key = old_config.api_key
    
    save_config(config)
    _config = config
    return {"message": "配置已保存"}


@router.get("/ai/models")
async def get_available_models():
    """获取可用的模型列表（优先返回已保存的列表）"""
    cloud_models = load_cloud_models()
    return {
        "cloud": cloud_models,
        "local": [],
        "doc_url": "https://help.aliyun.com/zh/model-studio/models"
    }


class FetchModelsRequest(BaseModel):
    """获取模型列表请求"""
    api_key: str = Field(..., description="DashScope API Key")


@router.post("/ai/fetch-models")
async def fetch_dashscope_models(request: FetchModelsRequest):
    """
    从 DashScope API 获取可用模型列表并保存
    
    使用 OpenAI 兼容接口获取所有可用模型。
    文档：https://help.aliyun.com/zh/dashscope/developer-reference/compatibility-of-openai-with-dashscope
    """
    api_key = request.api_key
    
    # 如果是脱敏的 key，使用已保存的
    if "****" in api_key:
        config = get_config()
        api_key = config.api_key
    
    if not api_key:
        raise HTTPException(status_code=400, detail="请先输入 API Key")
    
    try:
        async with httpx.AsyncClient(timeout=30.0) as client:
            response = await client.get(
                "https://dashscope.aliyuncs.com/compatible-mode/v1/models",
                headers={
                    "Authorization": f"Bearer {api_key}",
                    "Content-Type": "application/json",
                }
            )
            
            if response.status_code == 401:
                raise HTTPException(status_code=401, detail="API Key 无效")
            
            if response.status_code != 200:
                raise HTTPException(
                    status_code=response.status_code, 
                    detail=f"获取模型列表失败: {response.text}"
                )
            
            data = response.json()
            
            # 解析模型列表，只保留 qwen 系列
            models = []
            if "data" in data:
                for model in data["data"]:
                    model_id = model.get("id", "")
                    if "qwen" in model_id.lower():
                        models.append({
                            "id": model_id,
                            "name": model_id,
                        })
            
            # 按名称排序
            models.sort(key=lambda x: x["id"])
            
            # 保存到文件
            save_cloud_models(models)
            
            return {
                "models": models,
                "doc_url": "https://help.aliyun.com/zh/model-studio/models"
            }
            
    except httpx.TimeoutException:
        raise HTTPException(status_code=504, detail="请求超时，请稍后重试")
    except httpx.RequestError as e:
        raise HTTPException(status_code=500, detail=f"网络错误: {str(e)}")
