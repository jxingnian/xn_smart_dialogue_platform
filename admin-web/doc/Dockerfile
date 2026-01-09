# 知境中枢 - 管理端 Dockerfile
#
# 多阶段构建：
#   1. 构建阶段：编译 TypeScript，打包静态资源
#   2. 运行阶段：使用 Nginx 提供静态文件服务

# ========== 构建阶段 ==========
FROM node:20-alpine AS builder

WORKDIR /app

# 复制依赖文件
COPY package.json pnpm-lock.yaml* package-lock.json* yarn.lock* ./

# 安装依赖
RUN npm install

# 复制源代码
COPY . .

# 构建生产版本
RUN npm run build

# ========== 运行阶段 ==========
FROM nginx:alpine

# 复制构建产物到 Nginx 目录
COPY --from=builder /app/dist /usr/share/nginx/html

# 复制 Nginx 配置
COPY nginx.conf /etc/nginx/conf.d/default.conf

# 暴露端口
EXPOSE 80

# 启动 Nginx
CMD ["nginx", "-g", "daemon off;"]
