# -*- coding: utf-8 -*-
"""
数据库实体模块

功能说明：
    定义所有数据库表对应的 ORM 实体类。
    使用 SQLAlchemy 2.0 的声明式映射方式。
    
    每个实体类对应数据库中的一张表，
    类的属性对应表的字段。
"""

from app.models.entities.base import Base
from app.models.entities.device import Device
from app.models.entities.memory import TextMemory, PreferenceMemory, ImageMemory

# 导出所有实体，方便其他模块导入
__all__ = [
    "Base",
    "Device",
    "TextMemory",
    "PreferenceMemory",
    "ImageMemory",
]
