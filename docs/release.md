# Release Notes

## Portable release

Build locally:

```powershell
cmake -S . -B build-msvc -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-msvc --config Release
```

GitHub Actions builds x64 and x86 portable ZIPs on push, pull request, manual dispatch, and tags matching `v*`.

## Installer

The Inno Setup script is available at:

```text
installer/TraySymbols.iss
```

It expects the local x64 build at `build-msvc/TraySymbols.exe`. The portable ZIP remains the primary v0.2 artifact; installer output is optional.

## Code signing

The project is not signed by default. When a certificate is available, sign `TraySymbols.exe` before packaging and sign the installer after it is built.

Suggested future CI setup:

- Store signing certificate/password as GitHub Actions secrets.
- Sign only on protected release tags.
- Keep pull-request builds unsigned.

## XP compatibility

XP support is a separate build track. The code is intentionally Win32-light, but the default toolchain targets modern Windows. Investigate an older MSVC toolset or MinGW build before promising XP binaries.
