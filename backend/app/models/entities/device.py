# -*- coding: utf-8 -*-
"""
设备实体模块

功能说明：
    定义设备相关的数据库表结构。
    设备是智能家居系统的核心，包括净化器、喂鱼器等。
"""

from typing import Optional
from sqlalchemy import String, Boolean, Text, JSON
from sqlalchemy.orm import Mapped, mapped_column

from app.models.entities.base import Base


class Device(Base):
    """
    设备表
    
    存储所有接入系统的智能设备信息。
    每个设备有唯一的 device_id，用于 MQTT 通信标识。
    
    字段说明：
        device_id: 设备唯一标识，用于 MQTT 主题和设备识别
        name: 设备名称，用户自定义的友好名称
        type: 设备类型，如 purifier（净化器）、fish_feeder（喂鱼器）
        location: 设备位置，如"客厅"、"卧室"
        is_online: 是否在线，通过 MQTT 心跳更新
        status: 设备状态，JSON 格式存储设备特有的状态信息
        config: 设备配置，JSON 格式存储设备特有的配置项
    """
    
    __tablename__ = "devices"
    __table_args__ = {"comment": "设备表"}
    
    # 设备唯一标识，全局唯一，用于 MQTT 通信
    device_id: Mapped[str] = mapped_column(
        String(64), 
        unique=True, 
        index=True, 
        comment="设备唯一标识"
    )
    
    # 设备名称，用户可自定义
    name: Mapped[str] = mapped_column(
        String(100), 
        comment="设备名称"
    )
    
    # 设备类型：purifier（净化器）、fish_feeder（喂鱼器）等
    type: Mapped[str] = mapped_column(
        String(50), 
        index=True, 
        comment="设备类型"
    )
    
    # 设备位置，可选
    location: Mapped[Optional[str]] = mapped_column(
        String(100), 
        nullable=True, 
        comment="设备位置"
    )
    
    # 是否在线，默认离线
    is_online: Mapped[bool] = mapped_column(
        Boolean, 
        default=False, 
        comment="是否在线"
    )
    
    # 设备状态，JSON 格式，不同设备有不同的状态字段
    # 例如净化器：{"power": true, "mode": "auto", "pm25": 35}
    # 例如喂鱼器：{"last_feed_time": "2026-01-09 08:00:00", "food_level": 80}
    status: Mapped[Optional[dict]] = mapped_column(
        JSON, 
        nullable=True, 
        comment="设备状态(JSON)"
    )
    
    # 设备配置，JSON 格式
    # 例如喂鱼器的定时投喂计划：{"schedules": [{"time": "08:00", "amount": 10}]}
    config: Mapped[Optional[dict]] = mapped_column(
        JSON, 
        nullable=True, 
        comment="设备配置(JSON)"
    )
    
    # 设备描述，可选
    description: Mapped[Optional[str]] = mapped_column(
        Text, 
        nullable=True, 
        comment="设备描述"
    )
