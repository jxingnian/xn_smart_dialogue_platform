# -*- coding: utf-8 -*-
"""
依赖注入模块

功能说明：
    提供 FastAPI 的依赖注入函数。
    依赖注入是一种设计模式，让代码更容易测试和维护。
    
    比如获取数据库连接，不是在每个函数里自己创建连接，
    而是通过依赖注入的方式，由框架统一管理连接的创建和释放。

使用方式：
    在 API 路由函数的参数中声明依赖：
    async def get_devices(db: AsyncSession = Depends(get_db)):
        ...
"""

from typing import AsyncGenerator
from sqlalchemy.ext.asyncio import AsyncSession

# 数据库会话工厂，在 database.py 中初始化后导入
# 这里先声明为 None，启动时会被赋值
async_session_factory = None


async def get_db() -> AsyncGenerator[AsyncSession, None]:
    """
    获取数据库会话的依赖函数
    
    功能说明：
        创建一个数据库会话，在请求处理完成后自动关闭。
        使用 async with 确保会话正确释放，避免连接泄漏。
    
    返回：
        AsyncSession: 异步数据库会话对象
    
    使用示例：
        @router.get("/devices")
        async def list_devices(db: AsyncSession = Depends(get_db)):
            # 使用 db 进行数据库操作
            result = await db.execute(select(Device))
            return result.scalars().all()
    """
    if async_session_factory is None:
        raise RuntimeError("数据库未初始化，请先调用 init_db()")
    
    async with async_session_factory() as session:
        try:
            yield session
        finally:
            await session.close()
