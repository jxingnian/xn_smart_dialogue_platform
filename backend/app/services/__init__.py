'''
Author: 星年 && j_xingnian@163.com
Date: 2026-01-09 09:58:35
LastEditors: xingnian j_xingnian@163.com
LastEditTime: 2026-01-09 14:47:42
Description: 

Copyright (c) 2026 by ${git_name_email}, All Rights Reserved. 
'''
# -*- coding: utf-8 -*-
"""
业务服务模块

功能说明：
    包含所有业务逻辑的实现。
    服务层是业务逻辑的核心，负责：
        - 处理业务规则
        - 协调多个 Repository 的数据操作
        - 调用外部服务（AI、MQTT 等）
    
    服务层不直接处理 HTTP 请求，由 API 层调用。
"""
