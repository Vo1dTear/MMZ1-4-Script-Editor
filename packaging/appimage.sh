#!/usr/bin/env bash

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

BUILD_DIR="$PROJECT_ROOT/build"
APPDIR="$PROJECT_ROOT/packaging/AppDir"

DESKTOP_FILE="$PROJECT_ROOT/resources/desktop/MMZScriptEditor.desktop"
ICON_FILE="$PROJECT_ROOT/resources/icons/mmzscripteditor.png"

EXECUTABLE="MMZScriptEditor"

LINUXDEPLOY="$PROJECT_ROOT/packaging/linuxdeploy/linuxdeploy-x86_64.AppImage"
QT_PLUGIN="$PROJECT_ROOT/packaging/linuxdeploy/linuxdeploy-plugin-qt-x86_64.AppImage"

echo "== Cleaning AppDir =="
rm -rf "$APPDIR"

#
# Only build locally.
# GitHub Actions already built the project.
#
if [ ! -f "$BUILD_DIR/$EXECUTABLE" ]; then
    echo "== Building project =="
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" --parallel
fi

echo "== Creating AppDir =="
mkdir -p "$APPDIR/usr/bin"

echo "== Copying executable =="
cp "$BUILD_DIR/$EXECUTABLE" "$APPDIR/usr/bin/"

echo "== Generating AppImage =="

#
# Use the Qt installed by GitHub Actions if available.
#
if [ -n "$QT_ROOT_DIR" ]; then
    export QMAKE="$QT_ROOT_DIR/bin/qmake"
else
    export QMAKE="$(command -v qmake6)"
fi

export NO_STRIP=1
export LINUXDEPLOY_PLUGIN_QT="$QT_PLUGIN"

"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --desktop-file "$DESKTOP_FILE" \
    --icon-file "$ICON_FILE" \
    --plugin qt \
    --output appimage

# Rename AppImage
APPIMAGE=$(find "$PROJECT_ROOT" -maxdepth 1 -name "*.AppImage" | head -n1)

if [ -n "$APPIMAGE" ]; then
    mv "$APPIMAGE" "$PROJECT_ROOT/MMZScriptEditor.AppImage"
fi
    
echo
echo "Done!"
