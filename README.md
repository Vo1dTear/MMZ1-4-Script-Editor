# Mega Man Zero 1-4 Script Editor

A Qt 6 / Kirigami application for editing Mega Man Zero 1–4 `.tpl` script files.


## Features

- Open `.tpl` script files
- Edit individual scripts
- Save the current script or all scripts
- Global search (supports regular expressions)
- Undo / Redo
- Drag & drop support
- Automatic light/dark theme detection

## Requirements

- Qt 6.5 or newer (Quick, QML, Quick Controls 2; Qt Test for tests)
- KDE Kirigami 6 and Extra CMake Modules
- CMake 3.21 or newer and a C++17 compiler
- `qqc2-desktop-style` recommended for desktop theme integration

On Arch Linux:

```bash
sudo pacman -Syu base-devel cmake extra-cmake-modules qt6-base qt6-declarative kirigami qqc2-desktop-style
```

## Compatibility

This editor is designed to work with the `.tpl` script files included in the **MMZC GBA Script Restoration** project.

The mod is available at:
https://github.com/StraDaMa/MMZC-GBA-Script-Restoration

## Build

```bash
cmake -S . -B build-kirigami
cmake --build build-kirigami -j"$(nproc)"
ctest --test-dir build-kirigami --output-on-failure
./build-kirigami/MMZScriptEditor
```

## AppImage builds

The `Build AppImage` workflow runs on pushes to `main`, pull requests, and manual
dispatch. It builds on Ubuntu 24.04 with Qt 6 and KDE Frameworks from KDE neon,
runs the editor tests, and packages QML imports, the KDE desktop control style,
and the KIO local-file worker. The packaging tools run without requiring FUSE.

Before uploading `MMZScriptEditor.AppImage`, CI runs it with `--smoke-test` in a
clean Ubuntu 24.04 container without system Qt/KDE. This checks initial QML
creation, the event loop, and decoding of the four embedded action icons; it
does not replace interactive testing of the file
picker or editing. The build targets x86_64 and Ubuntu 24.04 or newer compatible
systems, not older glibc distributions. KDE neon and the linuxdeploy `continuous`
downloads are rolling inputs, so builds are not pinned or reproducible.

For local packaging, download both linuxdeploy AppImages as shown in the workflow
into `packaging/linuxdeploy/`, make them executable, then run
`bash packaging/appimage.sh`. Set `BUILD_DIR` to reuse a different CMake build
directory. The script rebuilds the editor before packaging.

## Windows builds

The `Build Windows` workflow runs on pushes to `main` and `kirigami`, pull
requests, and manual dispatch. It builds a Release executable for Windows x64
using MSYS2 UCRT64, Qt 6, and Kirigami, then runs the editor tests.

Download `MMZScriptEditor-Windows-x64` from the workflow's Artifacts section,
extract the entire archive, and launch `MMZScriptEditor.exe`. Keep the DLLs and
subdirectories beside the executable. No separate Qt or MSYS2 installation is
needed to run the package. CI checks startup with `--smoke-test` and a PATH
containing only Windows system directories before uploading it; interactive
file picker and theme behavior still need testing on Windows.

`packaging/windows.sh` uses `windeployqt6` to collect QML modules (including
Fusion) and Qt plugins, then copies transitive DLL dependencies from UCRT64.
For local packaging, run it from the repository root in an MSYS2 UCRT64 shell
after building. Set `BUILD_DIR` to use a build directory other than `build`.
MSYS2 packages are rolling inputs, so dependency versions are not pinned.

## Usage

On Windows 10, the interface uses Fusion for both Qt Widgets and Qt Quick
Controls so Qt 6.5 or newer can follow the system light/dark application theme.
Choose the default **app mode** in Windows Settings > Personalization > Colors;
the Windows mode setting alone does not select the theme for applications.
When packaging for Windows, include the `QtQuick.Controls.Fusion` QML module.

To verify on Windows 10, launch with app mode set to Light, switch to Dark while
the editor is open, and switch back. Check the page, text editor, script list,
search controls and Save/Discard/Cancel dialog, then relaunch in Dark mode.
The native file picker is managed separately by Windows and Qt.

1. Launch the application.
2. Open a `.tpl` file (`Ctrl+O`).
3. Select a script from the list.
4. Edit the text.
5. Save the current script (`Ctrl+S`) or save all changes.

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+O | Open TPL |
| Ctrl+S | Save Current Script |
| Ctrl+Shift+S | Save all scripts |
| Ctrl+Shift+F | Global Search |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |

## Credits

Special thanks to StraDaMa for the **MMZC GBA Script Restoration** project, which distributes the `.tpl` script files supported by this editor.

## License

This project is released under the MIT License.

## Architecture and file handling

`Main.qml` provides the Kirigami interface. `ScriptEditor` is a QObject backend for
UTF-8 TPL parsing, editing, per-script undo/redo, search and atomic file writes.
Kirigami is required; the application no longer uses a QWidget editor.

Save Current writes only the selected script; other edits remain in memory.
Save All writes all edited scripts. Opening another file or closing the window
prompts to save or discard unsaved changes. External file changes block saving
until the file is reopened. Headers, inter-script text and the original newline
style are retained. Script headers use `script NUMBER TYPE {`, with a standalone
closing `}`. A standalone closing brace inside an edited body is rejected.

Tests cover UTF-8/CRLF round trips, individual and full saves, undo across script
selection, search, malformed input, external edits and QML/backend integration.
Use `-DBUILD_TESTING=OFF` to build without the Qt Test dependency.
