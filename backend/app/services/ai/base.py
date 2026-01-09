# -*- coding: utf-8 -*-
"""
AI 服务抽象基类

功能说明：
    定义所有 AI 能力的抽象接口。
    具体实现（本地/云端）继承这些接口，实现具体的调用逻辑。
    
    这种设计的好处：
        - 调用方不需要关心具体实现，只依赖抽象接口
        - 可以随时切换本地和云端实现，不影响业务代码
        - 便于单元测试时 mock
"""

from abc import ABC, abstractmethod
from typing import Optional


class LLMProvider(ABC):
    """
    大语言模型服务抽象接口
    
    用于对话、文本生成等场景。
    实现类：OllamaProvider（本地）、DashScopeProvider（云端）
    """
    
    @abstractmethod
    async def chat(
        self, 
        messages: list[dict], 
        temperature: float = 0.7,
        max_tokens: Optional[int] = None
    ) -> str:
        """
        对话接口
        
        参数：
            messages: 对话历史，格式 [{"role": "user", "content": "你好"}]
            temperature: 温度参数，控制回复的随机性，0-1 之间
            max_tokens: 最大生成 token 数，None 表示不限制
            
        返回：
            模型生成的回复文本
        """
        pass


class ASRProvider(ABC):
    """
    语音识别服务抽象接口
    
    将语音转换为文字。
    实现类：WhisperProvider（本地）、DashScopeASRProvider（云端）
    """
    
    @abstractmethod
    async def transcribe(self, audio_data: bytes, format: str = "wav") -> str:
        """
        语音转文字
        
        参数：
            audio_data: 音频数据（二进制）
            format: 音频格式，如 wav、mp3
            
        返回：
            识别出的文字
        """
        pass


class TTSProvider(ABC):
    """
    语音合成服务抽象接口
    
    将文字转换为语音。
    实现类：EdgeTTSProvider（本地）、DashScopeTTSProvider（云端）
    """
    
    @abstractmethod
    async def synthesize(self, text: str, voice: Optional[str] = None) -> bytes:
        """
        文字转语音
        
        参数：
            text: 要合成的文字
            voice: 音色，None 表示使用默认音色
            
        返回：
            音频数据（二进制）
        """
        pass


class EmbeddingProvider(ABC):
    """
    文本向量化服务抽象接口
    
    将文本转换为向量，用于语义检索。
    实现类：BGEProvider（本地）、DashScopeEmbeddingProvider（云端）
    """
    
    @abstractmethod
    async def embed(self, texts: list[str]) -> list[list[float]]:
        """
        文本向量化
        
        参数：
            texts: 文本列表
            
        返回：
            向量列表，每个向量是一个浮点数列表
        """
        pass


class FaceRecognitionProvider(ABC):
    """
    人脸识别服务抽象接口
    
    用于人脸检测、特征提取、人脸比对。
    实现类：InsightFaceProvider（本地）、AliyunFaceProvider（云端）
    """
    
    @abstractmethod
    async def detect(self, image_data: bytes) -> list[dict]:
        """
        人脸检测
        
        参数：
            image_data: 图像数据（二进制）
            
        返回：
            检测到的人脸列表，每个人脸包含位置、特征等信息
        """
        pass
    
    @abstractmethod
    async def extract_feature(self, image_data: bytes) -> Optional[list[float]]:
        """
        提取人脸特征向量
        
        参数：
            image_data: 包含人脸的图像数据
            
        返回：
            人脸特征向量，如果没有检测到人脸返回 None
        """
        pass
    
    @abstractmethod
    async def compare(self, feature1: list[float], feature2: list[float]) -> float:
        """
        人脸比对
        
        参数：
            feature1: 第一个人脸特征向量
            feature2: 第二个人脸特征向量
            
        返回：
            相似度得分，0-1 之间，越高越相似
        """
        pass


class ImageUnderstandingProvider(ABC):
    """
    图像理解服务抽象接口
    
    用于图像描述、场景识别、物体检测等。
    实现类：QwenVLLocalProvider（本地）、DashScopeVLProvider（云端）
    """
    
    @abstractmethod
    async def describe(self, image_data: bytes, prompt: Optional[str] = None) -> str:
        """
        图像描述
        
        参数：
            image_data: 图像数据（二进制）
            prompt: 可选的提示词，引导模型关注特定内容
            
        返回：
            图像描述文本
        """
        pass
    
    @abstractmethod
    async def embed(self, image_data: bytes) -> list[float]:
        """
        图像向量化
        
        参数：
            image_data: 图像数据（二进制）
            
        返回：
            图像特征向量
        """
        pass
