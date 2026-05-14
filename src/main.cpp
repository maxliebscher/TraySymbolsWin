#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <strsafe.h>

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <iterator>
#include <string>
#include <vector>

#include "resource.h"
#include "version.h"

namespace {

constexpr wchar_t kAppName[] = L"TraySymbols";
constexpr wchar_t kRepositoryUrl[] = L"https://github.com/maxliebscher/TraySymbolsWin";
constexpr wchar_t kConfigFileName[] = L"TraySymbols.ini";
constexpr wchar_t kConfigSection[] = L"Symbols";
constexpr wchar_t kManagerClassName[] = L"TraySymbolsManagerWindow";
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kIdBuiltInBase = 1000;
constexpr UINT kIdCustomBase = 2000;
constexpr UINT kIdManageSymbols = 3002;
constexpr UINT kIdAbout = 3003;
constexpr UINT kIdOpenConfigFolder = 3004;
constexpr UINT kIdToggleAutostart = 3005;
constexpr UINT kIdExit = 3006;
constexpr UINT kManagerMinWidth = 780;
constexpr UINT kManagerMinHeight = 390;
constexpr int kMenuSymbolColumnWidth = 34;
constexpr int kMenuPipeColumnWidth = 12;
constexpr int kMenuTextGap = 6;
constexpr int kMenuHorizontalPadding = 10;
constexpr int kMenuVerticalPadding = 5;

constexpr int IDC_SYMBOL_LIST = 4101;
constexpr int IDC_LABEL_EDIT = 4102;
constexpr int IDC_TEXT_EDIT = 4103;
constexpr int IDC_ADD = 4104;
constexpr int IDC_UPDATE = 4105;
constexpr int IDC_DELETE = 4106;
constexpr int IDC_UP = 4107;
constexpr int IDC_DOWN = 4108;
constexpr int IDC_OK = 4109;
constexpr int IDC_CANCEL = 4110;

struct SymbolEntry {
    std::wstring label;
    std::wstring text;
};

struct BuiltInSymbol {
    const wchar_t* text;
    const wchar_t* englishLabel;
    const wchar_t* germanLabel;
};

enum class Language {
    English,
    German,
};

struct UiStrings {
    const wchar_t* customHeader;
    const wchar_t* manageSymbols;
    const wchar_t* about;
    const wchar_t* openConfigFolder;
    const wchar_t* startWithWindows;
    const wchar_t* exit;
    const wchar_t* copyFailed;
    const wchar_t* copied;
    const wchar_t* labelField;
    const wchar_t* textField;
    const wchar_t* add;
    const wchar_t* update;
    const wchar_t* remove;
    const wchar_t* up;
    const wchar_t* down;
    const wchar_t* ok;
    const wchar_t* cancel;
    const wchar_t* managerTitle;
    const wchar_t* emptyTextMessage;
    const wchar_t* saveFailedMessage;
    const wchar_t* managerOpenFailedMessage;
    const wchar_t* alreadyRunningMessage;
    const wchar_t* startupFailedMessage;
    const wchar_t* windowCreateFailedMessage;
    const wchar_t* configKindPortable;
    const wchar_t* configKindAppData;
    const wchar_t* architectureLabel;
    const wchar_t* configPathLabel;
    const wchar_t* configModeLabel;
    const wchar_t* usageHint;
    const wchar_t* configOpenFailedPrefix;
    const wchar_t* autostartEnableFailed;
    const wchar_t* autostartDisableFailed;
    const wchar_t* openGithub;
    const wchar_t* aboutDetails;
};

struct ManagerState {
    HWND hwnd = nullptr;
    HWND list = nullptr;
    HWND labelEdit = nullptr;
    HWND textEdit = nullptr;
    HFONT font = nullptr;
    std::vector<SymbolEntry> items;
    int selected = -1;
    bool saved = false;
};

struct MenuVisualItem {
    std::wstring symbol;
    std::wstring label;
};

HINSTANCE g_instance = nullptr;
HWND g_mainWindow = nullptr;
HICON g_icon = nullptr;
UINT g_taskbarCreatedMessage = 0;
Language g_language = Language::English;
std::wstring g_configPath;
bool g_usingPortableConfig = true;
std::vector<SymbolEntry> g_customSymbols;

const std::vector<BuiltInSymbol> kBuiltInSymbols = {
    {L"—", L"Em dash", L"Geviertstrich"},
    {L"–", L"En dash", L"Halbgeviertstrich"},
    {L"⸺", L"Two-em dash", L"Doppelgeviertstrich"},
    {L"⁂", L"Asterism", L"Asterismus"},
    {L"❧", L"Fleuron", L"Fleuron"},
    {L"❦", L"Floral heart", L"Florales Herz"},
    {L"✿", L"Flower", L"Blume"},
    {L"·", L"Middle dot", L"Mittelpunkt"},
};

const UiStrings kEnglishStrings = {
    L"Custom symbols",
    L"Manage symbols...",
    L"About...",
    L"Open config folder",
    L"Start with Windows",
    L"Exit",
    L"Copy failed.",
    L"Copied: ",
    L"Label",
    L"Symbol/Text",
    L"Add",
    L"Update",
    L"Delete",
    L"Up",
    L"Down",
    L"OK",
    L"Cancel",
    L"Manage symbols",
    L"Please enter a symbol or text.",
    L"Symbols could not be saved.",
    L"The symbol manager could not be opened.",
    L"TraySymbols is already running.",
    L"TraySymbols could not be started.",
    L"TraySymbols could not create its main window.",
    L"Portable config next to the EXE",
    L"AppData fallback config",
    L"Architecture",
    L"Config path",
    L"Config mode",
    L"Symbols are copied to the clipboard. Paste manually with Ctrl+V.",
    L"Could not open config folder:",
    L"Autostart shortcut could not be created.",
    L"Autostart shortcut could not be removed.",
    L"Open GitHub",
    L"Show details",
};

const UiStrings kGermanStrings = {
    L"Eigene Zeichen",
    L"Zeichen verwalten...",
    L"Info...",
    L"Config-Ordner öffnen",
    L"Mit Windows starten",
    L"Beenden",
    L"Kopieren fehlgeschlagen.",
    L"Kopiert: ",
    L"Label",
    L"Zeichen/Text",
    L"Hinzufügen",
    L"Aktualisieren",
    L"Löschen",
    L"Hoch",
    L"Runter",
    L"OK",
    L"Abbrechen",
    L"Zeichen verwalten",
    L"Bitte ein Zeichen oder einen Text eintragen.",
    L"Die Zeichen konnten nicht gespeichert werden.",
    L"Der Zeichen-Dialog konnte nicht geöffnet werden.",
    L"TraySymbols läuft bereits.",
    L"TraySymbols konnte nicht gestartet werden.",
    L"TraySymbols konnte kein Hauptfenster erstellen.",
    L"Portable Config neben der EXE",
    L"AppData-Fallback-Config",
    L"Architektur",
    L"Config-Pfad",
    L"Config-Modus",
    L"Zeichen werden in die Zwischenablage kopiert. Einfügen manuell mit Strg+V.",
    L"Config-Ordner konnte nicht geöffnet werden:",
    L"Autostart-Verknüpfung konnte nicht erstellt werden.",
    L"Autostart-Verknüpfung konnte nicht entfernt werden.",
    L"GitHub öffnen",
    L"Details anzeigen",
};

const UiStrings& Ui()
{
    return g_language == Language::German ? kGermanStrings : kEnglishStrings;
}

Language DetectLanguage()
{
    const WORD primaryLanguage = PRIMARYLANGID(GetUserDefaultUILanguage());
    return primaryLanguage == LANG_GERMAN ? Language::German : Language::English;
}

const wchar_t* BuiltInLabel(const BuiltInSymbol& symbol)
{
    return g_language == Language::German ? symbol.germanLabel : symbol.englishLabel;
}

std::wstring GetWindowTextString(HWND hwnd)
{
    const int length = GetWindowTextLengthW(hwnd);
    std::wstring value(static_cast<size_t>(length) + 1, L'\0');
    if (length > 0) {
        GetWindowTextW(hwnd, value.data(), length + 1);
    }
    value.resize(static_cast<size_t>(length));
    return value;
}

std::wstring Trim(std::wstring value)
{
    const wchar_t* whitespace = L" \t\r\n";
    const size_t begin = value.find_first_not_of(whitespace);
    if (begin == std::wstring::npos) {
        return L"";
    }
    const size_t end = value.find_last_not_of(whitespace);
    return value.substr(begin, end - begin + 1);
}

std::wstring ParentDirectory(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, slash);
}

