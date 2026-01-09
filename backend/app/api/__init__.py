# -*- coding: utf-8 -*-
"""
API 路由模块

功能说明：
    定义所有 HTTP API 接口。
    API 层只负责：
        - 接收 HTTP 请求
        - 参数校验
        - 调用 Service 层处理业务
        - 返回 HTTP 响应
    
    API 层不包含业务逻辑。
"""
