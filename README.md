# Mega Man Zero 1-4 Script Editor

A simple Qt6 application for editing Mega Man Zero 1–4 `.tpl` script files.

## Features

- Open `.tpl` script files
- Edit individual scripts
- Save the current script or all scripts
- Global search (supports regular expressions)
- Undo / Redo
- Drag & drop support
- Automatic light/dark theme detection

## Requirements

- Qt 6.5
- CMake
- C++17 compatible compiler

## Compatibility

This editor is designed to work with the `.tpl` script files included in the **MMZC GBA Script Restoration** project.

The mod is available at:
https://github.com/StraDaMa/MMZC-GBA-Script-Restoration

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
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
| Ctrl+Shift+F | Global Search |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |

## Credits

Special thanks to StraDaMa for the **MMZC GBA Script Restoration** project, which distributes the `.tpl` script files supported by this editor.

## License

This project is released under the MIT License.
