# -*- coding: utf-8 -*-
"""
应用入口

功能说明：
    FastAPI 应用的入口文件。
    负责：
        - 创建 FastAPI 应用实例
        - 注册路由
        - 配置中间件
        - 配置异常处理
        - 启动/关闭事件处理
"""

from contextlib import asynccontextmanager
from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware

from app.core.config import settings
from app.core.exceptions import BusinessError
from app.api.v1 import router as api_v1_router


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    应用生命周期管理
    
    在应用启动时执行初始化操作，
    在应用关闭时执行清理操作。
    """
    # ========== 启动时执行 ==========
    print(f"🚀 {settings.APP_NAME} 正在启动...")
    
    # TODO: 初始化数据库连接池
    # TODO: 初始化 Redis 连接
    # TODO: 初始化 MQTT 客户端
    # TODO: 加载 AI 模型（如果是本地部署）
    
    print(f"✅ {settings.APP_NAME} 启动完成")
    
    yield  # 应用运行中
    
    # ========== 关闭时执行 ==========
    print(f"🛑 {settings.APP_NAME} 正在关闭...")
    
    # TODO: 关闭数据库连接池
    # TODO: 关闭 Redis 连接
    # TODO: 断开 MQTT 连接
    
    print(f"👋 {settings.APP_NAME} 已关闭")


# 创建 FastAPI 应用实例
app = FastAPI(
    title=settings.APP_NAME,
    description="智能家居对话平台 - 后端服务",
    version="1.0.0",
    lifespan=lifespan,
    docs_url="/docs" if settings.DEBUG else None,  # 生产环境关闭文档
    redoc_url="/redoc" if settings.DEBUG else None,
)


# ========== 配置中间件 ==========

# CORS 跨域配置
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"] if settings.DEBUG else [],  # 生产环境需要配置具体域名
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ========== 配置异常处理 ==========

@app.exception_handler(BusinessError)
async def business_error_handler(request: Request, exc: BusinessError):
    """
    业务异常统一处理
    
    将业务异常转换为统一的 JSON 响应格式。
    """
    return JSONResponse(
        status_code=400,
        content={
            "code": exc.code,
            "message": exc.message,
            "data": None,
        },
    )


@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    """
    全局异常处理
    
    捕获所有未处理的异常，返回统一的错误响应。
    生产环境不暴露具体错误信息。
    """
    # 记录错误日志
    print(f"❌ 未处理的异常: {exc}")
    
    return JSONResponse(
        status_code=500,
        content={
            "code": "INTERNAL_ERROR",
            "message": str(exc) if settings.DEBUG else "服务器内部错误",
            "data": None,
        },
    )


# ========== 注册路由 ==========

# 注册 API v1 版本路由
app.include_router(api_v1_router, prefix=settings.API_V1_PREFIX)


# ========== 健康检查 ==========

@app.get("/health")
async def health_check():
    """
    健康检查接口
    
    用于负载均衡器或监控系统检查服务是否正常运行。
    """
    return {"status": "healthy", "app": settings.APP_NAME}
