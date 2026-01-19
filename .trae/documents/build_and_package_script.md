# 构建打包脚本

## 一键打包脚本

### build-and-package.sh
```bash
#!/bin/bash
# 服务器监控应用打包脚本

set -e

echo "🚀 开始构建服务器监控应用..."

# 配置变量
APP_NAME="server-monitor"
VERSION=$(date +%Y%m%d-%H%M%S)
BUILD_DIR="build"
PACKAGE_DIR="package"
FRONTEND_DIR="dist"
BACKEND_DIR="backend"

# 清理旧构建
echo "🧹 清理旧构建文件..."
rm -rf $BUILD_DIR $PACKAGE_DIR
mkdir -p $BUILD_DIR/{frontend,backend,scripts,config}

# 构建前端
echo "📦 构建前端应用..."
npm run build

# 检查构建结果
if [ ! -d "$FRONTEND_DIR" ]; then
    echo "❌ 前端构建失败，dist目录不存在"
    exit 1
fi

# 复制前端文件
echo "📁 复制前端文件..."
cp -r $FRONTEND_DIR/* $BUILD_DIR/frontend/
cp package.json $BUILD_DIR/frontend/
cp package-lock.json $BUILD_DIR/frontend/

# 复制后端文件（假设已编译）
echo "📁 复制后端文件..."
if [ -f "$BACKEND_DIR/server-monitor-backend" ]; then
    cp $BACKEND_DIR/server-monitor-backend $BUILD_DIR/backend/
else
    echo "⚠️  后端可执行文件不存在，跳过"
fi

if [ -f "$BACKEND_DIR/monitor.db" ]; then
    cp $BACKEND_DIR/monitor.db $BUILD_DIR/backend/
fi

# 复制脚本文件
echo "📁 复制部署脚本..."
cat > $BUILD_DIR/scripts/install.sh << 'EOF'
#!/bin/bash
# 安装脚本

set -e

INSTALL_DIR="/opt/server-monitor"
USER="www-data"

echo "🔧 开始安装服务器监控应用..."

# 检查权限
if [ "$EUID" -ne 0 ]; then 
    echo "❌ 请以root权限运行此脚本"
    exit 1
fi

# 创建用户
if ! id "$USER" &>/dev/null; then
    useradd -r -s /bin/false $USER
fi

# 创建安装目录
mkdir -p $INSTALL_DIR/{frontend,backend}
mkdir -p /var/log/server-monitor

# 复制文件
echo "📁 复制文件..."
cp -r frontend/* $INSTALL_DIR/frontend/
cp -r backend/* $INSTALL_DIR/backend/

# 设置权限
echo "🔒 设置文件权限..."
chown -R $USER:$USER $INSTALL_DIR
chown -R $USER:$USER /var/log/server-monitor
chmod +x $INSTALL_DIR/backend/server-monitor-backend

# 安装Node.js依赖
echo "📦 安装Node.js依赖..."
cd $INSTALL_DIR/frontend
npm install --production

# 安装systemd服务
echo "🔧 安装systemd服务..."
cat > /etc/systemd/system/server-monitor-frontend.service << 'SERVICE_EOF'
[Unit]
Description=Server Monitor Frontend Service
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/server-monitor/frontend
ExecStart=/usr/bin/serve -s /opt/server-monitor/frontend -l 23334 --cors
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
SERVICE_EOF

cat > /etc/systemd/system/server-monitor-backend.service << 'SERVICE_EOF'
[Unit]
Description=Server Monitor Backend Service
After=network.target
Before=server-monitor-frontend.service

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/server-monitor/backend
ExecStart=/opt/server-monitor/backend/server-monitor-backend
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
SERVICE_EOF

# 重新加载systemd
systemctl daemon-reload

# 启用服务
systemctl enable server-monitor-frontend.service
systemctl enable server-monitor-backend.service

echo "✅ 安装完成！"
echo ""
echo "使用以下命令管理服务："
echo "  启动:   systemctl start server-monitor-frontend.service"
echo "  停止:   systemctl stop server-monitor-frontend.service"
echo "  状态:   systemctl status server-monitor-frontend.service"
echo "  日志:   journalctl -u server-monitor-frontend.service -f"
echo ""
echo "应用地址: http://localhost:23334"
EOF

# 复制配置文件
echo "📁 创建配置文件..."
cat > $BUILD_DIR/config/config.json << 'EOF'
{
  "frontend": {
    "port": 23334,
    "host": "0.0.0.0",
    "cors": true
  },
  "backend": {
    "port": 8080,
    "host": "127.0.0.1",
    "database": "monitor.db"
  },
  "logging": {
    "level": "info",
    "file": "/var/log/server-monitor/app.log"
  }
}
EOF

# 创建卸载脚本
cat > $BUILD_DIR/scripts/uninstall.sh << 'EOF'
#!/bin/bash
# 卸载脚本

set -e

echo "🗑️  开始卸载服务器监控应用..."

# 检查权限
if [ "$EUID" -ne 0 ]; then 
    echo "❌ 请以root权限运行此脚本"
    exit 1
fi

# 停止服务
echo "⏹️  停止服务..."
systemctl stop server-monitor-frontend.service || true
systemctl stop server-monitor-backend.service || true

# 禁用服务
echo "🔧 禁用服务..."
systemctl disable server-monitor-frontend.service || true
systemctl disable server-monitor-backend.service || true

# 删除服务文件
rm -f /etc/systemd/system/server-monitor-frontend.service
rm -f /etc/systemd/system/server-monitor-backend.service

# 重新加载systemd
systemctl daemon-reload

# 删除应用文件
echo "🗑️  删除应用文件..."
rm -rf /opt/server-monitor
rm -rf /var/log/server-monitor

echo "✅ 卸载完成！"
EOF

# 创建更新脚本
cat > $BUILD_DIR/scripts/update.sh << 'EOF'
#!/bin/bash
# 更新脚本

set -e

BACKUP_DIR="/opt/server-monitor-backup-$(date +%Y%m%d-%H%M%S)"

echo "🔄 开始更新服务器监控应用..."

# 检查权限
if [ "$EUID" -ne 0 ]; then 
    echo "❌ 请以root权限运行此脚本"
    exit 1
fi

# 备份当前版本
echo "💾 备份当前版本..."
cp -r /opt/server-monitor $BACKUP_DIR

# 停止服务
echo "⏹️  停止服务..."
systemctl stop server-monitor-frontend.service
systemctl stop server-monitor-backend.service

# 更新文件
echo "📁 更新文件..."
cp -r frontend/* /opt/server-monitor/frontend/
cp -r backend/* /opt/server-monitor/backend/

# 设置权限
chown -R www-data:www-data /opt/server-monitor

# 启动服务
echo "▶️  启动服务..."
systemctl start server-monitor-backend.service
sleep 2
systemctl start server-monitor-frontend.service

echo "✅ 更新完成！"
echo "备份文件位于: $BACKUP_DIR"
EOF

# 设置脚本权限
chmod +x $BUILD_DIR/scripts/*.sh

# 创建打包归档
echo "📦 创建发布包..."
cd $(dirname $0)
tar -czf "server-monitor-${VERSION}.tar.gz" -C $BUILD_DIR .
zip -r "server-monitor-${VERSION}.zip" $BUILD_DIR/*

echo "✅ 构建打包完成！"
echo ""
echo "📁 输出文件:"
echo "  - server-monitor-${VERSION}.tar.gz"
echo "  - server-monitor-${VERSION}.zip"
echo ""
echo "📦 包内容:"
echo "  - frontend/    : 前端文件"
echo "  - backend/     : 后端文件"
echo "  - scripts/     : 部署脚本"
echo "  - config/      : 配置文件"
echo ""
echo "🔧 安装命令:"
echo "  tar -xzf server-monitor-${VERSION}.tar.gz"
echo "  sudo ./scripts/install.sh"
```

