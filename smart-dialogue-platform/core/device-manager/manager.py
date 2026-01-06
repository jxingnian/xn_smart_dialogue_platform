"""
设备管理模块 - 设备注册、能力管理、状态同步
"""
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Callable
from enum import Enum
import uuid
from datetime import datetime


class DeviceStatus(Enum):
    """设备状态"""
    ONLINE = "online"
    OFFLINE = "offline"
    UNKNOWN = "unknown"


class CapabilityType(Enum):
    """能力类型"""
    SWITCH = "switch"
    RANGE = "range"
    ENUM = "enum"
    COLOR = "color"
    POSITION = "position"
    TIMER = "timer"


@dataclass
class Capability:
    """设备能力"""
    name: str
    type: CapabilityType
    readable: bool = True
    writable: bool = True
    params: Dict = field(default_factory=dict)


@dataclass
class Device:
    """设备"""
    device_id: str
    device_type: str
    device_name: str
    user_id: str
    manufacturer: Optional[str] = None
    model: Optional[str] = None
    location: Dict = field(default_factory=dict)
    capabilities: List[Capability] = field(default_factory=list)
    sensors: Dict = field(default_factory=dict)
    status: DeviceStatus = DeviceStatus.UNKNOWN
    current_state: Dict = field(default_factory=dict)
    last_seen: Optional[str] = None
    created_at: Optional[str] = None


