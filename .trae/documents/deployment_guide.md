# 服务器监控应用部署指南

## 1. 项目概述
本文档描述了服务器监控应用的完整部署流程，包括前端打包、后端服务配置和systemd自启动设置。

## 2. 技术栈
- **前端**: React + TypeScript + Vite + TailwindCSS
- **后端**: C语言服务（端口8080）
- **数据库**: SQLite3
- **部署环境**: Linux systemd

## 3. 前端配置

### 3.1 端口配置
前端端口已设置为23334，配置文件：`vite.config.ts`
```typescript
server: {
  port: 23334,
  proxy: {
    '/api': {
      target: 'http://localhost:8080',
      changeOrigin: true,
    }
  }
}
```

### 3.2 中文界面
前端界面已完全中文化，包含：
- 页面标题：服务器监控
- 时间选择器：实时、1小时、1天、7天、30天
- 图表标题：CPU使用率、内存使用率、网络流量、磁盘I/O

## 4. 构建打包

### 4.1 安装依赖
```bash
npm install
```

### 4.2 构建生产版本
```bash
npm run build
```

### 4.3 预览构建结果
```bash
npm run preview
```

## 5. 部署脚本

### 5.1 创建部署目录
```bash
sudo mkdir -p /opt/server-monitor
sudo chown $USER:$USER /opt/server-monitor
```

### 5.2 复制文件
```bash
cp -r dist/* /opt/server-monitor/
cp package.json /opt/server-monitor/
```

### 5.3 创建后端服务目录
```bash
sudo mkdir -p /opt/server-monitor-backend
sudo chown $USER:$USER /opt/server-monitor-backend
```

## 6. Systemd服务配置

### 6.1 前端服务单元文件
创建 `/etc/systemd/system/server-monitor-frontend.service`：
```ini
[Unit]
Description=Server Monitor Frontend Service
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/server-monitor
ExecStart=/usr/bin/serve -s /opt/server-monitor -l 23334
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

### 6.2 后端服务单元文件
创建 `/etc/systemd/system/server-monitor-backend.service`：
```ini
[Unit]
Description=Server Monitor Backend Service
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/server-monitor-backend
ExecStart=/opt/server-monitor-backend/server-monitor-backend
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

### 6.3 服务管理命令
```bash
# 重新加载systemd配置
sudo systemctl daemon-reload

# 启用服务（开机自启）
sudo systemctl enable server-monitor-frontend.service
sudo systemctl enable server-monitor-backend.service

# 启动服务
sudo systemctl start server-monitor-frontend.service
sudo systemctl start server-monitor-backend.service

# 查看服务状态
sudo systemctl status server-monitor-frontend.service
sudo systemctl status server-monitor-backend.service

# 查看日志
sudo journalctl -u server-monitor-frontend.service -f
sudo journalctl -u server-monitor-backend.service -f
```

## 7. 一键部署脚本

创建 `deploy.sh`：
```bash
#!/bin/bash

set -e

echo "开始部署服务器监控应用..."

# 1. 构建前端
echo "构建前端应用..."
npm run build

# 2. 创建部署目录
echo "创建部署目录..."
sudo mkdir -p /opt/server-monitor
sudo mkdir -p /opt/server-monitor-backend

# 3. 复制前端文件
echo "部署前端文件..."
sudo cp -r dist/* /opt/server-monitor/
sudo cp package.json /opt/server-monitor/

# 4. 安装serve工具（如果未安装）
if ! command -v serve &> /dev/null; then
    echo "安装serve工具..."
    sudo npm install -g serve
fi

# 5. 复制后端文件（假设已编译）
echo "部署后端文件..."
# sudo cp backend/server-monitor-backend /opt/server-monitor-backend/
# sudo cp backend/database.db /opt/server-monitor-backend/

# 6. 设置权限
echo "设置文件权限..."
sudo chown -R www-data:www-data /opt/server-monitor
sudo chown -R www-data:www-data /opt/server-monitor-backend
sudo chmod +x /opt/server-monitor-backend/server-monitor-backend

# 7. 安装systemd服务
echo "安装systemd服务..."
sudo cp systemd/server-monitor-frontend.service /etc/systemd/system/
sudo cp systemd/server-monitor-backend.service /etc/systemd/system/

# 8. 重新加载systemd
sudo systemctl daemon-reload

# 9. 启用服务
sudo systemctl enable server-monitor-frontend.service
sudo systemctl enable server-monitor-backend.service

# 10. 启动服务
echo "启动服务..."
sudo systemctl start server-monitor-frontend.service
sudo systemctl start server-monitor-backend.service

# 11. 检查状态
echo "检查服务状态..."
sleep 3
sudo systemctl status server-monitor-frontend.service --no-pager
sudo systemctl status server-monitor-backend.service --no-pager

echo "部署完成！"
echo "前端地址: http://localhost:23334"
echo "后端API: http://localhost:8080"
echo ""
echo "查看日志命令:"
echo "前端: sudo journalctl -u server-monitor-frontend.service -f"
echo "后端: sudo journalctl -u server-monitor-backend.service -f"
```

## 8. 监控和维护

### 8.1 服务状态检查
```bash
# 检查服务是否运行
sudo systemctl is-active server-monitor-frontend.service
sudo systemctl is-active server-monitor-backend.service

# 检查服务是否启用
sudo systemctl is-enabled server-monitor-frontend.service
sudo systemctl is-enabled server-monitor-backend.service
```

### 8.2 日志查看
```bash
# 实时查看日志
sudo journalctl -u server-monitor-frontend.service -f
sudo journalctl -u server-monitor-backend.service -f

# 查看最近100条日志
sudo journalctl -u server-monitor-frontend.service -n 100
sudo journalctl -u server-monitor-backend.service -n 100
```

### 8.3 服务重启
```bash
# 重启服务
sudo systemctl restart server-monitor-frontend.service
sudo systemctl restart server-monitor-backend.service

# 停止服务
sudo systemctl stop server-monitor-frontend.service
sudo systemctl stop server-monitor-backend.service
```

## 9. 防火墙配置

如果需要外部访问，配置防火墙：
```bash
# 开放23334端口（前端）
sudo ufw allow 23334/tcp

# 开放8080端口（后端API，可选）
sudo ufw allow 8080/tcp

# 重新加载防火墙
sudo ufw reload
```

## 10. 故障排除

### 10.1 端口冲突
如果23334端口被占用：
```bash
# 检查端口使用情况
sudo netstat -tlnp | grep 23334

# 修改前端端口（编辑vite.config.ts后重新构建）
```

### 10.2 权限问题
```bash
# 修复文件权限
sudo chown -R www-data:www-data /opt/server-monitor
sudo chown -R www-data:www-data /opt/server-monitor-backend
```

### 10.3 服务启动失败
```bash
# 查看详细错误信息
sudo journalctl -u server-monitor-frontend.service --no-pager -l
sudo journalctl -u server-monitor-backend.service --no-pager -l
```