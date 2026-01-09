# -*- coding: utf-8 -*-
# 知境中枢 - 后端服务 Dockerfile
#
# 构建镜像：docker build -t zhijing-backend .
# 运行容器：docker run -p 8000:8000 zhijing-backend

# 使用 Python 3.11 作为基础镜像
FROM python:3.11-slim

# 设置工作目录
WORKDIR /app

# 设置环境变量
# 禁止 Python 生成 .pyc 文件
ENV PYTHONDONTWRITEBYTECODE=1
# 禁止 Python 缓冲输出，便于查看日志
ENV PYTHONUNBUFFERED=1

# 安装系统依赖
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    && rm -rf /var/lib/apt/lists/*

# 复制依赖文件
COPY requirements.txt .

# 安装 Python 依赖
RUN pip install --no-cache-dir -r requirements.txt

# 复制应用代码
COPY . .

# 暴露端口
EXPOSE 8000

# 启动命令
# --host 0.0.0.0 允许外部访问
# --workers 4 启动 4 个工作进程（根据 CPU 核心数调整）
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"]
