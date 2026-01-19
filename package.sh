#!/bin/bash

# Exit on error
set -e

APP_NAME="server_monitor"
VERSION="1.0.0"
PACKAGE_NAME="${APP_NAME}_${VERSION}"
OUTPUT_DIR="package"

echo "📦 Packaging $APP_NAME..."

# Create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR/$PACKAGE_NAME"

# 1. Build Backend
echo "🔨 Building Backend..."
cd backend
make clean
make all
cd ..

# 2. Build Frontend
echo "🎨 Building Frontend..."
npm install
npm run build

# 3. Copy files to package directory
echo "📂 Copying files..."

# Backend files
mkdir -p "$OUTPUT_DIR/$PACKAGE_NAME/backend/bin"
mkdir -p "$OUTPUT_DIR/$PACKAGE_NAME/backend/lib"
cp backend/bin/server_monitor "$OUTPUT_DIR/$PACKAGE_NAME/backend/bin/"
# If there are dynamic libs or db needed, copy them. 
# The db will be created by the app, but we can copy the structure if needed.

# Frontend files
cp -r dist "$OUTPUT_DIR/$PACKAGE_NAME/"
cp server.js "$OUTPUT_DIR/$PACKAGE_NAME/"
cp package.json "$OUTPUT_DIR/$PACKAGE_NAME/"
cp package-lock.json "$OUTPUT_DIR/$PACKAGE_NAME/"

# Deployment script
cp deploy.sh "$OUTPUT_DIR/$PACKAGE_NAME/"

# 4. Create Tarball
echo "🗜️ Creating tarball..."
cd "$OUTPUT_DIR"
tar -czf "${PACKAGE_NAME}.tar.gz" "$PACKAGE_NAME"
echo "✅ Package created: $OUTPUT_DIR/${PACKAGE_NAME}.tar.gz"

echo "To install on target machine:"
echo "1. Copy ${PACKAGE_NAME}.tar.gz to the target machine."
echo "2. Run: tar -xzf ${PACKAGE_NAME}.tar.gz"
echo "3. Run: cd ${PACKAGE_NAME} && ./deploy.sh"
