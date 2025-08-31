# TraySymbols (Windows • AutoHotkey v2)
A tiny **tray tool** to quickly insert typographic symbols (—, –, ⸺, ❧, ❦, ✿, ⁂, · …).  
**Left‑click** the tray icon to open the menu. Clicking a symbol **copies to clipboard**.

## Features
- Tray menu with common symbols
- Left‑click opens the menu (right‑click works too)
- Robust copying via **WinAPI (CF_UNICODETEXT)**
- Hotstrings (e.g. `;em` → `—`, `;en` → `–`, `;dot` → `·`)

## Install
1. Install **AutoHotkey v2**: https://www.autohotkey.com/
2. Double‑click `TraySymbols_en.ahk` → tray icon appears.
3. (Autostart) `Win + R` → `shell:startup` → place file/shortcut there.

## Customize
- Add more symbols in the `symbols := [...]` array.
- Add hotstrings under the “Hotstrings” section.

## Build EXE
Use **Ahk2Exe** (bundled with AutoHotkey) and set a custom icon if desired.

## License
MIT.
