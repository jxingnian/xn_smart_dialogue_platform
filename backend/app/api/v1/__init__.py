# -*- coding: utf-8 -*-
"""
API v1 版本模块

功能说明：
    API 第一版本的所有接口。
    版本号在 URL 中体现：/api/v1/xxx
    
    当 API 有重大变更时，创建新版本（v2），
    旧版本保持兼容一段时间后再废弃。
"""

from fastapi import APIRouter

from app.api.v1 import device, memory, chat

# 创建 v1 版本的总路由
router = APIRouter()

# 注册各模块的路由
router.include_router(device.router, prefix="/devices", tags=["设备管理"])
router.include_router(memory.router, prefix="/memories", tags=["记忆管理"])
router.include_router(chat.router, prefix="/chat", tags=["对话交互"])
