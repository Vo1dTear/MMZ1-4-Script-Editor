#!/usr/bin/env bash
# Run in an MSYS2 UCRT64 shell after building the application.
set -euo pipefail

build_dir="${BUILD_DIR:-build}"
package_dir="$build_dir/windows-package"
qt_qml_dir="$(qtpaths6 --query QT_INSTALL_QML)"
qt_qml_dir="$(cygpath -u "$qt_qml_dir")"
mkdir -p "$package_dir"
cp "$build_dir/MMZScriptEditor.exe" "$package_dir/"

# Include Fusion explicitly: main.cpp selects this style at runtime.
mkdir -p "$build_dir/windows-qml-imports"
cp Main.qml "$build_dir/windows-qml-imports/"
cat > "$build_dir/windows-qml-imports/Styles.qml" <<'QML'
import QtQuick.Controls.Fusion
QML
windeployqt6 --release --qmldir "$build_dir/windows-qml-imports" \
    --qmlimport "$qt_qml_dir" "$package_dir/MMZScriptEditor.exe"

# Resolve third-party DLLs too (Kirigami and MinGW dependencies are not
# necessarily collected by windeployqt). Scan every deployed plugin recursively.
mapfile -d '' binaries < <(find "$package_dir" -type f \( -iname '*.dll' -o -iname '*.exe' \) -print0)
for ((i = 0; i < ${#binaries[@]}; i++)); do
    while IFS= read -r dependency; do
        dependency="${dependency%$'\r'}"
        if [[ -f "/ucrt64/bin/$dependency" && ! -f "$package_dir/$dependency" ]]; then
            cp "/ucrt64/bin/$dependency" "$package_dir/"
            binaries+=("$package_dir/$dependency")
        fi
    done < <(objdump -p "${binaries[i]}" | awk '/DLL Name:/ {print $3}')
done

cat > "$package_dir/qt.conf" <<'CONF'
[Paths]
Prefix=.
Plugins=.
QmlImports=qml
CONF
