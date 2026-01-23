<!--
 * @Author: xingnian jixingnian@gmail.com
 * @Date: 2026-01-22 19:45:40
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2026-01-23 11:59:38
 * @FilePath: \xn_smart_dialogue_platform\device\xn_esp32_web_manager\README.md
 * @Description: AI语音对话项目 esp32设备端 readme说明文档
 * VX:Jxingnian
 * Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
-->

代码注释要求
1. 头文件需对所有代码逐行注释、尽可能一句话描述清晰
2. 源文件需对函数内所有代码逐行注释、尽可能一句话描述清晰
3. 文件顶部注释规范参考本文件顶部注释格式

项目框架
组件层 (Components)：负责具体功能的实现（机制）。
    提供纯粹的 API（如 scan(), connect(ssid, pwd), send_data()）。 
    不包含业务逻辑（例如：不决定什么时候重连，不决定什么时候保存数据，不决定是配网还是正常工作）。
    只负责“怎么做”（How）。
应用层 (App Managers)：负责业务逻辑处理（策略）。
    管理组件的生命周期（Init/Deinit）。
    响应事件（收到 DISCONNECT 事件后，决定是立即重连、还是报错、还是切回配网模式）。
    负责“做什么”和“什么时候做”（What & When）。
顶层状态机 (State Machine)：负责系统级的模式切换。
    协调各个 Manager（例如：从 Init 状态 -> 进入 Normal 模式，或者是进入 Provisioning 模式）。