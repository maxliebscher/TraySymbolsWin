# TraySymbols v0.2.0

TraySymbols is now a standalone native Windows app. This release removes the AutoHotkey runtime requirement and ships a small Win32 tray tool for copying typographic symbols.

## Highlights

- Native Win32 tray app, no AutoHotkey dependency
- Portable x64 and x86 builds
- Symbol-first tray menu for quick visual scanning
- Built-in symbols: `—`, `–`, `⸺`, `⁂`, `❧`, `❦`, `✿`, `·`
- Custom symbols with persistent portable/AppData config
- Unicode clipboard copy via `CF_UNICODETEXT`
- English/German UI based on Windows UI language
- About dialog with version, architecture, config path, and GitHub link
- Config folder opener
- Current-user autostart toggle via Startup-folder shortcut

## Notes

- Auto-paste was intentionally removed because Windows focus/input rules made it unreliable. Copy a symbol, then paste manually with `Ctrl+V`.
- Global hotstrings from the old AutoHotkey version are not included in this release.
- The portable ZIP is the primary release artifact.
- The binaries are currently unsigned.

## Assets

- `TraySymbols-v0.2.0-x64.zip`
- `TraySymbols-v0.2.0-x86.zip`

Each ZIP contains:

- `TraySymbols.exe`
- `README.md`
- `LICENSE`