std::wstring JoinPath(const std::wstring& left, const std::wstring& right)
{
    if (left.empty()) {
        return right;
    }
    if (left.back() == L'\\' || left.back() == L'/') {
        return left + right;
    }
    return left + L"\\" + right;
}

bool FileExists(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool DirectoryExists(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool EnsureDirectory(const std::wstring& path)
{
    if (DirectoryExists(path)) {
        return true;
    }
    return SHCreateDirectoryExW(nullptr, path.c_str(), nullptr) == ERROR_SUCCESS || DirectoryExists(path);
}

bool IsDirectoryWritable(const std::wstring& directory)
{
    const std::wstring testPath = JoinPath(directory, L".__traysymbols_write_test.tmp");
    HANDLE file = CreateFileW(
        testPath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    CloseHandle(file);
    return true;
}

bool IsFileWritable(const std::wstring& path)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    CloseHandle(file);
    return true;
}

std::wstring GetExecutableDirectory()
{
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
    return ParentDirectory(path);
}

std::wstring GetExecutablePath()
{
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
    return path;
}

bool SamePath(const std::wstring& left, const std::wstring& right)
{
    wchar_t normalizedLeft[MAX_PATH] = {};
    wchar_t normalizedRight[MAX_PATH] = {};
    GetFullPathNameW(left.c_str(), static_cast<DWORD>(std::size(normalizedLeft)), normalizedLeft, nullptr);
    GetFullPathNameW(right.c_str(), static_cast<DWORD>(std::size(normalizedRight)), normalizedRight, nullptr);
    return CompareStringOrdinal(normalizedLeft, -1, normalizedRight, -1, TRUE) == CSTR_EQUAL;
}

std::wstring GetAppDataConfigPath()
{
    wchar_t appData[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, appData))) {
        return JoinPath(GetExecutableDirectory(), kConfigFileName);
    }

    const std::wstring directory = JoinPath(appData, kAppName);
    EnsureDirectory(directory);
    return JoinPath(directory, kConfigFileName);
}

std::wstring ResolveConfigPath()
{
    const std::wstring portablePath = JoinPath(GetExecutableDirectory(), kConfigFileName);
    if ((FileExists(portablePath) && IsFileWritable(portablePath)) || (!FileExists(portablePath) && IsDirectoryWritable(GetExecutableDirectory()))) {
        g_usingPortableConfig = true;
        return portablePath;
    }

    g_usingPortableConfig = false;
    return GetAppDataConfigPath();
}

