'''
Author: 星年 && j_xingnian@163.com
Date: 2026-01-09 09:58:46
LastEditors: xingnian j_xingnian@163.com
LastEditTime: 2026-01-09 10:07:44
FilePath: \xn_smart_dialogue_platform\backend\app\services\ai\__init__.py
Description: 

Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
'''
# -*- coding: utf-8 -*-
"""
AI 服务模块

功能说明：
    封装所有 AI 能力的调用，包括：
        - LLM 对话
        - 语音识别 (ASR)
        - 语音合成 (TTS)
        - 文本向量化 (Embedding)
        - 图像理解
        - 人脸识别
    
    每种 AI 能力都定义了抽象接口，支持本地部署和云端 API 两种实现。
    通过配置文件选择使用哪种实现。
"""
