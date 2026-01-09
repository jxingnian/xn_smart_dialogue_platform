# -*- coding: utf-8 -*-
"""
设备 Schema 模块

功能说明：
    定义设备相关 API 的请求和响应数据结构。
"""

from typing import Optional, Any
from datetime import datetime
from pydantic import BaseModel, Field


class DeviceCreate(BaseModel):
    """
    创建设备请求
    
    添加新设备时需要提供的信息。
    """
    
    device_id: str = Field(
        ..., 
        min_length=1, 
        max_length=64, 
        description="设备唯一标识，用于MQTT通信"
    )
    name: str = Field(
        ..., 
        min_length=1, 
        max_length=100, 
        description="设备名称"
    )
    type: str = Field(
        ..., 
        min_length=1, 
        max_length=50, 
        description="设备类型，如 purifier、fish_feeder"
    )
    location: Optional[str] = Field(
        default=None, 
        max_length=100, 
        description="设备位置"
    )
    description: Optional[str] = Field(
        default=None, 
        description="设备描述"
    )
    config: Optional[dict] = Field(
        default=None, 
        description="设备配置"
    )


class DeviceUpdate(BaseModel):
    """
    更新设备请求
    
    修改设备信息时可以更新的字段，所有字段都是可选的。
    """
    
    name: Optional[str] = Field(
        default=None, 
        min_length=1, 
        max_length=100, 
        description="设备名称"
    )
    location: Optional[str] = Field(
        default=None, 
        max_length=100, 
        description="设备位置"
    )
    description: Optional[str] = Field(
        default=None, 
        description="设备描述"
    )
    config: Optional[dict] = Field(
        default=None, 
        description="设备配置"
    )


class DeviceResponse(BaseModel):
    """
    设备响应
    
    返回给前端的设备信息。
    """
    
    id: int = Field(description="数据库主键ID")
    device_id: str = Field(description="设备唯一标识")
    name: str = Field(description="设备名称")
    type: str = Field(description="设备类型")
    location: Optional[str] = Field(description="设备位置")
    is_online: bool = Field(description="是否在线")
    status: Optional[dict] = Field(description="设备状态")
    config: Optional[dict] = Field(description="设备配置")
    description: Optional[str] = Field(description="设备描述")
    created_at: datetime = Field(description="创建时间")
    updated_at: datetime = Field(description="更新时间")
    
    class Config:
        # 允许从 ORM 对象直接转换
        from_attributes = True


class DeviceListResponse(BaseModel):
    """
    设备列表响应
    
    分页查询设备列表的响应。
    """
    
    items: list[DeviceResponse] = Field(description="设备列表")
    total: int = Field(description="总数量")
    page: int = Field(description="当前页码")
    size: int = Field(description="每页数量")


class DeviceControlRequest(BaseModel):
    """
    设备控制请求
    
    发送控制指令到设备。
    """
    
    command: str = Field(
        ..., 
        description="控制命令，如 power_on、power_off、set_mode"
    )
    params: Optional[dict] = Field(
        default=None, 
        description="命令参数"
    )
