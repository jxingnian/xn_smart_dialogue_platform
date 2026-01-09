# -*- coding: utf-8 -*-
"""
对话相关的数据模型

功能说明：
    定义对话 API 的请求和响应数据结构。
"""

from typing import Optional, List
from pydantic import BaseModel, Field


class ChatMessage(BaseModel):
    """
    单条对话消息
    
    role: 角色，user（用户）或 assistant（AI）
    content: 消息内容
    """
    role: str = Field(..., description="角色：user 或 assistant")
    content: str = Field(..., description="消息内容")


class ChatRequest(BaseModel):
    """
    对话请求
    
    message: 用户发送的消息
    history: 对话历史，用于多轮对话
    """
    message: str = Field(..., description="用户消息")
    history: Optional[List[ChatMessage]] = Field(default=[], description="对话历史")


class ChatResponse(BaseModel):
    """
    对话响应
    
    reply: AI 的回复内容
    """
    reply: str = Field(..., description="AI 回复")