bool EnsureUtf16IniFile(const std::wstring& path)
{
    EnsureDirectory(ParentDirectory(path));
    if (FileExists(path)) {
        return true;
    }

    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    const WORD bom = 0xFEFF;
    DWORD written = 0;
    const BOOL ok = WriteFile(file, &bom, sizeof(bom), &written, nullptr);
    CloseHandle(file);
    return ok && written == sizeof(bom);
}

std::wstring ReadIniString(const wchar_t* section, const wchar_t* key, const std::wstring& path)
{
    std::vector<wchar_t> buffer(4096);
    GetPrivateProfileStringW(section, key, L"", buffer.data(), static_cast<DWORD>(buffer.size()), path.c_str());
    return buffer.data();
}

void LoadCustomSymbols()
{
    g_customSymbols.clear();
    const UINT count = GetPrivateProfileIntW(kConfigSection, L"Count", 0, g_configPath.c_str());
    const UINT cappedCount = std::min<UINT>(count, 500);

    for (UINT index = 1; index <= cappedCount; ++index) {
        wchar_t key[32] = {};
        StringCchPrintfW(key, std::size(key), L"Label%u", index);
        std::wstring label = ReadIniString(kConfigSection, key, g_configPath);

        StringCchPrintfW(key, std::size(key), L"Text%u", index);
        std::wstring text = ReadIniString(kConfigSection, key, g_configPath);

        label = Trim(label);
        text = Trim(text);
        if (!text.empty()) {
            if (label.empty()) {
                label = text;
            }
            g_customSymbols.push_back({label, text});
        }
    }
}

bool WriteConfigToPath(const std::wstring& path, const std::vector<SymbolEntry>& symbols)
{
    if (!EnsureUtf16IniFile(path)) {
        return false;
    }

    WritePrivateProfileStringW(kConfigSection, nullptr, nullptr, path.c_str());

    wchar_t value[32] = {};
    StringCchPrintfW(value, std::size(value), L"%u", static_cast<unsigned int>(symbols.size()));
    if (!WritePrivateProfileStringW(kConfigSection, L"Count", value, path.c_str())) {
        return false;
    }

    for (size_t index = 0; index < symbols.size(); ++index) {
        wchar_t key[32] = {};
        StringCchPrintfW(key, std::size(key), L"Label%u", static_cast<unsigned int>(index + 1));
        if (!WritePrivateProfileStringW(kConfigSection, key, symbols[index].label.c_str(), path.c_str())) {
            return false;
        }

        StringCchPrintfW(key, std::size(key), L"Text%u", static_cast<unsigned int>(index + 1));
        if (!WritePrivateProfileStringW(kConfigSection, key, symbols[index].text.c_str(), path.c_str())) {
            return false;
        }
    }

    return true;
}

bool SaveCustomSymbols(const std::vector<SymbolEntry>& symbols)
{
    if (WriteConfigToPath(g_configPath, symbols)) {
        g_customSymbols = symbols;
        return true;
    }

    const std::wstring fallback = GetAppDataConfigPath();
    if (fallback != g_configPath && WriteConfigToPath(fallback, symbols)) {
        g_configPath = fallback;
        g_usingPortableConfig = false;
        g_customSymbols = symbols;
        return true;
    }

    return false;
}

std::wstring GetStartupShortcutPath()
{
    wchar_t startupPath[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_STARTUP | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, startupPath))) {
        return L"";
    }
    return JoinPath(startupPath, L"TraySymbols.lnk");
}

bool ResolveShortcutTarget(const std::wstring& shortcutPath, std::wstring* targetPath)
{
    if (!FileExists(shortcutPath)) {
        return false;
    }

    bool initializedCom = false;
    const HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(initResult)) {
        initializedCom = initResult == S_OK || initResult == S_FALSE;
    }

    IShellLinkW* link = nullptr;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&link));
    if (FAILED(result)) {
        if (initializedCom) {
            CoUninitialize();
        }
        return false;
    }

    IPersistFile* file = nullptr;
    result = link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&file));
    if (SUCCEEDED(result)) {
        result = file->Load(shortcutPath.c_str(), STGM_READ);
        if (SUCCEEDED(result)) {
            wchar_t resolved[MAX_PATH] = {};
            WIN32_FIND_DATAW findData = {};
            result = link->GetPath(resolved, static_cast<int>(std::size(resolved)), &findData, SLGP_RAWPATH);
            if (SUCCEEDED(result) && resolved[0] != L'\0') {
                *targetPath = resolved;
            }
        }
        file->Release();
    }

    link->Release();
    if (initializedCom) {
        CoUninitialize();
    }
    return !targetPath->empty();
}

bool IsAutostartEnabled()
{
    const std::wstring shortcutPath = GetStartupShortcutPath();
    std::wstring targetPath;
    return !shortcutPath.empty() && ResolveShortcutTarget(shortcutPath, &targetPath) && SamePath(targetPath, GetExecutablePath());
}

bool CreateAutostartShortcut()
{
    const std::wstring shortcutPath = GetStartupShortcutPath();
    if (shortcutPath.empty() || !EnsureDirectory(ParentDirectory(shortcutPath))) {
        return false;
    }

    bool initializedCom = false;
    const HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(initResult)) {
        initializedCom = initResult == S_OK || initResult == S_FALSE;
    }

    IShellLinkW* link = nullptr;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&link));
    if (FAILED(result)) {
        if (initializedCom) {
            CoUninitialize();
        }
        return false;
    }

    const std::wstring executablePath = GetExecutablePath();
    link->SetPath(executablePath.c_str());
    link->SetWorkingDirectory(GetExecutableDirectory().c_str());
    link->SetDescription(L"TraySymbols");

    IPersistFile* file = nullptr;
    result = link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&file));
    if (SUCCEEDED(result)) {
        result = file->Save(shortcutPath.c_str(), TRUE);
        file->Release();
    }

    link->Release();
    if (initializedCom) {
        CoUninitialize();
    }
    return SUCCEEDED(result);
}

