# -*- coding: utf-8 -*-
"""
Pydantic 模型模块

功能说明：
    定义 API 请求和响应的数据结构。
    Pydantic 模型用于：
        - 请求参数校验：自动验证客户端传入的数据格式
        - 响应数据序列化：将数据库对象转换为 JSON 响应
        - API 文档生成：自动生成 OpenAPI 文档
"""

from app.models.schemas.device import (
    DeviceCreate,
    DeviceUpdate,
    DeviceResponse,
    DeviceListResponse,
)
from app.models.schemas.common import (
    ResponseBase,
    PageParams,
    PageResponse,
)

__all__ = [
    "DeviceCreate",
    "DeviceUpdate",
    "DeviceResponse",
    "DeviceListResponse",
    "ResponseBase",
    "PageParams",
    "PageResponse",
]
