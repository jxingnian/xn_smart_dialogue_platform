"""
智能对话平台 Python SDK
"""
from .device import Device
from .capability import Capability, SwitchCapability, RangeCapability, EnumCapability

__version__ = "0.1.0"

__all__ = [
    "Device",
    "Capability",
    "SwitchCapability",
    "RangeCapability",
    "EnumCapability"
]
