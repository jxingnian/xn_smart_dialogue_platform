# -*- coding: utf-8 -*-
"""
通用 Schema 模块

功能说明：
    定义 API 通用的请求和响应结构。
    包括统一响应格式、分页参数、分页响应等。
"""

from typing import TypeVar, Generic, Optional, Any
from pydantic import BaseModel, Field

# 泛型类型变量，用于定义通用的响应数据类型
T = TypeVar("T")


class ResponseBase(BaseModel, Generic[T]):
    """
    统一响应格式
    
    所有 API 响应都使用这个格式，便于前端统一处理。
    
    字段说明：
        code: 响应码，0 表示成功，其他表示错误码
        message: 响应消息，成功时为 "success"，失败时为错误描述
        data: 响应数据，具体类型由泛型参数决定
    """
    
    code: int = Field(default=0, description="响应码，0表示成功")
    message: str = Field(default="success", description="响应消息")
    data: Optional[T] = Field(default=None, description="响应数据")


class PageParams(BaseModel):
    """
    分页参数
    
    用于列表查询接口的分页参数。
    
    字段说明：
        page: 页码，从 1 开始
        size: 每页数量，默认 20，最大 100
    """
    
    page: int = Field(default=1, ge=1, description="页码，从1开始")
    size: int = Field(default=20, ge=1, le=100, description="每页数量")


class PageResponse(BaseModel, Generic[T]):
    """
    分页响应
    
    用于列表查询接口的响应数据。
    
    字段说明：
        items: 数据列表
        total: 总数量
        page: 当前页码
        size: 每页数量
        pages: 总页数
    """
    
    items: list[T] = Field(description="数据列表")
    total: int = Field(description="总数量")
    page: int = Field(description="当前页码")
    size: int = Field(description="每页数量")
    pages: int = Field(description="总页数")
