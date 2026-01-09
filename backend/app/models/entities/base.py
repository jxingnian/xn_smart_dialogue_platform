# -*- coding: utf-8 -*-
"""
数据库实体基类

功能说明：
    定义所有数据库实体的公共基类。
    包含通用字段（如 id、创建时间、更新时间）和通用方法。
"""

from datetime import datetime
from sqlalchemy import DateTime, func
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column


class Base(DeclarativeBase):
    """
    所有数据库实体的基类
    
    提供通用字段：
        - id: 主键，自增整数
        - created_at: 创建时间，自动填充
        - updated_at: 更新时间，自动更新
    """
    
    # 主键 ID，所有表都有这个字段
    id: Mapped[int] = mapped_column(primary_key=True, autoincrement=True, comment="主键ID")
    
    # 创建时间，插入记录时自动填充当前时间
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True),
        server_default=func.now(),
        comment="创建时间"
    )
    
    # 更新时间，每次更新记录时自动更新为当前时间
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True),
        server_default=func.now(),
        onupdate=func.now(),
        comment="更新时间"
    )
