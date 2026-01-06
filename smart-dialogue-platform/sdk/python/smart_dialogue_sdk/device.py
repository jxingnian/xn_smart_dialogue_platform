"""
设备 SDK - 设备接入封装
"""
from typing import Dict, Optional, Callable, List
import json
import asyncio


class Device:
    """智能设备"""
    
    def __init__(
        self,
        device_type: str,
        device_name: str,
        capabilities: Dict = None,
        device_id: str = None
    ):
        self.device_id = device_id
        self.device_type = device_type
        self.device_name = device_name
        self.capabilities = capabilities or {}
        self.sensors = {}
        self.location = {}
        
        self._connection = None
        self._command_handler: Optional[Callable] = None
        self._is_connected = False
        self._current_state = {}
    
    def set_location(self, room: str, zone: str = None) -> 'Device':
        """设置设备位置"""
        self.location = {"room": room}
        if zone:
            self.location["zone"] = zone
        return self
    
    def add_sensor(self, name: str, unit: str, report_interval: int = 60) -> 'Device':
        """添加传感器"""
        self.sensors[name] = {
            "type": "number",
            "unit": unit,
            "report_interval": report_interval
        }
        return self
    
    async def register(self, platform_url: str, device_token: str) -> bool:
        """注册到平台"""
        # 构建注册信息
        registration = {
            "type": "register",
            "device_id": self.device_id,
            "device_type": self.device_type,
            "device_name": self.device_name,
            "capabilities": self._serialize_capabilities(),
            "sensors": self.sensors,
            "location": self.location,
            "token": device_token
        }
        
        # TODO: 建立连接并发送注册请求
        # self._connection = await self._connect(platform_url)
        # response = await self._send_and_wait(registration)
        
        self._is_connected = True
        return True
    
    def _serialize_capabilities(self) -> Dict:
        """序列化能力定义"""
        result = {}
        for name, cap in self.capabilities.items():
            if hasattr(cap, 'to_dict'):
                result[name] = cap.to_dict()
            else:
                result[name] = cap
        return result
    
    def on_command(self, handler: Callable):
        """注册命令处理器（装饰器）"""
        self._command_handler = handler
        return handler
    
    async def _handle_command(self, command: Dict):
        """处理平台下发的命令"""
        if self._command_handler:
            result = await self._command_handler(command)
            # 发送响应
            await self._send_response(command.get("command_id"), result)
    
    async def _send_response(self, command_id: str, result: Dict):
        """发送命令响应"""
        response = {
            "type": "command_response",
            "command_id": command_id,
            "device_id": self.device_id,
            **result
        }
        # TODO: 发送响应
    
    async def report_status(self, status: Dict):
        """上报设备状态"""
        self._current_state.update(status)
        
        message = {
            "type": "status_report",
            "device_id": self.device_id,
            "status": status,
            "timestamp": self._get_timestamp()
        }
        
        # TODO: 发送状态上报
    
    async def report_audio(self, audio_data: bytes, metadata: Dict = None):
        """上报音频数据"""
        import base64
        
        message = {
            "type": "audio_report",
            "device_id": self.device_id,
            "audio": {
                "format": "pcm",
                "sample_rate": 16000,
                "channels": 1,
                "data": base64.b64encode(audio_data).decode()
            },
            "metadata": metadata or {},
            "timestamp": self._get_timestamp()
        }
        
        # TODO: 发送音频数据
    
    async def start_audio_stream(self, sample_rate: int = 16000, channels: int = 1):
        """开始音频流上报"""
        # TODO: 实现音频流采集和上报
        pass
    
    async def disconnect(self):
        """断开连接"""
        if self._connection:
            # TODO: 关闭连接
            pass
        self._is_connected = False
    
    def _get_timestamp(self) -> str:
        """获取时间戳"""
        from datetime import datetime
        return datetime.utcnow().isoformat() + "Z"
    
    @property
    def is_connected(self) -> bool:
        """是否已连接"""
        return self._is_connected
    
    @property
    def current_state(self) -> Dict:
        """当前状态"""
        return self._current_state.copy()