class DeviceManager:
    """设备管理器"""
    
    def __init__(self, config: Dict):
        self.config = config
        self.devices: Dict[str, Device] = {}
        self.user_devices: Dict[str, List[str]] = {}  # user_id -> [device_ids]
        self.command_handlers: Dict[str, Callable] = {}
        self.status_listeners: List[Callable] = []
    
    def register_device(self, device_info: Dict, user_id: str) -> Device:
        """注册设备"""
        device_id = device_info.get("device_id") or str(uuid.uuid4())
        
        # 解析能力
        capabilities = self._parse_capabilities(device_info.get("capabilities", {}))
        
        device = Device(
            device_id=device_id,
            device_type=device_info["device_type"],
            device_name=device_info["device_name"],
            user_id=user_id,
            manufacturer=device_info.get("manufacturer"),
            model=device_info.get("model"),
            location=device_info.get("location", {}),
            capabilities=capabilities,
            sensors=device_info.get("sensors", {}),
            status=DeviceStatus.ONLINE,
            current_state={},
            last_seen=datetime.utcnow().isoformat() + "Z",
            created_at=datetime.utcnow().isoformat() + "Z"
        )
        
        self.devices[device_id] = device
        
        if user_id not in self.user_devices:
            self.user_devices[user_id] = []
        self.user_devices[user_id].append(device_id)
        
        return device
    
    def _parse_capabilities(self, capabilities_dict: Dict) -> List[Capability]:
        """解析能力定义"""
        capabilities = []
        for name, cap_info in capabilities_dict.items():
            cap_type = CapabilityType(cap_info.get("type", "switch"))
            capabilities.append(Capability(
                name=name,
                type=cap_type,
                readable=cap_info.get("readable", True),
                writable=cap_info.get("writable", True),
                params=cap_info
            ))
        return capabilities
    
    def unregister_device(self, device_id: str) -> bool:
        """注销设备"""
        if device_id not in self.devices:
            return False
        
        device = self.devices[device_id]
        user_id = device.user_id
        
        del self.devices[device_id]
        
        if user_id in self.user_devices:
            self.user_devices[user_id].remove(device_id)
        
        return True
    
    def get_device(self, device_id: str) -> Optional[Device]:
        """获取设备"""
        return self.devices.get(device_id)
    
    def get_user_devices(self, user_id: str) -> List[Dict]:
        """获取用户所有设备"""
        device_ids = self.user_devices.get(user_id, [])
        devices = []
        for device_id in device_ids:
            device = self.devices.get(device_id)
            if device:
                devices.append(self._device_to_dict(device))
        return devices
    
    def get_devices_by_type(self, user_id: str, device_type: str) -> List[Device]:
        """按类型获取设备"""
        devices = []
        for device_id in self.user_devices.get(user_id, []):
            device = self.devices.get(device_id)
            if device and device.device_type == device_type:
                devices.append(device)
        return devices
    
    def get_devices_by_location(self, user_id: str, room: str) -> List[Device]:
        """按位置获取设备"""
        devices = []
        for device_id in self.user_devices.get(user_id, []):
            device = self.devices.get(device_id)
            if device and device.location.get("room") == room:
                devices.append(device)
        return devices
    
    def update_device_status(self, device_id: str, status_data: Dict):
        """更新设备状态"""
        device = self.devices.get(device_id)
        if not device:
            return
        
        device.current_state.update(status_data.get("status", {}))
        device.last_seen = datetime.utcnow().isoformat() + "Z"
        
        if status_data.get("online") is not None:
            device.status = DeviceStatus.ONLINE if status_data["online"] else DeviceStatus.OFFLINE
        
        # 通知监听器
        for listener in self.status_listeners:
            listener(device_id, device.current_state)
    
    def send_command(self, device_id: str, command: Dict) -> Dict:
        """发送控制指令"""
        device = self.devices.get(device_id)
        if not device:
            return {"status": "error", "message": "Device not found"}
        
        if device.status != DeviceStatus.ONLINE:
            return {"status": "error", "message": "Device offline"}
        
        # 验证能力
        action = command.get("action")
        if not self._validate_command(device, command):
            return {"status": "error", "message": "Invalid command"}
        
        # 调用命令处理器
        handler = self.command_handlers.get(device.device_type)
        if handler:
            result = handler(device_id, command)
            return result
        
        # 默认处理：更新状态
        if command.get("properties"):
            device.current_state.update(command["properties"])
        
        return {
            "status": "success",
            "command_id": command.get("command_id"),
            "result": device.current_state
        }
    
    def _validate_command(self, device: Device, command: Dict) -> bool:
        """验证命令是否有效"""
        properties = command.get("properties", {})
        
        for prop_name, value in properties.items():
            # 查找对应能力
            capability = None
            for cap in device.capabilities:
                if cap.name == prop_name:
                    capability = cap
                    break
            
            if not capability:
                return False
            
            if not capability.writable:
                return False
            
            # 验证值范围
            if capability.type == CapabilityType.RANGE:
                min_val = capability.params.get("min", 0)
                max_val = capability.params.get("max", 100)
                if not (min_val <= value <= max_val):
                    return False
            
            if capability.type == CapabilityType.ENUM:
                valid_values = capability.params.get("values", [])
                if value not in valid_values:
                    return False
        
        return True
    
    def register_command_handler(self, device_type: str, handler: Callable):
        """注册命令处理器"""
        self.command_handlers[device_type] = handler
    
    def add_status_listener(self, listener: Callable):
        """添加状态监听器"""
        self.status_listeners.append(listener)
    
    def _device_to_dict(self, device: Device) -> Dict:
        """设备转字典"""
        return {
            "device_id": device.device_id,
            "device_type": device.device_type,
            "device_name": device.device_name,
            "manufacturer": device.manufacturer,
            "model": device.model,
            "location": device.location,
            "capabilities": [
                {
                    "name": cap.name,
                    "type": cap.type.value,
                    "readable": cap.readable,
                    "writable": cap.writable,
                    "params": cap.params
                }
                for cap in device.capabilities
            ],
            "sensors": device.sensors,
            "status": device.status.value,
            "current_state": device.current_state,
            "last_seen": device.last_seen
        }
    
    def get_device_capabilities_summary(self, user_id: str) -> Dict:
        """获取用户设备能力汇总"""
        summary = {
            "total_devices": 0,
            "by_type": {},
            "by_room": {},
            "capabilities": set()
        }
        
        for device_id in self.user_devices.get(user_id, []):
            device = self.devices.get(device_id)
            if not device:
                continue
            
            summary["total_devices"] += 1
            
            # 按类型统计
            if device.device_type not in summary["by_type"]:
                summary["by_type"][device.device_type] = []
            summary["by_type"][device.device_type].append(device.device_id)
            
            # 按房间统计
            room = device.location.get("room", "unknown")
            if room not in summary["by_room"]:
                summary["by_room"][room] = []
            summary["by_room"][room].append(device.device_id)
            
            # 能力汇总
            for cap in device.capabilities:
                summary["capabilities"].add(cap.name)
        
        summary["capabilities"] = list(summary["capabilities"])
        return summary
