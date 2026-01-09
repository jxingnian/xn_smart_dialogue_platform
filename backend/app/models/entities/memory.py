# -*- coding: utf-8 -*-
"""
记忆实体模块

功能说明：
    定义记忆层相关的数据库表结构。
    记忆分为三类：
        - 文本记忆：对话历史、感知数据等
        - 偏好习惯记忆：用户操作习惯、设备使用偏好
        - 图像记忆：人脸特征、感知视频帧
    
    所有记忆都支持向量检索，通过 pgvector 扩展实现语义搜索。
"""

from typing import Optional
from datetime import datetime
from sqlalchemy import String, Text, DateTime, Integer, Float, JSON
from sqlalchemy.orm import Mapped, mapped_column
from pgvector.sqlalchemy import Vector

from app.models.entities.base import Base


class TextMemory(Base):
    """
    文本记忆表
    
    存储对话历史、感知数据、事件日志等文本类型的记忆。
    支持通过向量进行语义检索，比如"找到上次讨论空调的对话"。
    
    字段说明：
        content: 记忆内容，原始文本
        type: 记忆类型，如 conversation（对话）、sensor（感知数据）、event（事件）
        source: 来源，如 user（用户输入）、device（设备上报）、system（系统生成）
        embedding: 文本向量，用于语义检索
        metadata: 元数据，JSON 格式存储额外信息
    """
    
    __tablename__ = "text_memories"
    __table_args__ = {"comment": "文本记忆表"}
    
    # 记忆内容，原始文本
    content: Mapped[str] = mapped_column(
        Text, 
        comment="记忆内容"
    )
    
    # 记忆类型：conversation（对话）、sensor（感知数据）、event（事件日志）
    type: Mapped[str] = mapped_column(
        String(50), 
        index=True, 
        comment="记忆类型"
    )
    
    # 来源：user（用户）、device（设备）、system（系统）
    source: Mapped[str] = mapped_column(
        String(50), 
        index=True, 
        comment="记忆来源"
    )
    
    # 文本向量，维度 1024（根据使用的 Embedding 模型调整）
    # 用于语义相似度检索
    embedding: Mapped[Optional[list]] = mapped_column(
        Vector(1024), 
        nullable=True, 
        comment="文本向量"
    )
    
    # 元数据，存储额外信息
    # 例如对话记忆：{"session_id": "xxx", "turn": 3}
    # 例如感知数据：{"device_id": "xxx", "sensor_type": "temperature"}
    metadata: Mapped[Optional[dict]] = mapped_column(
        JSON, 
        nullable=True, 
        comment="元数据(JSON)"
    )


class PreferenceMemory(Base):
    """
    偏好习惯记忆表
    
    存储用户的操作习惯和设备使用偏好。
    系统会分析这些数据，自动学习用户习惯，提供智能建议。
    
    字段说明：
        action_type: 动作类型，如 device_control（设备控制）、schedule（定时任务）
        action_detail: 动作详情，JSON 格式
        device_id: 关联的设备 ID，可选
        frequency: 发生频率，用于判断是否形成习惯
        last_occur_at: 最后发生时间
        time_pattern: 时间规律，如 "weekday_morning"（工作日早晨）
    """
    
    __tablename__ = "preference_memories"
    __table_args__ = {"comment": "偏好习惯记忆表"}
    
    # 动作类型：device_control（设备控制）、schedule（定时）、scene（场景）
    action_type: Mapped[str] = mapped_column(
        String(50), 
        index=True, 
        comment="动作类型"
    )
    
    # 动作详情，JSON 格式
    # 例如：{"device_id": "purifier_001", "command": "set_mode", "params": {"mode": "sleep"}}
    action_detail: Mapped[dict] = mapped_column(
        JSON, 
        comment="动作详情(JSON)"
    )
    
    # 关联的设备 ID，可选
    device_id: Mapped[Optional[str]] = mapped_column(
        String(64), 
        nullable=True, 
        index=True, 
        comment="关联设备ID"
    )
    
    # 发生频率（次数），用于判断习惯强度
    frequency: Mapped[int] = mapped_column(
        Integer, 
        default=1, 
        comment="发生频率"
    )
    
    # 最后发生时间
    last_occur_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), 
        comment="最后发生时间"
    )
    
    # 时间规律，描述这个习惯通常发生的时间段
    # 例如：weekday_morning（工作日早晨）、weekend_evening（周末晚上）
    time_pattern: Mapped[Optional[str]] = mapped_column(
        String(50), 
        nullable=True, 
        comment="时间规律"
    )
    
    # 习惯得分，0-100，越高表示习惯越稳定
    habit_score: Mapped[float] = mapped_column(
        Float, 
        default=0.0, 
        comment="习惯得分"
    )


class ImageMemory(Base):
    """
    图像记忆表
    
    存储人脸特征、感知视频关键帧等图像类型的记忆。
    图像本身存储在 MinIO，这里只存储元数据和特征向量。
    
    字段说明：
        image_path: 图像文件路径（MinIO 中的路径）
        type: 图像类型，如 face（人脸）、scene（场景）、frame（视频帧）
        source_device: 来源设备 ID
        embedding: 图像特征向量，用于相似图像检索
        labels: 图像标签，JSON 数组
        face_id: 人脸 ID，用于人脸识别关联
    """
    
    __tablename__ = "image_memories"
    __table_args__ = {"comment": "图像记忆表"}
    
    # 图像文件路径，存储在 MinIO 中的对象路径
    image_path: Mapped[str] = mapped_column(
        String(500), 
        comment="图像文件路径"
    )
    
    # 图像类型：face（人脸）、scene（场景）、frame（视频帧）
    type: Mapped[str] = mapped_column(
        String(50), 
        index=True, 
        comment="图像类型"
    )
    
    # 来源设备 ID，记录是哪个摄像头拍摄的
    source_device: Mapped[Optional[str]] = mapped_column(
        String(64), 
        nullable=True, 
        index=True, 
        comment="来源设备ID"
    )
    
    # 图像特征向量，维度 512（CLIP 模型输出）
    # 用于图像相似度检索
    embedding: Mapped[Optional[list]] = mapped_column(
        Vector(512), 
        nullable=True, 
        comment="图像特征向量"
    )
    
    # 图像标签，JSON 数组
    # 例如：["客厅", "白天", "有人"]
    labels: Mapped[Optional[list]] = mapped_column(
        JSON, 
        nullable=True, 
        comment="图像标签(JSON)"
    )
    
    # 人脸 ID，如果是人脸图像，关联到人脸库
    face_id: Mapped[Optional[str]] = mapped_column(
        String(64), 
        nullable=True, 
        index=True, 
        comment="人脸ID"
    )
    
    # 元数据，存储额外信息
    # 例如：{"capture_time": "2026-01-09 10:30:00", "confidence": 0.95}
    metadata: Mapped[Optional[dict]] = mapped_column(
        JSON, 
        nullable=True, 
        comment="元数据(JSON)"
    )
