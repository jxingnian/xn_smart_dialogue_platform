"""
意图识别引擎 - 理解用户意图并做出决策
"""
from dataclasses import dataclass
from typing import Optional, List, Dict
from enum import Enum


class IntentType(Enum):
    """意图类型"""
    # 显式意图
    DEVICE_CONTROL = "device_control"
    INFO_QUERY = "info_query"
    CONTENT_PLAY = "content_play"
    TIMER_SET = "timer_set"
    SCENE_TRIGGER = "scene_trigger"
    
    # 隐式意图
    ENVIRONMENT_ADJUST = "environment_adjust"
    MOOD_SETTING = "mood_setting"
    SAFETY_CHECK = "safety_check"
    HEALTH_CARE = "health_care"


class DecisionStrategy(Enum):
    """决策策略"""
    DIRECT_EXECUTE = "direct_execute"           # 直接执行
    EXECUTE_AND_INFORM = "execute_and_inform"   # 执行并告知
    ASK_CONFIRMATION = "ask_confirmation"       # 询问确认
    PROACTIVE_SUGGESTION = "proactive_suggestion"  # 主动建议
    SILENT_OBSERVE = "silent_observe"           # 静默观察


@dataclass
class Intent:
    """意图"""
    type: IntentType
    category: str
    target: Optional[str]
    confidence: float
    entities: Dict
    is_implicit: bool


@dataclass
class DeviceMatch:
    """设备匹配结果"""
    device_id: str
    device_type: str
    capability: str
    current_state: Dict
    match_score: float


@dataclass
class Action:
    """执行动作"""
    action_type: str  # voice_response, device_control, notification
    device_id: Optional[str]
    action: Optional[str]
    params: Dict
    requires_confirmation: bool


@dataclass
class Decision:
    """决策结果"""
    strategy: DecisionStrategy
    actions: List[Action]
    response_text: Optional[str]
    pending_actions: List[Action]  # 等待确认的动作


@dataclass
class IntentResult:
    """意图识别完整结果"""
    intent_id: str
    timestamp: str
    input_text: str
    intent: Intent
    device_matches: List[DeviceMatch]
    decision: Decision