## 2. 依赖安装脚本

### install-deps.sh
```bash
#!/bin/bash
# 安装依赖脚本

set -e

echo "📦 安装系统依赖..."

# 更新包列表
sudo apt update

# 安装Node.js
echo "📦 安装Node.js..."
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# 安装构建工具
echo "🔧 安装构建工具..."
sudo apt install -y build-essential gcc make

# 安装其他依赖
echo "📦 安装其他依赖..."
sudo apt install -y sqlite3 libsqlite3-dev
sudo apt install -y curl wget net-tools

# 安装全局Node.js工具
echo "📦 安装全局Node.js工具..."
sudo npm install -g serve pm2

# 验证安装
echo "✅ 验证安装..."
node --version
npm --version
sqlite3 --version

echo "✅ 依赖安装完成！"
```

## 3. 快速部署命令

```bash
# 1. 克隆代码
git clone <your-repo>
cd server-monitor

# 2. 安装依赖
./scripts/install-deps.sh

# 3. 构建打包
./build-and-package.sh

# 4. 安装部署
tar -xzf server-monitor-*.tar.gz
sudo ./scripts/install.sh

# 5. 检查状态
./scripts/monitor-services.sh status
```

## 4. 目录结构

```
server-monitor/
├── build/                    # 构建输出目录
│   ├── frontend/            # 前端文件
│   ├── backend/             # 后端文件
│   ├── scripts/             # 部署脚本
│   └── config/              # 配置文件
├── package/                 # 打包文件
├── scripts/                 # 构建脚本
│   ├── build-and-package.sh
│   ├── install-deps.sh
│   └── monitor-services.sh
├── dist/                    # 前端构建输出
├── src/                     # 源代码
├── package.json
└── README.md
```

## 5. 版本管理

### 版本命名规则
- 格式: `YYYYMMDD-HHMMSS`
- 示例: `20240119-143022`
- 说明: 基于构建时间的版本号

### 版本信息文件
```bash
cat > $BUILD_DIR/VERSION << EOF
VERSION=${VERSION}
BUILD_DATE=$(date)
GIT_COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
NODE_VERSION=$(node --version)
NPM_VERSION=$(npm --version)
EOF
```

## 6. 回滚机制

### 回滚脚本
```bash
#!/bin/bash
# 回滚到上一个版本

BACKUP_DIR="/opt/server-monitor-backup"
CURRENT_DIR="/opt/server-monitor"

# 找到最新的备份
LATEST_BACKUP=$(ls -t $BACKUP_DIR-* | head -1)

if [ -z "$LATEST_BACKUP" ]; then
    echo "❌ 没有找到备份文件"
    exit 1
fi

echo "🔄 回滚到: $LATEST_BACKUP"

# 停止当前服务
systemctl stop server-monitor-frontend.service
systemctl stop server-monitor-backend.service

# 恢复备份
cp -r $LATEST_BACKUP/* $CURRENT_DIR/

# 启动服务
systemctl start server-monitor-backend.service
systemctl start server-monitor-frontend.service

echo "✅ 回滚完成！"
```