bool RemoveAutostartShortcut()
{
    const std::wstring shortcutPath = GetStartupShortcutPath();
    std::wstring targetPath;
    if (shortcutPath.empty() || !ResolveShortcutTarget(shortcutPath, &targetPath) || !SamePath(targetPath, GetExecutablePath())) {
        return true;
    }
    return DeleteFileW(shortcutPath.c_str()) != FALSE || !FileExists(shortcutPath);
}

void CopyToFixed(wchar_t* destination, size_t destinationCount, const std::wstring& value)
{
    StringCchCopyW(destination, destinationCount, value.c_str());
}

void AddTrayIcon()
{
    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = g_mainWindow;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = kTrayMessage;
    data.hIcon = g_icon;
    CopyToFixed(data.szTip, std::size(data.szTip), L"TraySymbols");
    Shell_NotifyIconW(NIM_ADD, &data);
}

void RemoveTrayIcon()
{
    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = g_mainWindow;
    data.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &data);
}

void ShowBalloon(const std::wstring& message)
{
    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = g_mainWindow;
    data.uID = 1;
    data.uFlags = NIF_INFO;
    data.dwInfoFlags = NIIF_INFO;
    data.uTimeout = 1400;
    CopyToFixed(data.szInfoTitle, std::size(data.szInfoTitle), L"TraySymbols");
    CopyToFixed(data.szInfo, std::size(data.szInfo), message);
    Shell_NotifyIconW(NIM_MODIFY, &data);
}

bool SetClipboardUnicode(const std::wstring& text)
{
    if (text.empty()) {
        return false;
    }

    bool opened = false;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (OpenClipboard(g_mainWindow)) {
            opened = true;
            break;
        }
        Sleep(20);
    }

    if (!opened) {
        return false;
    }

    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) {
        CloseClipboard();
        return false;
    }

    void* locked = GlobalLock(memory);
    if (!locked) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }

    memcpy(locked, text.c_str(), bytes);
    GlobalUnlock(memory);

    EmptyClipboard();
    if (!SetClipboardData(CF_UNICODETEXT, memory)) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

void InsertSymbol(const SymbolEntry& symbol)
{
    if (!SetClipboardUnicode(symbol.text)) {
        ShowBalloon(Ui().copyFailed);
        return;
    }

    std::wstring message = std::wstring(Ui().copied) + symbol.text;
    ShowBalloon(message);
}

std::wstring ArchitectureName()
{
#if defined(_M_X64) || defined(__x86_64__)
    return L"x64";
#elif defined(_M_IX86) || defined(__i386__)
    return L"x86";
#elif defined(_M_ARM64) || defined(__aarch64__)
    return L"ARM64";
#else
    return L"unknown";
#endif
}

void OpenGitHub()
{
    ShellExecuteW(g_mainWindow, L"open", kRepositoryUrl, nullptr, nullptr, SW_SHOWNORMAL);
}

void OpenConfigFolder()
{
    const std::wstring directory = ParentDirectory(g_configPath);
    EnsureDirectory(directory);
    EnsureUtf16IniFile(g_configPath);

    HINSTANCE result = ShellExecuteW(g_mainWindow, L"open", directory.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        const std::wstring message = std::wstring(Ui().configOpenFailedPrefix) + L"\r\n" + directory;
        MessageBoxW(g_mainWindow, message.c_str(), kAppName, MB_OK | MB_ICONERROR);
    }
}

void ShowAboutDialog()
{
    constexpr int kButtonOpenGitHub = 1001;
    constexpr int kButtonOpenConfigFolder = 1002;
    using TaskDialogIndirectFn = HRESULT(WINAPI*)(const TASKDIALOGCONFIG*, int*, int*, BOOL*);

    const std::wstring mainInstruction = std::wstring(kAppName) + L" " + TRAYSYMBOLS_VERSION_TEXT_W;
    const std::wstring content =
        Ui().architectureLabel + std::wstring(L": ") + ArchitectureName() + L"\r\n" +
        Ui().configModeLabel + std::wstring(L": ") + (g_usingPortableConfig ? Ui().configKindPortable : Ui().configKindAppData) + L"\r\n\r\n" +
        Ui().usageHint;
    const std::wstring details =
        L"GitHub:\r\n" + std::wstring(kRepositoryUrl) + L"\r\n\r\n" +
        Ui().configPathLabel + std::wstring(L":\r\n") + g_configPath;

    const TASKDIALOG_BUTTON buttons[] = {
        {kButtonOpenGitHub, Ui().openGithub},
        {kButtonOpenConfigFolder, Ui().openConfigFolder},
    };

    HMODULE comctl = LoadLibraryW(L"comctl32.dll");
    TaskDialogIndirectFn taskDialog = comctl ? reinterpret_cast<TaskDialogIndirectFn>(GetProcAddress(comctl, "TaskDialogIndirect")) : nullptr;
    if (taskDialog) {
        TASKDIALOGCONFIG config = {};
        config.cbSize = sizeof(config);
        config.hwndParent = g_mainWindow;
        config.hInstance = g_instance;
        config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SIZE_TO_CONTENT;
        config.pszWindowTitle = kAppName;
        config.pszMainIcon = TD_INFORMATION_ICON;
        config.pszMainInstruction = mainInstruction.c_str();
        config.pszContent = content.c_str();
        config.cButtons = static_cast<UINT>(std::size(buttons));
        config.pButtons = buttons;
        config.nDefaultButton = IDOK;
        config.dwCommonButtons = TDCBF_OK_BUTTON;
        config.pszExpandedControlText = Ui().aboutDetails;
        config.pszExpandedInformation = details.c_str();

        int button = IDOK;
        if (SUCCEEDED(taskDialog(&config, &button, nullptr, nullptr))) {
            if (button == kButtonOpenGitHub) {
                OpenGitHub();
            } else if (button == kButtonOpenConfigFolder) {
                OpenConfigFolder();
            }
            FreeLibrary(comctl);
            return;
        }
    }

    if (comctl) {
        FreeLibrary(comctl);
    }

    const std::wstring fallback =
        mainInstruction + L"\r\n\r\n" +
        L"GitHub:\r\n" + std::wstring(kRepositoryUrl) + L"\r\n\r\n" +
        content + L"\r\n\r\n" +
        Ui().configPathLabel + std::wstring(L":\r\n") + g_configPath;
    MessageBoxW(g_mainWindow, fallback.c_str(), kAppName, MB_OK | MB_ICONINFORMATION);
}

