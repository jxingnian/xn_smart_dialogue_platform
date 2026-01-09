# -*- coding: utf-8 -*-
"""
通义千问 AI 服务实现

功能说明：
    调用阿里云通义千问 API 实现 LLM 对话功能。
    支持文本模型和多模态模型（qwen-omni-turbo）。
"""

from typing import Optional, List
import httpx

from app.services.ai.base import LLMProvider


class DashScopeProvider(LLMProvider):
    """
    通义千问 LLM 服务
    
    使用阿里云 DashScope API 调用通义千问模型。
    支持多轮对话，支持多模态输入。
    """
    
    def __init__(self, api_key: str, model: str = "qwen-omni-turbo"):
        """
        初始化通义千问服务
        
        参数：
            api_key: DashScope API Key
            model: 模型名称
        """
        self.model = model
        self.api_key = api_key
        self.base_url = "https://dashscope.aliyuncs.com/compatible-mode/v1"
    
    async def chat(
        self,
        messages: List[dict],
        temperature: float = 0.7,
        max_tokens: Optional[int] = None
    ) -> str:
        """
        对话接口
        
        参数：
            messages: 对话历史，格式 [{"role": "user", "content": "你好"}]
            temperature: 温度参数，控制回复的随机性
            max_tokens: 最大生成 token 数
            
        返回：
            模型生成的回复文本
        """
        if not self.api_key:
            return "错误：未配置 API Key，请在系统设置中配置"
        
        try:
            # 构建请求
            headers = {
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json",
            }
            
            payload = {
                "model": self.model,
                "messages": messages,
                "temperature": temperature,
            }
            
            if max_tokens:
                payload["max_tokens"] = max_tokens
            
            # 发送请求
            async with httpx.AsyncClient(timeout=60.0) as client:
                response = await client.post(
                    f"{self.base_url}/chat/completions",
                    headers=headers,
                    json=payload,
                )
                
                if response.status_code != 200:
                    error_data = response.json()
                    error_msg = error_data.get("error", {}).get("message", response.text)
                    return f"错误：API 调用失败 - {error_msg}"
                
                data = response.json()
                return data["choices"][0]["message"]["content"]
                
        except httpx.TimeoutException:
            return "错误：请求超时，请稍后重试"
        except Exception as e:
            return f"错误：{str(e)}"
    
    async def chat_with_image(
        self,
        messages: List[dict],
        image_url: str,
        temperature: float = 0.7,
    ) -> str:
        """
        图文对话接口（多模态）
        
        参数：
            messages: 对话历史
            image_url: 图片 URL 或 base64
            temperature: 温度参数
            
        返回：
            模型生成的回复文本
        """
        if not self.api_key:
            return "错误：未配置 API Key"
        
        # 构建多模态消息
        # 将最后一条用户消息改为包含图片
        if messages and messages[-1]["role"] == "user":
            text_content = messages[-1]["content"]
            messages[-1]["content"] = [
                {"type": "image_url", "image_url": {"url": image_url}},
                {"type": "text", "text": text_content},
            ]
        
        return await self.chat(messages, temperature)
