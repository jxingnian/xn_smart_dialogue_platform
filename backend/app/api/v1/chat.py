# -*- coding: utf-8 -*-
"""
对话交互 API

功能说明：
    提供对话相关的 HTTP 接口。
    根据系统配置自动选择模型。
"""

from typing import Optional
from fastapi import APIRouter

from app.models.schemas.chat import ChatRequest, ChatResponse
from app.services.ai.dashscope_provider import DashScopeProvider
from app.api.v1.config import get_config

router = APIRouter()


def get_llm_provider():
    """
    根据配置获取 LLM Provider
    """
    config = get_config()
    
    if config.provider == "cloud":
        return DashScopeProvider(
            api_key=config.api_key,
            model=config.cloud_model,
        )
    else:
        # 本地模型，后续实现
        return None


@router.post("/text", response_model=ChatResponse)
async def chat_text(request: ChatRequest):
    """
    文本对话
    
    发送文本消息，获取 AI 回复。
    """
    provider = get_llm_provider()
    
    if provider is None:
        return ChatResponse(reply="错误：未配置 AI 模型，请在系统设置中配置")
    
    # 构建消息列表
    messages = []
    
    # 添加系统提示
    messages.append({
        "role": "system",
        "content": "你是知境中枢的 AI 助手，负责帮助用户管理智能家居设备、查询记忆、回答问题。请用简洁友好的方式回复。"
    })
    
    # 添加对话历史
    if request.history:
        for msg in request.history:
            messages.append({
                "role": msg.role,
                "content": msg.content
            })
    
    # 添加当前用户消息
    messages.append({
        "role": "user",
        "content": request.message
    })
    
    # 调用 LLM 获取回复
    reply = await provider.chat(messages)
    
    return ChatResponse(reply=reply)


@router.get("/model-info")
async def get_model_info():
    """
    获取当前使用的模型信息
    """
    config = get_config()
    
    # 判断是否是多模态模型
    multimodal_models = ["qwen-omni-turbo", "qwen-vl-plus", "qwen-vl-max"]
    is_multimodal = config.cloud_model in multimodal_models if config.provider == "cloud" else False
    
    return {
        "provider": config.provider,
        "model": config.cloud_model if config.provider == "cloud" else config.local_model,
        "multimodal": is_multimodal,
        "configured": bool(config.api_key) if config.provider == "cloud" else bool(config.local_endpoint),
    }