void ToggleAutostart()
{
    if (IsAutostartEnabled()) {
        if (!RemoveAutostartShortcut()) {
            MessageBoxW(g_mainWindow, Ui().autostartDisableFailed, kAppName, MB_OK | MB_ICONERROR);
        }
        return;
    }

    if (!CreateAutostartShortcut()) {
        MessageBoxW(g_mainWindow, Ui().autostartEnableFailed, kAppName, MB_OK | MB_ICONERROR);
    }
}

std::wstring MenuLabelForSymbol(const std::wstring& text, const std::wstring& label)
{
    if (label.empty() || label == text) {
        return text;
    }
    return text + L" | " + label;
}

std::wstring MenuLabelForCustomSymbol(const SymbolEntry& symbol)
{
    return MenuLabelForSymbol(symbol.text, symbol.label);
}

SIZE MeasureText(HDC dc, const std::wstring& text)
{
    SIZE size = {};
    if (!text.empty()) {
        GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
    }
    return size;
}

void MeasureSymbolMenuItem(MEASUREITEMSTRUCT* measure)
{
    const MenuVisualItem* item = reinterpret_cast<const MenuVisualItem*>(measure->itemData);
    if (!item) {
        return;
    }

    HDC dc = GetDC(g_mainWindow);
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT)));
    const SIZE labelSize = MeasureText(dc, item->label);
    const SIZE symbolSize = MeasureText(dc, item->symbol);
    SelectObject(dc, oldFont);
    ReleaseDC(g_mainWindow, dc);

    measure->itemWidth = static_cast<UINT>(
        (kMenuHorizontalPadding * 2) +
        kMenuSymbolColumnWidth +
        kMenuPipeColumnWidth +
        kMenuTextGap +
        labelSize.cx);
    measure->itemHeight = static_cast<UINT>(std::max<int>(GetSystemMetrics(SM_CYMENU), std::max(labelSize.cy, symbolSize.cy) + (kMenuVerticalPadding * 2)));
}

