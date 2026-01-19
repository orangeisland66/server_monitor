#!/bin/bash

# Exit on error
set -e

PROJECT_ROOT=$(pwd)
BACKEND_DIR="$PROJECT_ROOT/backend"
FRONTEND_DIR="$PROJECT_ROOT"

echo "🚀 Starting deployment process..."

# 1. Build Backend
echo "🔨 Building Backend..."
cd "$BACKEND_DIR"
make clean
make all
if [ $? -eq 0 ]; then
    echo "✅ Backend built successfully."
else
    echo "❌ Backend build failed."
    exit 1
fi

# 2. Build Frontend
echo "🎨 Building Frontend..."
cd "$FRONTEND_DIR"
npm install
npm run build
if [ $? -eq 0 ]; then
    echo "✅ Frontend built successfully."
else
    echo "❌ Frontend build failed."
    exit 1
fi

# 3. Generate Systemd Service Files

echo "⚙️ Generating Systemd Service Files..."

# Backend Service
cat > server-monitor-backend.service <<EOL
[Unit]
Description=Server Monitor Backend Service
After=network.target

[Service]
Type=simple
User=$(whoami)
WorkingDirectory=$BACKEND_DIR
ExecStart=$BACKEND_DIR/bin/server_monitor
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOL

# Frontend Service
# Using node to run the custom server.js
NODE_PATH=$(which node)

cat > server-monitor-frontend.service <<EOL
[Unit]
Description=Server Monitor Frontend Service
After=network.target

[Service]
Type=simple
User=$(whoami)
WorkingDirectory=$FRONTEND_DIR
ExecStart=$NODE_PATH server.js
Restart=always
RestartSec=3
Environment=PORT=23334

[Install]
WantedBy=multi-user.target
EOL

echo "✅ Service files created."

# 4. Install Services
echo "📦 Installing Services (requires sudo)..."

# Stop existing services if running
sudo systemctl stop server-monitor-backend.service || true
sudo systemctl stop server-monitor-frontend.service || true

# Copy service files
sudo cp server-monitor-backend.service /etc/systemd/system/
sudo cp server-monitor-frontend.service /etc/systemd/system/

# Reload daemon
sudo systemctl daemon-reload

# Enable and Start
sudo systemctl enable server-monitor-backend.service
sudo systemctl enable server-monitor-frontend.service
sudo systemctl start server-monitor-backend.service
sudo systemctl start server-monitor-frontend.service

echo "✅ Services installed and started!"
echo "   - Backend Status: $(systemctl is-active server-monitor-backend.service)"
echo "   - Frontend Status: $(systemctl is-active server-monitor-frontend.service)"
echo ""
echo "🎉 Deployment Complete!"
echo "👉 Frontend accessible at: http://localhost:23334"
