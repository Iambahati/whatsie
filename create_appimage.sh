#!/bin/bash

# Configuration
VERSION="4.16.3"
APP_NAME="whatsie"
BINARY="build/whatsie"
ICON="src/icons/app/icon-512.png"
DESKTOP="dist/linux/com.ktechpit.whatsie.desktop"

# Tools
LINUXDEPLOY="$HOME/Downloads/linuxdeploy-x86_64.AppImage"
QT_PLUGIN="$HOME/Downloads/linuxdeploy-plugin-qt-x86_64.AppImage"

echo "Creating AppImage for $APP_NAME version $VERSION..."

# Ensure tools are executable
chmod +x "$LINUXDEPLOY"
chmod +x "$QT_PLUGIN"

# Prepare environment
export VERSION=$VERSION
export PATH="$HOME/Downloads:$PATH"
export APPIMAGELAUNCHER_DISABLE=1

# Run linuxdeploy
"$LINUXDEPLOY" --appimage-extract-and-run --appdir AppDir \
    -e "$BINARY" \
    -i "$ICON" \
    -d "$DESKTOP" \
    --plugin qt \
    --output appimage

echo "AppImage creation finished!"