class IntentEngine:
    """意图识别引擎"""
    
    def __init__(self, config: Dict, device_manager):
        self.config = config
        self.device_manager = device_manager
        self.context_manager = ContextManager()
        self.user_preferences = {}
    
    def process(
        self,
        text: str,
        audio_result: Dict,
        scene_result: Dict,
        user_id: str
    ) -> IntentResult:
        """处理意图"""
        # 1. 意图理解
        intent = self._understand_intent(text, audio_result, scene_result)
        
        # 2. 设备能力匹配
        device_matches = self._match_devices(intent, user_id)
        
        # 3. 获取环境数据
        env_data = self._get_environment_data(device_matches)
        
        # 4. 决策
        decision = self._make_decision(intent, device_matches, env_data, user_id)
        
        # 5. 更新上下文
        self.context_manager.update(user_id, intent, decision)
        
        return self._create_result(text, intent, device_matches, decision)
    
    def _understand_intent(
        self,
        text: str,
        audio_result: Dict,
        scene_result: Dict
    ) -> Intent:
        """理解用户意图"""
        # 1. 显式意图识别
        explicit_intent = self._recognize_explicit_intent(text)
        
        if explicit_intent and explicit_intent.confidence > 0.7:
            return explicit_intent
        
        # 2. 隐式意图挖掘
        implicit_intent = self._mine_implicit_intent(text, audio_result, scene_result)
        
        if implicit_intent:
            return implicit_intent
        
        return explicit_intent or self._create_unknown_intent()
    
    def _recognize_explicit_intent(self, text: str) -> Optional[Intent]:
        """识别显式意图"""
        # 设备控制意图
        control_patterns = {
            "打开": ("turn_on", IntentType.DEVICE_CONTROL),
            "关闭": ("turn_off", IntentType.DEVICE_CONTROL),
            "关": ("turn_off", IntentType.DEVICE_CONTROL),
            "开": ("turn_on", IntentType.DEVICE_CONTROL),
            "调到": ("set_value", IntentType.DEVICE_CONTROL),
            "调高": ("increase", IntentType.DEVICE_CONTROL),
            "调低": ("decrease", IntentType.DEVICE_CONTROL),
        }
        
        for pattern, (action, intent_type) in control_patterns.items():
            if pattern in text:
                entities = self._extract_entities(text)
                return Intent(
                    type=intent_type,
                    category=action,
                    target=entities.get("device"),
                    confidence=0.85,
                    entities=entities,
                    is_implicit=False
                )
        
        # TODO: 更多意图识别逻辑
        return None
    
    def _mine_implicit_intent(
        self,
        text: str,
        audio_result: Dict,
        scene_result: Dict
    ) -> Optional[Intent]:
        """挖掘隐式意图"""
        # 环境相关隐式意图
        env_complaints = {
            "干燥": ("humidity", "humidifier", "increase"),
            "潮湿": ("humidity", "dehumidifier", "decrease"),
            "热": ("temperature", "ac", "cool"),
            "冷": ("temperature", "ac", "heat"),
            "暗": ("light", "light", "increase"),
            "亮": ("light", "light", "decrease"),
        }
        
        for keyword, (category, device, action) in env_complaints.items():
            if keyword in text:
                return Intent(
                    type=IntentType.ENVIRONMENT_ADJUST,
                    category=category,
                    target=device,
                    confidence=0.7,
                    entities={"condition": keyword, "suggested_action": action},
                    is_implicit=True
                )
        
        return None
    
    def _extract_entities(self, text: str) -> Dict:
        """提取实体"""
        entities = {}
        
        # 设备实体
        device_keywords = {
            "灯": "light",
            "空调": "ac",
            "加湿器": "humidifier",
            "电视": "tv",
            "窗帘": "curtain",
            "音箱": "speaker"
        }
        for kw, device_type in device_keywords.items():
            if kw in text:
                entities["device"] = device_type
                break
        
        # 数值实体
        import re
        numbers = re.findall(r'\d+', text)
        if numbers:
            entities["value"] = int(numbers[0])
        
        # 房间实体
        room_keywords = ["客厅", "卧室", "厨房", "卫生间", "书房"]
        for room in room_keywords:
            if room in text:
                entities["room"] = room
                break
        
        return entities
    
    def _match_devices(self, intent: Intent, user_id: str) -> List[DeviceMatch]:
        """匹配可用设备"""
        if not intent.target:
            return []
        
        # 从设备管理器获取用户设备
        user_devices = self.device_manager.get_user_devices(user_id)
        
        matches = []
        for device in user_devices:
            if self._device_matches_intent(device, intent):
                match = DeviceMatch(
                    device_id=device["device_id"],
                    device_type=device["device_type"],
                    capability=self._get_matching_capability(device, intent),
                    current_state=device.get("status", {}),
                    match_score=self._calculate_match_score(device, intent)
                )
                matches.append(match)
        
        return sorted(matches, key=lambda x: x.match_score, reverse=True)
    
    def _device_matches_intent(self, device: Dict, intent: Intent) -> bool:
        """判断设备是否匹配意图"""
        # TODO: 实现设备匹配逻辑
        return device.get("device_type") == intent.target
    
    def _get_matching_capability(self, device: Dict, intent: Intent) -> str:
        """获取匹配的设备能力"""
        # TODO: 实现
        return "power"
    
    def _calculate_match_score(self, device: Dict, intent: Intent) -> float:
        """计算匹配分数"""
        # TODO: 实现
        return 0.9
    
    def _get_environment_data(self, device_matches: List[DeviceMatch]) -> Dict:
        """获取环境数据"""
        env_data = {}
        for match in device_matches:
            if "humidity" in match.current_state:
                env_data["humidity"] = match.current_state["humidity"]
            if "temperature" in match.current_state:
                env_data["temperature"] = match.current_state["temperature"]
        return env_data
    
    def _make_decision(
        self,
        intent: Intent,
        device_matches: List[DeviceMatch],
        env_data: Dict,
        user_id: str
    ) -> Decision:
        """做出决策"""
        user_prefs = self.user_preferences.get(user_id, {})
        
        # 决策矩阵
        if intent.is_implicit:
            # 隐式意图 -> 主动建议或询问
            if device_matches:
                return self._create_suggestion_decision(intent, device_matches, env_data)
            return self._create_observe_decision()
        
        # 显式意图
        if intent.confidence > 0.8 and device_matches:
            # 高置信度 -> 执行
            if self._is_safe_action(intent):
                return self._create_execute_decision(intent, device_matches)
            else:
                return self._create_confirmation_decision(intent, device_matches)
        
        # 低置信度 -> 询问
        return self._create_confirmation_decision(intent, device_matches)
    
    def _is_safe_action(self, intent: Intent) -> bool:
        """判断是否为安全操作"""
        safe_categories = ["turn_on", "turn_off", "set_value"]
        return intent.category in safe_categories
    
    def _create_execute_decision(
        self,
        intent: Intent,
        device_matches: List[DeviceMatch]
    ) -> Decision:
        """创建执行决策"""
        actions = []
        for match in device_matches[:1]:  # 只取最匹配的设备
            actions.append(Action(
                action_type="device_control",
                device_id=match.device_id,
                action=intent.category,
                params=intent.entities,
                requires_confirmation=False
            ))
        
        return Decision(
            strategy=DecisionStrategy.EXECUTE_AND_INFORM,
            actions=actions,
            response_text=f"好的，已为您{intent.category}",
            pending_actions=[]
        )
    
    def _create_suggestion_decision(
        self,
        intent: Intent,
        device_matches: List[DeviceMatch],
        env_data: Dict
    ) -> Decision:
        """创建建议决策"""
        match = device_matches[0]
        
        # 生成建议文本
        if intent.category == "humidity" and "humidity" in env_data:
            response = f"检测到室内湿度为{env_data['humidity']}%，需要我帮您打开加湿器吗？"
        else:
            response = f"需要我帮您调节{intent.target}吗？"
        
        pending_action = Action(
            action_type="device_control",
            device_id=match.device_id,
            action="turn_on",
            params={},
            requires_confirmation=True
        )
        
        return Decision(
            strategy=DecisionStrategy.PROACTIVE_SUGGESTION,
            actions=[Action(
                action_type="voice_response",
                device_id=None,
                action=None,
                params={"text": response},
                requires_confirmation=False
            )],
            response_text=response,
            pending_actions=[pending_action]
        )
    
    def _create_confirmation_decision(
        self,
        intent: Intent,
        device_matches: List[DeviceMatch]
    ) -> Decision:
        """创建确认决策"""
        # TODO: 实现
        pass
    
    def _create_observe_decision(self) -> Decision:
        """创建观察决策"""
        return Decision(
            strategy=DecisionStrategy.SILENT_OBSERVE,
            actions=[],
            response_text=None,
            pending_actions=[]
        )
    
    def _create_unknown_intent(self) -> Intent:
        """创建未知意图"""
        return Intent(
            type=IntentType.INFO_QUERY,
            category="unknown",
            target=None,
            confidence=0.0,
            entities={},
            is_implicit=False
        )
    
    def _create_result(
        self,
        text: str,
        intent: Intent,
        device_matches: List[DeviceMatch],
        decision: Decision
    ) -> IntentResult:
        """创建意图结果"""
        import uuid
        from datetime import datetime
        
        return IntentResult(
            intent_id=str(uuid.uuid4()),
            timestamp=datetime.utcnow().isoformat() + "Z",
            input_text=text,
            intent=intent,
            device_matches=device_matches,
            decision=decision
        )


class ContextManager:
    """上下文管理器"""
    
    def __init__(self):
        self.contexts = {}  # user_id -> context
    
    def update(self, user_id: str, intent: Intent, decision: Decision):
        """更新上下文"""
        if user_id not in self.contexts:
            self.contexts[user_id] = {"history": []}
        
        self.contexts[user_id]["history"].append({
            "intent": intent,
            "decision": decision
        })
        
        # 保留最近10轮对话
        self.contexts[user_id]["history"] = self.contexts[user_id]["history"][-10:]
    
    def get(self, user_id: str) -> Dict:
        """获取上下文"""
        return self.contexts.get(user_id, {"history": []})
