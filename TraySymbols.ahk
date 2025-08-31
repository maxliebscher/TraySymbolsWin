#Requires AutoHotkey v2.0
#SingleInstance Force

; --- Ahk2Exe Build-Metadaten (optional, GUI kann überschreiben) ---
;@Ahk2Exe-Base "AutoHotkeyU64.exe"
;@Ahk2Exe-SetMainIcon "assets\traysymbols_asterism.ico"
;@Ahk2Exe-SetName "TraySymbols"
;@Ahk2Exe-SetProductName "TraySymbols"
;@Ahk2Exe-SetDescription "Schnelle typografische Zeichen aus dem Tray"
;@Ahk2Exe-SetVersion 0.1.0
;@Ahk2Exe-SetCompanyName "Max Liebscher"
;@Ahk2Exe-SetFileVersion 0.1.0.0
;@Ahk2Exe-SetOrigFilename "TraySymbols.exe"

; Versuche Runtime-Icon zu setzen (praktisch unkompiliert)
try TraySetIcon(A_ScriptDir "\assets\traysymbols_asterism.ico")

global PASTE_ON_CLICK := true
global g_LastWin := 0

; ===== Zeichenliste =====
symbols := [
    ["Em dash —",      "—"],
    ["En dash –",      "–"],
    ["Two-em dash ⸺",  "⸺"],
    ["Asterism ⁂",     "⁂"],
    ["Fleuron ❧",      "❧"],
    ["Floral heart ❦", "❦"],
    ["Flower ✿",       "✿"],
    ["Middle dot ·",   "·"]
]

; ===== Tray-Menü =====
BuildMenu()

; Linksklick/Rechtsklick auf Tray-Icon → Menü zeigen
OnMessage(0x404, TrayHook)
TrayHook(wParam, lParam, msg, hwnd) {
    global g_LastWin
    if (lParam = 0x202) || (lParam = 0x205) {
        g_LastWin := WinExist("A")
        A_TrayMenu.Show()
        return 0
    }
}

; ===== Hotstrings (Tippen → Zeichen) =====
::;em::—
::;en::–
::;2em::⸺
::;ast::⁂
::;fl::❧
::;fh::❦
::;flower::✿
::;dot::·

; ===== Funktionen =====
BuildMenu() {
    A_TrayMenu.Delete()
    A_TrayMenu.Add("Einfügen (Klick kopiert & tippt ein):", (*) => 0)
    A_TrayMenu.Disable("Einfügen (Klick kopiert & tippt ein):")
    for sym in symbols {
        label := sym[1], ch := sym[2]
        A_TrayMenu.Add(label, InsertSymbol.Bind(ch))
    }
    A_TrayMenu.Add()
    A_TrayMenu.Add("Modus: Beim Klick einfügen (" . (PASTE_ON_CLICK ? "AN" : "AUS") . ")", TogglePaste)
    A_TrayMenu.Add("Beenden", (*) => ExitApp())
}

TogglePaste(*) {
    global PASTE_ON_CLICK
    PASTE_ON_CLICK := !PASTE_ON_CLICK
    BuildMenu()
}

InsertSymbol(text, args*) {
    global g_LastWin, PASTE_ON_CLICK
    if !SetClipboardUnicode(text) {
        ToolTip("Kopieren fehlgeschlagen.")
        SetTimer(() => ToolTip(), -1200)
        return
    }
    tt := "Kopiert: " . A_Clipboard
    if PASTE_ON_CLICK && g_LastWin
        tt .= "  •  eingefügt"
    ToolTip(tt), SetTimer(() => ToolTip(), -1400)
    if PASTE_ON_CLICK && g_LastWin {
        try {
            WinActivate g_LastWin
            Sleep 80
            Send "^v"
        }
    }
}

; Setzt Unicode-Text direkt per WinAPI (CF_UNICODETEXT), robust gegen Clipboard-Manager
SetClipboardUnicode(text) {
    sz := (StrLen(text) + 1) * 2
    hMem := DllCall("GlobalAlloc", "UInt", 0x0042, "Ptr", sz, "Ptr") ; GHND
    if !hMem
        return false
    pMem := DllCall("GlobalLock", "Ptr", hMem, "Ptr")
    if !pMem
        return false
    StrPut(text, pMem, "UTF-16")
    DllCall("GlobalUnlock", "Ptr", hMem)
    if !DllCall("OpenClipboard", "Ptr", 0)
        return false
    DllCall("EmptyClipboard")
    if !DllCall("SetClipboardData", "UInt", 13, "Ptr", hMem) { ; CF_UNICODETEXT
        DllCall("CloseClipboard")
        return false
    }
    DllCall("CloseClipboard")
    return true
}
