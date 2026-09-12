# Mega Man Zero 1-4 Script Editor

A Qt 6 / Kirigami application for editing Mega Man Zero 1–4 `.tpl` script files.
<img width="826" height="548" alt="01" src="https://github.com/user-attachments/assets/abe96ffc-2807-4f75-9b9a-40a1ede95df0" />


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

## Usage

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
