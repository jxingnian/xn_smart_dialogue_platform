"""
设备能力定义
"""
from typing import List, Optional
from dataclasses import dataclass


class Capability:
    """能力基类"""
    
    def __init__(self, readable: bool = True, writable: bool = True):
        self.readable = readable
        self.writable = writable
    
    def to_dict(self) -> dict:
        raise NotImplementedError


class SwitchCapability(Capability):
    """开关能力"""
    
    def __init__(self, readable: bool = True, writable: bool = True):
        super().__init__(readable, writable)
    
    def to_dict(self) -> dict:
        return {
            "type": "switch",
            "actions": ["turn_on", "turn_off", "toggle"],
            "readable": self.readable,
            "writable": self.writable
        }
    
    @staticmethod
    def Switch():
        """快捷创建"""
        return SwitchCapability()


class RangeCapability(Capability):
    """范围能力"""
    
    def __init__(
        self,
        min_val: float,
        max_val: float,
        step: float = 1,
        unit: str = "",
        readable: bool = True,
        writable: bool = True
    ):
        super().__init__(readable, writable)
        self.min_val = min_val
        self.max_val = max_val
        self.step = step
        self.unit = unit
    
    def to_dict(self) -> dict:
        return {
            "type": "range",
            "min": self.min_val,
            "max": self.max_val,
            "step": self.step,
            "unit": self.unit,
            "readable": self.readable,
            "writable": self.writable
        }
    
    @staticmethod
    def Range(min_val: float, max_val: float, step: float = 1, unit: str = ""):
        """快捷创建"""
        return RangeCapability(min_val, max_val, step, unit)


class EnumCapability(Capability):
    """枚举能力"""
    
    def __init__(
        self,
        values: List[str],
        readable: bool = True,
        writable: bool = True
    ):
        super().__init__(readable, writable)
        self.values = values
    
    def to_dict(self) -> dict:
        return {
            "type": "enum",
            "values": self.values,
            "readable": self.readable,
            "writable": self.writable
        }
    
    @staticmethod
    def Enum(values: List[str]):
        """快捷创建"""
        return EnumCapability(values)


class ColorCapability(Capability):
    """颜色能力"""
    
    def __init__(
        self,
        color_mode: str = "rgb",  # rgb, hsv, temperature
        readable: bool = True,
        writable: bool = True
    ):
        super().__init__(readable, writable)
        self.color_mode = color_mode
    
    def to_dict(self) -> dict:
        return {
            "type": "color",
            "color_mode": self.color_mode,
            "readable": self.readable,
            "writable": self.writable
        }


class PositionCapability(Capability):
    """位置能力（如窗帘开合度）"""
    
    def __init__(
        self,
        min_val: float = 0,
        max_val: float = 100,
        unit: str = "%",
        readable: bool = True,
        writable: bool = True
    ):
        super().__init__(readable, writable)
        self.min_val = min_val
        self.max_val = max_val
        self.unit = unit
    
    def to_dict(self) -> dict:
        return {
            "type": "position",
            "min": self.min_val,
            "max": self.max_val,
            "unit": self.unit,
            "readable": self.readable,
            "writable": self.writable
        }