void DrawSymbolMenuItem(const DRAWITEMSTRUCT* draw)
{
    const MenuVisualItem* item = reinterpret_cast<const MenuVisualItem*>(draw->itemData);
    if (!item) {
        return;
    }

    const bool selected = (draw->itemState & ODS_SELECTED) != 0;
    const COLORREF background = GetSysColor(selected ? COLOR_HIGHLIGHT : COLOR_MENU);
    const COLORREF textColor = GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);
    const COLORREF separatorColor = GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT);

    HBRUSH brush = CreateSolidBrush(background);
    FillRect(draw->hDC, &draw->rcItem, brush);
    DeleteObject(brush);

    const int oldBkMode = SetBkMode(draw->hDC, TRANSPARENT);
    const COLORREF oldTextColor = SetTextColor(draw->hDC, textColor);
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(draw->hDC, GetStockObject(DEFAULT_GUI_FONT)));

    RECT symbolRect = draw->rcItem;
    symbolRect.left += kMenuHorizontalPadding;
    symbolRect.right = symbolRect.left + kMenuSymbolColumnWidth;

    RECT pipeRect = symbolRect;
    pipeRect.left = symbolRect.right;
    pipeRect.right = pipeRect.left + kMenuPipeColumnWidth;

    RECT labelRect = pipeRect;
    labelRect.left = pipeRect.right + kMenuTextGap;
    labelRect.right = draw->rcItem.right - kMenuHorizontalPadding;

    DrawTextW(draw->hDC, item->symbol.c_str(), static_cast<int>(item->symbol.size()), &symbolRect, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
    SetTextColor(draw->hDC, separatorColor);
    DrawTextW(draw->hDC, L"|", 1, &pipeRect, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
    SetTextColor(draw->hDC, textColor);
    DrawTextW(draw->hDC, item->label.c_str(), static_cast<int>(item->label.size()), &labelRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);

    SelectObject(draw->hDC, oldFont);
    SetTextColor(draw->hDC, oldTextColor);
    SetBkMode(draw->hDC, oldBkMode);
}

void AppendSymbolMenuItem(HMENU menu, UINT id, std::vector<MenuVisualItem>* visuals, const std::wstring& symbol, const std::wstring& label)
{
    visuals->push_back({symbol, label.empty() ? symbol : label});
    AppendMenuW(menu, MF_OWNERDRAW, id, reinterpret_cast<LPCWSTR>(&visuals->back()));
}

void ShowTrayMenu()
{
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    std::vector<MenuVisualItem> visuals;
    visuals.reserve(kBuiltInSymbols.size() + g_customSymbols.size());

    AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, L"TraySymbols");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kIdManageSymbols, Ui().manageSymbols);
    AppendMenuW(menu, MF_STRING, kIdAbout, Ui().about);
    AppendMenuW(menu, MF_STRING, kIdOpenConfigFolder, Ui().openConfigFolder);
    AppendMenuW(menu, MF_STRING | (IsAutostartEnabled() ? MF_CHECKED : MF_UNCHECKED), kIdToggleAutostart, Ui().startWithWindows);
    AppendMenuW(menu, MF_STRING, kIdExit, Ui().exit);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    if (!g_customSymbols.empty()) {
        AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, Ui().customHeader);
        for (size_t offset = 0; offset < g_customSymbols.size(); ++offset) {
            const size_t index = g_customSymbols.size() - 1 - offset;
            AppendSymbolMenuItem(menu, kIdCustomBase + static_cast<UINT>(index), &visuals, g_customSymbols[index].text, g_customSymbols[index].label);
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }

    for (size_t offset = 0; offset < kBuiltInSymbols.size(); ++offset) {
        const size_t index = kBuiltInSymbols.size() - 1 - offset;
        const BuiltInSymbol& symbol = kBuiltInSymbols[index];
        AppendSymbolMenuItem(menu, kIdBuiltInBase + static_cast<UINT>(index), &visuals, symbol.text, BuiltInLabel(symbol));
    }

    POINT cursor = {};
    GetCursorPos(&cursor);
    SetForegroundWindow(g_mainWindow);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_BOTTOMALIGN, cursor.x, cursor.y, 0, g_mainWindow, nullptr);
    PostMessageW(g_mainWindow, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

void SetControlFont(HWND control, HFONT font)
{
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

HMENU ControlId(int id)
{
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

void PopulateManagerList(ManagerState* state)
{
    SendMessageW(state->list, LB_RESETCONTENT, 0, 0);
    for (const SymbolEntry& item : state->items) {
        const std::wstring label = MenuLabelForCustomSymbol(item);
        SendMessageW(state->list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }

    if (state->selected >= static_cast<int>(state->items.size())) {
        state->selected = static_cast<int>(state->items.size()) - 1;
    }

    if (state->selected >= 0) {
        SendMessageW(state->list, LB_SETCURSEL, static_cast<WPARAM>(state->selected), 0);
    }
}

void UpdateManagerEditFields(ManagerState* state)
{
    if (state->selected >= 0 && state->selected < static_cast<int>(state->items.size())) {
        SetWindowTextW(state->labelEdit, state->items[static_cast<size_t>(state->selected)].label.c_str());
        SetWindowTextW(state->textEdit, state->items[static_cast<size_t>(state->selected)].text.c_str());
    } else {
        SetWindowTextW(state->labelEdit, L"");
        SetWindowTextW(state->textEdit, L"");
    }
}

bool ReadManagerEntry(ManagerState* state, SymbolEntry* entry)
{
    std::wstring label = Trim(GetWindowTextString(state->labelEdit));
    std::wstring text = Trim(GetWindowTextString(state->textEdit));
    if (text.empty()) {
        MessageBoxW(state->hwnd, Ui().emptyTextMessage, kAppName, MB_ICONINFORMATION);
        return false;
    }
    if (label.empty()) {
        label = text;
    }

    entry->label = label;
    entry->text = text;
    return true;
}

void AddManagerEntry(ManagerState* state)
{
    SymbolEntry entry;
    if (!ReadManagerEntry(state, &entry)) {
        return;
    }

    state->items.push_back(entry);
    state->selected = static_cast<int>(state->items.size()) - 1;
    PopulateManagerList(state);
    UpdateManagerEditFields(state);
}

void UpdateSelectedManagerEntry(ManagerState* state)
{
    if (state->selected < 0 || state->selected >= static_cast<int>(state->items.size())) {
        return;
    }

    SymbolEntry entry;
    if (!ReadManagerEntry(state, &entry)) {
        return;
    }

    state->items[static_cast<size_t>(state->selected)] = entry;
    PopulateManagerList(state);
    UpdateManagerEditFields(state);
}

void DeleteSelectedManagerEntry(ManagerState* state)
{
    if (state->selected < 0 || state->selected >= static_cast<int>(state->items.size())) {
        return;
    }

    state->items.erase(state->items.begin() + state->selected);
    if (state->selected >= static_cast<int>(state->items.size())) {
        state->selected = static_cast<int>(state->items.size()) - 1;
    }
    PopulateManagerList(state);
    UpdateManagerEditFields(state);
}

void MoveSelectedManagerEntry(ManagerState* state, int direction)
{
    const int next = state->selected + direction;
    if (state->selected < 0 || next < 0 || next >= static_cast<int>(state->items.size())) {
        return;
    }

    std::swap(state->items[static_cast<size_t>(state->selected)], state->items[static_cast<size_t>(next)]);
    state->selected = next;
    PopulateManagerList(state);
    UpdateManagerEditFields(state);
}

void LayoutManagerWindow(ManagerState* state)
{
    RECT client = {};
    GetClientRect(state->hwnd, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    const int margin = 12;
    const int gap = 8;
    const int buttonHeight = 28;
    const int editHeight = 24;
    const int listWidth = 245;
    const int rightX = margin + listWidth + 18;
    const int rightWidth = width - rightX - margin;
    const int bottomY = height - margin - buttonHeight;

    MoveWindow(state->list, margin, margin, listWidth, bottomY - margin - gap, TRUE);

    HWND labelStatic = GetDlgItem(state->hwnd, 4201);
    HWND textStatic = GetDlgItem(state->hwnd, 4202);
    MoveWindow(labelStatic, rightX, margin, rightWidth, 18, TRUE);
    MoveWindow(state->labelEdit, rightX, margin + 20, rightWidth, editHeight, TRUE);
    MoveWindow(textStatic, rightX, margin + 54, rightWidth, 18, TRUE);
    MoveWindow(state->textEdit, rightX, margin + 74, rightWidth, editHeight, TRUE);

    const int smallButtonWidth = (rightWidth - (4 * gap)) / 5;
    int x = rightX;
    const int editButtonY = margin + 112;
    MoveWindow(GetDlgItem(state->hwnd, IDC_ADD), x, editButtonY, smallButtonWidth, buttonHeight, TRUE);
    x += smallButtonWidth + gap;
    MoveWindow(GetDlgItem(state->hwnd, IDC_UPDATE), x, editButtonY, smallButtonWidth, buttonHeight, TRUE);
    x += smallButtonWidth + gap;
    MoveWindow(GetDlgItem(state->hwnd, IDC_DELETE), x, editButtonY, smallButtonWidth, buttonHeight, TRUE);
    x += smallButtonWidth + gap;
    MoveWindow(GetDlgItem(state->hwnd, IDC_UP), x, editButtonY, smallButtonWidth, buttonHeight, TRUE);
    x += smallButtonWidth + gap;
    MoveWindow(GetDlgItem(state->hwnd, IDC_DOWN), x, editButtonY, smallButtonWidth, buttonHeight, TRUE);

    const int okWidth = 96;
    MoveWindow(GetDlgItem(state->hwnd, IDC_CANCEL), width - margin - okWidth, bottomY, okWidth, buttonHeight, TRUE);
    MoveWindow(GetDlgItem(state->hwnd, IDC_OK), width - margin - (okWidth * 2) - gap, bottomY, okWidth, buttonHeight, TRUE);
}

void FinishManagerDialog(ManagerState* state, bool save)
{
    if (save) {
        if (!SaveCustomSymbols(state->items)) {
            MessageBoxW(state->hwnd, Ui().saveFailedMessage, kAppName, MB_ICONERROR);
            return;
        }
        state->saved = true;
    }
    DestroyWindow(state->hwnd);
}

LRESULT CALLBACK ManagerWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ManagerState* state = reinterpret_cast<ManagerState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE: {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<ManagerState*>(create->lpCreateParams);
        state->hwnd = hwnd;
        state->font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

        state->list = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | LBS_NOTIFY, 0, 0, 0, 0, hwnd, ControlId(IDC_SYMBOL_LIST), g_instance, nullptr);
        HWND labelStatic = CreateWindowExW(0, L"STATIC", Ui().labelField, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, ControlId(4201), g_instance, nullptr);
        state->labelEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd, ControlId(IDC_LABEL_EDIT), g_instance, nullptr);
        HWND textStatic = CreateWindowExW(0, L"STATIC", Ui().textField, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, ControlId(4202), g_instance, nullptr);
        state->textEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd, ControlId(IDC_TEXT_EDIT), g_instance, nullptr);

        CreateWindowExW(0, L"BUTTON", Ui().add, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, hwnd, ControlId(IDC_ADD), g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", Ui().update, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, hwnd, ControlId(IDC_UPDATE), g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", Ui().remove, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, hwnd, ControlId(IDC_DELETE), g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", Ui().up, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, hwnd, ControlId(IDC_UP), g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", Ui().down, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, hwnd, ControlId(IDC_DOWN), g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", Ui().ok, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hwnd, ControlId(IDC_OK), g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", Ui().cancel, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, hwnd, ControlId(IDC_CANCEL), g_instance, nullptr);

        HWND children[] = {state->list, labelStatic, state->labelEdit, textStatic, state->textEdit,
            GetDlgItem(hwnd, IDC_ADD), GetDlgItem(hwnd, IDC_UPDATE), GetDlgItem(hwnd, IDC_DELETE),
            GetDlgItem(hwnd, IDC_UP), GetDlgItem(hwnd, IDC_DOWN), GetDlgItem(hwnd, IDC_OK), GetDlgItem(hwnd, IDC_CANCEL)};
        for (HWND child : children) {
            SetControlFont(child, state->font);
        }

        PopulateManagerList(state);
        UpdateManagerEditFields(state);
        LayoutManagerWindow(state);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = kManagerMinWidth;
        info->ptMinTrackSize.y = kManagerMinHeight;
        return 0;
    }
    case WM_SIZE:
        if (state) {
            LayoutManagerWindow(state);
        }
        return 0;
    case WM_COMMAND:
        if (!state) {
            break;
        }
        if (LOWORD(wParam) == IDC_SYMBOL_LIST && HIWORD(wParam) == LBN_SELCHANGE) {
            state->selected = static_cast<int>(SendMessageW(state->list, LB_GETCURSEL, 0, 0));
            UpdateManagerEditFields(state);
            return 0;
        }

        switch (LOWORD(wParam)) {
        case IDC_ADD:
            AddManagerEntry(state);
            return 0;
        case IDC_UPDATE:
            UpdateSelectedManagerEntry(state);
            return 0;
        case IDC_DELETE:
            DeleteSelectedManagerEntry(state);
            return 0;
        case IDC_UP:
            MoveSelectedManagerEntry(state, -1);
            return 0;
        case IDC_DOWN:
            MoveSelectedManagerEntry(state, 1);
            return 0;
        case IDC_OK:
            FinishManagerDialog(state, true);
            return 0;
        case IDC_CANCEL:
            FinishManagerDialog(state, false);
            return 0;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        if (state) {
            FinishManagerDialog(state, false);
        }
        return 0;
    case WM_NCDESTROY:
        delete state;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void RegisterManagerWindowClass()
{
    static bool registered = false;
    if (registered) {
        return;
    }

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = ManagerWindowProc;
    windowClass.hInstance = g_instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = g_icon;
    windowClass.hIconSm = g_icon;
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = kManagerClassName;
    RegisterClassExW(&windowClass);
    registered = true;
}

void CenterWindowOnOwner(HWND hwnd, HWND owner)
{
    RECT ownerRect = {};
    RECT windowRect = {};
    GetWindowRect(owner, &ownerRect);
    GetWindowRect(hwnd, &windowRect);

    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;
    SetWindowPos(hwnd, nullptr, std::max(0, x), std::max(0, y), 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void ShowSymbolManager()
{
    RegisterManagerWindowClass();

    ManagerState* state = new ManagerState();
    state->items = g_customSymbols;

    HWND dialog = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        kManagerClassName,
        Ui().managerTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        static_cast<int>(kManagerMinWidth),
        static_cast<int>(kManagerMinHeight),
        g_mainWindow,
        nullptr,
        g_instance,
        state);

    if (!dialog) {
        delete state;
        MessageBoxW(g_mainWindow, Ui().managerOpenFailedMessage, kAppName, MB_ICONERROR);
        return;
    }

    CenterWindowOnOwner(dialog, g_mainWindow);
    EnableWindow(g_mainWindow, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG message = {};
    while (IsWindow(dialog) && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    EnableWindow(g_mainWindow, TRUE);
    SetForegroundWindow(g_mainWindow);
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == g_taskbarCreatedMessage) {
        AddTrayIcon();
        return 0;
    }

    switch (message) {
    case WM_CREATE:
        g_mainWindow = hwnd;
        AddTrayIcon();
        return 0;
    case kTrayMessage:
        if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            ShowTrayMenu();
        }
        return 0;
    case WM_MEASUREITEM:
        if (wParam == 0) {
            MeasureSymbolMenuItem(reinterpret_cast<MEASUREITEMSTRUCT*>(lParam));
            return TRUE;
        }
        break;
    case WM_DRAWITEM:
        if (wParam == 0) {
            DrawSymbolMenuItem(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        break;
    case WM_COMMAND: {
        const UINT id = LOWORD(wParam);
        if (id >= kIdBuiltInBase && id < kIdBuiltInBase + kBuiltInSymbols.size()) {
            const BuiltInSymbol& symbol = kBuiltInSymbols[id - kIdBuiltInBase];
            InsertSymbol({BuiltInLabel(symbol), symbol.text});
            return 0;
        }
        if (id >= kIdCustomBase && id < kIdCustomBase + g_customSymbols.size()) {
            InsertSymbol(g_customSymbols[id - kIdCustomBase]);
            return 0;
        }

        switch (id) {
        case kIdManageSymbols:
            ShowSymbolManager();
            return 0;
        case kIdAbout:
            ShowAboutDialog();
            return 0;
        case kIdOpenConfigFolder:
            OpenConfigFolder();
            return 0;
        case kIdToggleAutostart:
            ToggleAutostart();
            return 0;
        case kIdExit:
            DestroyWindow(hwnd);
            return 0;
        default:
            break;
        }
        break;
    }
    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterMainWindowClass()
{
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = MainWindowProc;
    windowClass.hInstance = g_instance;
    windowClass.hIcon = g_icon;
    windowClass.hIconSm = g_icon;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"TraySymbolsHiddenWindow";
    return RegisterClassExW(&windowClass) != 0;
}

} // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
    g_instance = instance;

    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"TraySymbols.Native.SingleInstance");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        g_language = DetectLanguage();
        MessageBoxW(nullptr, Ui().alreadyRunningMessage, kAppName, MB_ICONINFORMATION);
        CloseHandle(mutex);
        return 0;
    }

    INITCOMMONCONTROLSEX controls = {};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    g_icon = LoadIconW(g_instance, MAKEINTRESOURCEW(IDI_APP));
    if (!g_icon) {
        g_icon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    g_language = DetectLanguage();
    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
    g_configPath = ResolveConfigPath();
    LoadCustomSymbols();

    if (!RegisterMainWindowClass()) {
        MessageBoxW(nullptr, Ui().startupFailedMessage, kAppName, MB_ICONERROR);
        if (mutex) {
            CloseHandle(mutex);
        }
        return 1;
    }

    g_mainWindow = CreateWindowExW(
        0,
        L"TraySymbolsHiddenWindow",
        kAppName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        g_instance,
        nullptr);

    if (!g_mainWindow) {
        MessageBoxW(nullptr, Ui().windowCreateFailedMessage, kAppName, MB_ICONERROR);
        if (mutex) {
            CloseHandle(mutex);
        }
        return 1;
    }

    ShowWindow(g_mainWindow, SW_HIDE);

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (mutex) {
        CloseHandle(mutex);
    }
    return static_cast<int>(message.wParam);
}
