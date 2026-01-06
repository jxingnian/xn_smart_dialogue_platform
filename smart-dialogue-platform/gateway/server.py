"""
接入网关 - 处理设备连接和消息路由
"""
from typing import Dict, Optional
import asyncio
import json


class DeviceGateway:
    """设备接入网关"""
    
    def __init__(self, config: Dict):
        self.config = config
        self.connections: Dict[str, 'DeviceConnection'] = {}
        self.message_handlers = {}
    
    async def start(self):
        """启动网关"""
        # 启动 WebSocket 服务
        ws_port = self.config.get("websocket_port", 8080)
        # TODO: 启动 WebSocket 服务器
        
        # 启动 MQTT 客户端
        mqtt_config = self.config.get("mqtt", {})
        # TODO: 连接 MQTT broker
        
        print(f"Gateway started on port {ws_port}")
    
    async def handle_device_connect(self, device_id: str, connection):
        """处理设备连接"""
        self.connections[device_id] = connection
        print(f"Device connected: {device_id}")
    
    async def handle_device_disconnect(self, device_id: str):
        """处理设备断开"""
        if device_id in self.connections:
            del self.connections[device_id]
        print(f"Device disconnected: {device_id}")
    
    async def handle_message(self, device_id: str, message: Dict):
        """处理设备消息"""
        msg_type = message.get("type")
        
        handler = self.message_handlers.get(msg_type)
        if handler:
            await handler(device_id, message)
        else:
            print(f"Unknown message type: {msg_type}")
    
    async def send_to_device(self, device_id: str, message: Dict) -> bool:
        """发送消息到设备"""
        connection = self.connections.get(device_id)
        if not connection:
            return False
        
        try:
            await connection.send(json.dumps(message))
            return True
        except Exception as e:
            print(f"Failed to send message to {device_id}: {e}")
            return False
    
    def register_handler(self, msg_type: str, handler):
        """注册消息处理器"""
        self.message_handlers[msg_type] = handler


class DeviceConnection:
    """设备连接抽象"""
    
    def __init__(self, websocket=None, mqtt_client=None):
        self.websocket = websocket
        self.mqtt_client = mqtt_client
    
    async def send(self, message: str):
        """发送消息"""
        if self.websocket:
            await self.websocket.send(message)
        elif self.mqtt_client:
            # TODO: MQTT 发送
            pass
