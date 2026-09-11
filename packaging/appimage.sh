#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"
APPDIR="$PROJECT_ROOT/packaging/AppDir"
LINUXDEPLOY="$PROJECT_ROOT/packaging/linuxdeploy/linuxdeploy-x86_64.AppImage"
QT_PLUGIN="$PROJECT_ROOT/packaging/linuxdeploy/linuxdeploy-plugin-qt-x86_64.AppImage"
cd "$PROJECT_ROOT"

# Rebuild even locally so an existing executable cannot silently become stale.
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel

if [[ -z "${QMAKE:-}" ]]; then
    if [[ -n "${QT_ROOT_DIR:-}" ]]; then
        QMAKE="$QT_ROOT_DIR/bin/qmake"
    else
        QMAKE="$(command -v qmake6)"
    fi
fi
export QMAKE
qml_dir="$("$QMAKE" -query QT_INSTALL_QML)"
plugins_dir="$("$QMAKE" -query QT_INSTALL_PLUGINS)"
for module in org/kde/kirigami org/kde/desktop QtQuick/Controls/Fusion; do
    test -f "$qml_dir/$module/qmldir" || { echo "Missing QML module: $module" >&2; exit 1; }
done
test -x "$LINUXDEPLOY"
test -x "$QT_PLUGIN"

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/plugins/kf6"
cp "$BUILD_DIR/MMZScriptEditor" "$APPDIR/usr/bin/"

# KIO loads its local-file worker dynamically; ELF dependency scanning misses it.
test -f "$plugins_dir/kf6/kio/kio_file.so"
mkdir -p "$APPDIR/usr/plugins/kf6/kio"
cp "$plugins_dir/kf6/kio/kio_file.so" "$APPDIR/usr/plugins/kf6/kio/"
kioworker="$(find /usr/lib /usr/libexec -type f -name kioworker -print -quit 2>/dev/null || true)"
test -n "$kioworker" || { echo "Missing KIO worker launcher" >&2; exit 1; }
# KIO resolves this relative to libKIOCore, which linuxdeploy places in usr/lib.
mkdir -p "$APPDIR/usr/lib/libexec/kf6"
cp "$kioworker" "$APPDIR/usr/lib/libexec/kf6/"

# Scan only the application's sources, not an old AppDir or build tree.
qml_sources="$(mktemp -d)"
trap 'rm -rf "$qml_sources"' EXIT
cp "$PROJECT_ROOT/Main.qml" "$qml_sources/"
# These styles are selected in C++, so make their imports visible to the scanner.
cat > "$qml_sources/PackagingImports.qml" <<'QML'
import QtQuick
import QtQuick.Controls.Fusion
import org.kde.desktop
Item {}
QML
export QML_SOURCES_PATHS="$qml_sources"
export QML_MODULES_PATHS="$qml_dir"
export APPIMAGE_EXTRACT_AND_RUN=1
export NO_STRIP=1
# The action icons are SVG resources embedded in the executable.
export EXTRA_QT_MODULES=svg
export EXTRA_PLATFORM_PLUGINS=libqoffscreen.so
export OUTPUT="$PROJECT_ROOT/MMZScriptEditor.AppImage"

"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/MMZScriptEditor" \
    --executable "$APPDIR/usr/lib/libexec/kf6/kioworker" \
    --library "$APPDIR/usr/plugins/kf6/kio/kio_file.so" \
    --desktop-file "$PROJECT_ROOT/resources/desktop/MMZScriptEditor.desktop" \
    --icon-file "$PROJECT_ROOT/resources/icons/mmzscripteditor.png" \
    --plugin qt \
    --output appimage

test -s "$OUTPUT"
echo "Created $OUTPUT"
