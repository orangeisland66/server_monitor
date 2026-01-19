# Systemd服务配置文件

## 前端服务配置

### server-monitor-frontend.service
```ini
[Unit]
Description=Server Monitor Frontend Service
Documentation=https://github.com/your-repo/server-monitor
After=network.target
Wants=network.target

[Service]
Type=simple
User=www-data
Group=www-data
WorkingDirectory=/opt/server-monitor
ExecStart=/usr/bin/serve -s /opt/server-monitor -l 23334 --cors
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=server-monitor-frontend

# 安全设置
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/server-monitor

# 资源限制
CPUQuota=50%
MemoryLimit=512M
TasksMax=50

# 环境变量
Environment=NODE_ENV=production
Environment=PORT=23334

[Install]
WantedBy=multi-user.target
Alias=server-monitor-frontend.service
```

## 后端服务配置

### server-monitor-backend.service
```ini
[Unit]
Description=Server Monitor Backend Service
Documentation=https://github.com/your-repo/server-monitor
After=network.target
Wants=network.target
Before=server-monitor-frontend.service

[Service]
Type=simple
User=www-data
Group=www-data
WorkingDirectory=/opt/server-monitor-backend
ExecStart=/opt/server-monitor-backend/server-monitor-backend
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=server-monitor-backend

# 安全设置
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/server-monitor-backend

# 资源限制
CPUQuota=30%
MemoryLimit=256M
TasksMax=30

# 环境变量
Environment=DATABASE_PATH=/opt/server-monitor-backend/monitor.db
Environment=LOG_LEVEL=info
Environment=PORT=8080

[Install]
WantedBy=multi-user.target
Alias=server-monitor-backend.service
```

## 服务管理脚本

### monitor-services.sh
```bash
#!/bin/bash
# 服务器监控服务管理脚本

SERVICE_FRONTEND="server-monitor-frontend.service"
SERVICE_BACKEND="server-monitor-backend.service"

case "$1" in
    start)
        echo "启动服务器监控服务..."
        sudo systemctl start $SERVICE_BACKEND
        sleep 2
        sudo systemctl start $SERVICE_FRONTEND
        echo "服务已启动"
        ;;
    stop)
        echo "停止服务器监控服务..."
        sudo systemctl stop $SERVICE_FRONTEND
        sudo systemctl stop $SERVICE_BACKEND
        echo "服务已停止"
        ;;
    restart)
        echo "重启服务器监控服务..."
        sudo systemctl restart $SERVICE_BACKEND
        sleep 2
        sudo systemctl restart $SERVICE_FRONTEND
        echo "服务已重启"
        ;;
    status)
        echo "前端服务状态:"
        sudo systemctl status $SERVICE_FRONTEND --no-pager
        echo ""
        echo "后端服务状态:"
        sudo systemctl status $SERVICE_BACKEND --no-pager
        ;;
    logs)
        echo "查看前端服务日志:"
        sudo journalctl -u $SERVICE_FRONTEND -f
        ;;
    logs-backend)
        echo "查看后端服务日志:"
        sudo journalctl -u $SERVICE_BACKEND -f
        ;;
    enable)
        echo "启用开机自启..."
        sudo systemctl enable $SERVICE_FRONTEND
        sudo systemctl enable $SERVICE_BACKEND
        echo "开机自启已启用"
        ;;
    disable)
        echo "禁用开机自启..."
        sudo systemctl disable $SERVICE_FRONTEND
        sudo systemctl disable $SERVICE_BACKEND
        echo "开机自启已禁用"
        ;;
    *)
        echo "用法: $0 {start|stop|restart|status|logs|logs-backend|enable|disable}"
        exit 1
        ;;
esac
```

## 健康检查配置

### health-check.sh
```bash
#!/bin/bash
# 健康检查脚本

FRONTEND_URL="http://localhost:23334"
BACKEND_URL="http://localhost:8080/api/status"

# 检查前端服务
frontend_status=$(curl -s -o /dev/null -w "%{http_code}" $FRONTEND_URL)
if [ "$frontend_status" = "200" ]; then
    echo "✓ 前端服务运行正常"
else
    echo "✗ 前端服务异常 (HTTP $frontend_status)"
fi

# 检查后端服务
backend_status=$(curl -s -o /dev/null -w "%{http_code}" $BACKEND_URL)
if [ "$backend_status" = "200" ]; then
    echo "✓ 后端服务运行正常"
else
    echo "✗ 后端服务异常 (HTTP $backend_status)"
fi

# 检查端口监听
if netstat -tlnp | grep -q ':23334'; then
    echo "✓ 前端端口23334监听正常"
else
    echo "✗ 前端端口23334未监听"
fi

if netstat -tlnp | grep -q ':8080'; then
    echo "✓ 后端端口8080监听正常"
else
    echo "✗ 后端端口8080未监听"
fi
```

## 日志轮转配置

### /etc/logrotate.d/server-monitor
```
/var/log/server-monitor/*.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    create 644 www-data www-data
    postrotate
        systemctl reload server-monitor-frontend.service
        systemctl reload server-monitor-backend.service
    endscript
}
```

## 安装和配置步骤

```bash
#!/bin/bash
# 一键安装systemd服务

# 1. 创建服务文件目录
sudo mkdir -p /etc/systemd/system

# 2. 复制服务文件
cat > /tmp/server-monitor-frontend.service << 'EOF'
[Unit]
Description=Server Monitor Frontend Service
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/server-monitor
ExecStart=/usr/bin/serve -s /opt/server-monitor -l 23334 --cors
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

cat > /tmp/server-monitor-backend.service << 'EOF'
[Unit]
Description=Server Monitor Backend Service
After=network.target
Before=server-monitor-frontend.service

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/server-monitor-backend
ExecStart=/opt/server-monitor-backend/server-monitor-backend
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# 3. 移动服务文件
sudo mv /tmp/server-monitor-frontend.service /etc/systemd/system/
sudo mv /tmp/server-monitor-backend.service /etc/systemd/system/

# 4. 创建www-data用户（如果不存在）
if ! id "www-data" &>/dev/null; then
    sudo useradd -r -s /bin/false www-data
fi

# 5. 重新加载systemd
sudo systemctl daemon-reload

# 6. 启用服务
sudo systemctl enable server-monitor-frontend.service
sudo systemctl enable server-monitor-backend.service

echo "Systemd服务安装完成！"
echo "使用以下命令管理服务："
echo "sudo systemctl start server-monitor-frontend.service"
echo "sudo systemctl start server-monitor-backend.service"
echo "sudo systemctl status server-monitor-frontend.service"
echo "sudo systemctl status server-monitor-backend.service"
```