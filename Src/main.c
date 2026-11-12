#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config.h"

/* Global State */
AppConfig g_cfg;
HINSTANCE g_hInst;
HWND g_hwndTaskbar = NULL;
HWND g_hwndRevealBar = NULL;
HWND g_hwndStartPanel = NULL;
HWND g_hwndPreview = NULL;
HWND g_hwndDesktop = NULL;
HWND g_hwndSettings = NULL;

bool g_startOpen = false;
bool g_tbTemporarilyShown = false;
DWORD g_tbHideTick = 0;
int g_hoveredRunningIdx = -1;
int g_activeSettingsTab = 0;

/* Running Windows Enumeration Cache */
typedef struct {
    HWND hwnd;
    char title[128];
    HICON hIcon;
    RECT btnRect;
} RunningWin;

#define MAX_RUNNING 32
RunningWin g_runningWins[MAX_RUNNING];
int g_runningCount = 0;

/* Theme Names */
const char* THEME_NAMES[THEME_COUNT] = {
    "None (Custom)", "Windows 98", "Windows 7", "Windows 8", "Windows 10",
    "Linux Ubuntu", "Linux Arch", "Linux Mint", "Linux XFCE", "Linux Cinnamon", "Linux KDE"
};

/* =========================================================================
 * 1. CONFIGURATION & THEME PRESETS
 * ========================================================================= */
void ApplyThemePreset(AppConfig *cfg, ThemeType theme, ThemeLookMode mode) {
    cfg->theme = theme;
    cfg->lookMode = mode;
    cfg->tbRoundCorners = (mode == MODE_MODERN);
    cfg->tbDetachGap = (mode == MODE_MODERN) ? 6 : 0;

    switch (theme) {
        case THEME_WIN98:
            cfg->tbColor = RGB(192, 192, 192);
            cfg->startPanelColor = RGB(192, 192, 192);
            cfg->vertBarColor = RGB(0, 0, 128);
            cfg->startBtnColor = RGB(192, 192, 192);
            cfg->accentColor = RGB(0, 0, 128);
            cfg->desktopBgColor = RGB(0, 128, 128);
            cfg->tbRoundCorners = false;
            cfg->tbDetachGap = 0;
            break;
        case THEME_WIN7:
            cfg->tbColor = RGB(28, 58, 94);
            cfg->startPanelColor = RGB(34, 68, 108);
            cfg->vertBarColor = RGB(20, 45, 75);
            cfg->startBtnColor = RGB(45, 140, 210);
            cfg->accentColor = RGB(70, 175, 245);
            cfg->desktopBgColor = RGB(18, 78, 138);
            cfg->effectType = 1; /* Glass */
            break;
        case THEME_WIN8:
            cfg->tbColor = RGB(18, 18, 22);
            cfg->startPanelColor = RGB(29, 0, 82);
            cfg->vertBarColor = RGB(45, 15, 110);
            cfg->startBtnColor = RGB(0, 120, 215);
            cfg->accentColor = RGB(0, 174, 239);
            cfg->desktopBgColor = RGB(0, 114, 198);
            break;
        case THEME_WIN10:
            cfg->tbColor = RGB(16, 16, 18);
            cfg->startPanelColor = RGB(24, 24, 27);
            cfg->vertBarColor = RGB(18, 18, 20);
            cfg->startBtnColor = RGB(0, 120, 215);
            cfg->accentColor = RGB(0, 120, 215);
            cfg->desktopBgColor = RGB(15, 45, 85);
            break;
        case THEME_UBUNTU:
            cfg->tbColor = RGB(44, 0, 30);
            cfg->startPanelColor = RGB(58, 12, 42);
            cfg->vertBarColor = RGB(35, 0, 24);
            cfg->startBtnColor = RGB(233, 84, 32);
            cfg->accentColor = RGB(233, 84, 32);
            cfg->desktopBgColor = RGB(82, 20, 58);
            break;
        case THEME_ARCH:
            cfg->tbColor = RGB(22, 27, 34);
            cfg->startPanelColor = RGB(13, 17, 23);
            cfg->vertBarColor = RGB(22, 27, 34);
            cfg->startBtnColor = RGB(23, 147, 209);
            cfg->accentColor = RGB(23, 147, 209);
            cfg->desktopBgColor = RGB(18, 22, 28);
            break;
        case THEME_MINT:
            cfg->tbColor = RGB(36, 42, 36);
            cfg->startPanelColor = RGB(45, 52, 45);
            cfg->vertBarColor = RGB(30, 35, 30);
            cfg->startBtnColor = RGB(135, 207, 62);
            cfg->accentColor = RGB(135, 207, 62);
            cfg->desktopBgColor = RGB(42, 60, 45);
            break;
        case THEME_XFCE:
            cfg->tbColor = RGB(38, 45, 52);
            cfg->startPanelColor = RGB(48, 56, 65);
            cfg->vertBarColor = RGB(30, 36, 42);
            cfg->startBtnColor = RGB(0, 168, 224);
            cfg->accentColor = RGB(0, 168, 224);
            cfg->desktopBgColor = RGB(34, 65, 92);
            break;
        case THEME_CINNAMON:
            cfg->tbColor = RGB(35, 35, 35);
            cfg->startPanelColor = RGB(45, 45, 45);
            cfg->vertBarColor = RGB(28, 28, 28);
            cfg->startBtnColor = RGB(218, 92, 42);
            cfg->accentColor = RGB(218, 92, 42);
            cfg->desktopBgColor = RGB(48, 42, 38);
            break;
        case THEME_KDE:
            cfg->tbColor = RGB(27, 35, 44);
            cfg->startPanelColor = RGB(35, 45, 56);
            cfg->vertBarColor = RGB(20, 27, 34);
            cfg->startBtnColor = RGB(61, 174, 233);
            cfg->accentColor = RGB(61, 174, 233);
            cfg->desktopBgColor = RGB(25, 55, 88);
            break;
        default:
            break;
    }

    /* Apply Color Mode Override if selected */
    if (cfg->colorMode == COLOR_MODE_AMOLED) {
        cfg->tbColor = RGB(0, 0, 0);
        cfg->startPanelColor = RGB(5, 5, 5);
    } else if (cfg->colorMode == COLOR_MODE_ROSE) {
        cfg->tbColor = RGB(72, 22, 42);
        cfg->accentColor = RGB(245, 95, 145);
    } else if (cfg->colorMode == COLOR_MODE_SAPPHIRE) {
        cfg->tbColor = RGB(12, 28, 68);
        cfg->accentColor = RGB(65, 145, 255);
    }
}

void LoadDefaultConfig(AppConfig *cfg) {
    memset(cfg, 0, sizeof(AppConfig));
    cfg->theme = THEME_WIN10;
    cfg->lookMode = MODE_MODERN;
    cfg->colorMode = COLOR_MODE_NONE;
    cfg->effectType = 1;
    cfg->blurIntensity = 65;
    cfg->glassShine = 50;
    cfg->glassGloss = 40;
    cfg->dropShadow = true;
    cfg->roundAllCorners = true;
    cfg->effectOnTaskbar = true;
    cfg->effectOnStartPanel = true;
    cfg->effectOnPreview = true;
    cfg->effectOnTitleBar = true;
    cfg->effectOnInstalledApps = false;
    cfg->iconPresetIndex = 0;
    strcpy(cfg->fontName, "Segoe UI");
    cfg->fontSize = 10;
    cfg->uiScale = 100;

    cfg->tbPosition = POS_BOTTOM;
    cfg->tbAlignment = ALIGN_LEFT;
    cfg->tbHeight = 44;
    cfg->tbAutoWidth = false;
    cfg->tbIconSize = 24;
    cfg->tbShowWindowTitle = true;
    cfg->tbRoundCorners = true;
    cfg->tbDetachGap = 0;
    cfg->tbRunUnderline = true;
    cfg->tbUnderlineHeight = 3;
    cfg->tbDividerStyle = 1;
    cfg->tbShowDayInClock = true;

    cfg->hideWineTaskbar = true;
    cfg->underlineAdaptIconColor = true;
    cfg->fullscreenMode = 0;
    cfg->fullscreenAppliesTo = 2;
    cfg->autoStartWithContainer = true;
    cfg->startupDelaySec = 1;
    cfg->showHoverPreview = true;
    cfg->previewWidth = 200;
    cfg->previewHeight = 120;

    cfg->revealBarEnabled = true;
    cfg->revealBarColor = RGB(0, 174, 239);
    cfg->revealBarPos = 0;
    cfg->revealBarAlign = ALIGN_MIDDLE;
    cfg->revealBarLCorner = false;
    cfg->revealBarOpacity = 180;
    cfg->revealBarHeight = 5;
    cfg->revealBarWidth = 260;
    cfg->tbVisibleSeconds = 4;
    cfg->revealAnimation = 1;

    cfg->startPanelSize = 1;
    strcpy(cfg->userName, "Winlator User");
    cfg->vertBarTransparent = false;
    cfg->startPanelTransparent = true;
    cfg->startPanelAlpha = 235;
    cfg->pinnedRandomColors = true;
    cfg->pinnedTileAlpha = 220;
    cfg->pinnedSlideAnim = true;
    cfg->pinnedTileSize = 72;

    cfg->customCursorEnabled = false;
    cfg->cursorColor = RGB(0, 210, 255);
    cfg->cursorBorderColor = RGB(255, 255, 255);
    cfg->cursorBorderWidth = 2;

    cfg->kbAutoOpenEnabled = true;
    cfg->kbTriggerOnIBeam = true;
    cfg->kbTriggerOnCaretBlink = true;
    cfg->kbTriggerOnOfficeClick = true;
    strcpy(cfg->kbBridgeCommand, "explorer.exe /e,keyboard");

    cfg->skinWindowsEnabled = true;
    cfg->windowTransparency = 245;
    cfg->skinTargetMode = 0;

    cfg->desktopModeEnabled = true;
    cfg->desktopGridSize = 84;
    cfg->desktopSnapToGrid = true;
    cfg->desktopIconCorner = 0;
    cfg->showDesktopIcons = true;
    cfg->showClockWidget = true;
    cfg->showAppWidget = true;
    cfg->wallpaperFit = 2;
    cfg->lockDesktopIcons = false;
    strcpy(cfg->desktopFont, "Segoe UI");
    cfg->desktopFontSize = 10;
    cfg->desktopUiScale = 100;

    /* Default Built-in Shortcuts for Winlator Wine Container */
    cfg->appCount = 6;
    const char* defNames[6] = {"File Explorer", "Command Prompt", "Control Panel", "Registry Editor", "Task Manager", "Notepad"};
    const char* defPaths[6] = {"explorer.exe", "cmd.exe", "control.exe", "regedit.exe", "taskmgr.exe", "notepad.exe"};
    COLORREF defCols[6] = {RGB(215, 145, 0), RGB(45, 45, 55), RGB(0, 120, 215), RGB(0, 150, 136), RGB(46, 125, 50), RGB(103, 58, 183)};

    for (int i = 0; i < 6; i++) {
        strcpy(cfg->apps[i].name, defNames[i]);
        strcpy(cfg->apps[i].execPath, defPaths[i]);
        cfg->apps[i].tileColor = defCols[i];
        cfg->apps[i].tileW = 1;
        cfg->apps[i].tileH = 1;
        cfg->apps[i].gridX = 0;
        cfg->apps[i].gridY = i;
        cfg->apps[i].inStartList = true;
        cfg->apps[i].inPinnedTiles = true;
        cfg->apps[i].inTaskbar = (i < 4);
        cfg->apps[i].inAppTray = (i >= 4);
        cfg->apps[i].onDesktop = true;
    }

    ApplyThemePreset(cfg, THEME_WIN10, MODE_MODERN);
}

bool SaveConfigToFile(const AppConfig *cfg, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return false;
    fwrite(cfg, sizeof(AppConfig), 1, fp);
    fclose(fp);
    return true;
}

bool LoadConfigFromFile(AppConfig *cfg, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return false;
    size_t r = fread(cfg, sizeof(AppConfig), 1, fp);
    fclose(fp);
    return (r == 1);
}

/* =========================================================================
 * 2. WINE & WINLATOR SYSTEM INTEGRATION
 * ========================================================================= */
void ApplyWineTaskbarVisibility(void) {
    HWND hWineTray = FindWindowA("Shell_TrayWnd", NULL);
    if (hWineTray) {
        ShowWindow(hWineTray, g_cfg.hideWineTaskbar ? SW_HIDE : SW_SHOW);
    }
}

void UpdateAutoStartRegistry(void) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (g_cfg.autoStartWithContainer) {
            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);
            RegSetValueExA(hKey, "WinlatorShellPro", 0, REG_SZ, (const BYTE*)exePath, (DWORD)strlen(exePath) + 1);
        } else {
            RegDeleteValueA(hKey, "WinlatorShellPro");
        }
        RegCloseKey(hKey);
    }
}

/* Detects I-Beam cursor or active text Caret inside Winlator apps to trigger Android Keyboard */
void CheckAndTriggerAndroidKeyboard(void) {
    if (!g_cfg.kbAutoOpenEnabled) return;
    static DWORD lastTrigger = 0;
    if (GetTickCount() - lastTrigger < 2500) return;

    bool shouldOpen = false;

    /* Check 1: Cursor is IDC_IBEAM ("I" shape) + Left Click pressed */
    if (g_cfg.kbTriggerOnIBeam && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        CURSORINFO ci = { sizeof(CURSORINFO) };
        if (GetCursorInfo(&ci) && ci.hCursor == LoadCursor(NULL, IDC_IBEAM)) {
            shouldOpen = true;
        }
    }

    /* Check 2: Blinking Caret active in foreground thread */
    if (!shouldOpen && g_cfg.kbTriggerOnCaretBlink) {
        HWND fg = GetForegroundWindow();
        if (fg && fg != g_hwndTaskbar && fg != g_hwndStartPanel && fg != g_hwndSettings) {
            DWORD tid = GetWindowThreadProcessId(fg, NULL);
            GUITHREADINFO gti = { sizeof(GUITHREADINFO) };
            if (GetGUIThreadInfo(tid, &gti) && gti.hwndCaret != NULL) {
                if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) shouldOpen = true;
            }
        }
    }

    if (shouldOpen) {
        lastTrigger = GetTickCount();
        /* Send F12/Virtual Keyboard trigger used by Winlator input bridge */
        keybd_event(VK_F24, 0, 0, 0);
        keybd_event(VK_F24, 0, KEYEVENTF_KEYUP, 0);
    }
}

/* Enumerates active top-level windows for Taskbar & applies Window Transparency / Skinning */
BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    if (hwnd == g_hwndTaskbar || hwnd == g_hwndRevealBar ||
        hwnd == g_hwndStartPanel || hwnd == g_hwndPreview ||
        hwnd == g_hwndDesktop) return TRUE;

    LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return TRUE;

    char title[128] = {0};
    GetWindowTextA(hwnd, title, sizeof(title) - 1);
    if (strlen(title) == 0 || strcmp(title, "Program Manager") == 0) return TRUE;

    /* Apply Window Skinning / Transparency if enabled */
    if (g_cfg.skinWindowsEnabled && g_cfg.windowTransparency < 255) {
        SetWindowLongA(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwnd, 0, (BYTE)g_cfg.windowTransparency, LWA_ALPHA);
    }

    if (g_runningCount < MAX_RUNNING) {
        g_runningWins[g_runningCount].hwnd = hwnd;
        strncpy(g_runningWins[g_runningCount].title, title, 127);
        HICON hIco = (HICON)SendMessageA(hwnd, WM_GETICON, ICON_SMALL, 0);
        if (!hIco) hIco = (HICON)GetClassLongPtrA(hwnd, GCLP_HICONSM);
        if (!hIco) hIco = LoadIcon(NULL, IDI_APPLICATION);
        g_runningWins[g_runningCount].hIcon = hIco;
        g_runningCount++;
    }
    return TRUE;
}

void RefreshRunningWindows(void) {
    g_runningCount = 0;
    EnumWindows(EnumWindowsCallback, 0);
}

/* =========================================================================
 * 3. HELPER DRAWING ROUTINES (GDI)
 * ========================================================================= */
void FillRoundRectSolid(HDC hdc, RECT rc, int radius, COLORREF col, COLORREF borderCol, bool drawBorder) {
    HBRUSH hBrush = CreateSolidBrush(col);
    HPEN hPen = drawBorder ? CreatePen(PS_SOLID, 1, borderCol) : (HPEN)GetStockObject(NULL_PEN);
    HGDIOBJ oldB = SelectObject(hdc, hBrush);
    HGDIOBJ oldP = SelectObject(hdc, hPen);
    if (radius > 0) {
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    } else {
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    }
    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);
    DeleteObject(hBrush);
    if (drawBorder) DeleteObject(hPen);
}

/* Draws a Modern Pill-Shaped On/Off Toggle Switch */
void DrawModernToggle(HDC hdc, int x, int y, bool state) {
    RECT rc = { x, y, x + 46, y + 22 };
    COLORREF trackCol = state ? g_cfg.accentColor : RGB(90, 95, 105);
    FillRoundRectSolid(hdc, rc, 20, trackCol, RGB(140, 140, 140), false);

    int knobX = state ? (x + 25) : (x + 3);
    RECT knob = { knobX, y + 3, knobX + 16, y + 19 };
    FillRoundRectSolid(hdc, knob, 16, RGB(255, 255, 255), 0, false);
}

/* =========================================================================
 * 4. TASKBAR POSITIONING & RENDERING
 * ========================================================================= */
void RepositionShellWindows(void) {
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int gap = g_cfg.tbDetachGap;
    int h = g_cfg.tbHeight;

    int tbX = gap, tbY = scrH - h - gap, tbW = scrW - (gap * 2), tbH = h;

    if (g_cfg.tbAutoWidth) {
        int calcW = 280 + (g_runningCount * (g_cfg.tbShowWindowTitle ? 140 : 48));
        if (calcW < scrW - 40) tbW = calcW;
        if (g_cfg.tbAlignment == ALIGN_MIDDLE) tbX = (scrW - tbW) / 2;
        else if (g_cfg.tbAlignment == ALIGN_RIGHT) tbX = scrW - tbW - gap;
    }

    if (g_cfg.tbPosition == POS_TOP) {
        tbY = gap;
    } else if (g_cfg.tbPosition == POS_LEFT) {
        tbX = gap; tbY = gap; tbW = h * 2; tbH = scrH - (gap * 2);
    } else if (g_cfg.tbPosition == POS_RIGHT) {
        tbX = scrW - (h * 2) - gap; tbY = gap; tbW = h * 2; tbH = scrH - (gap * 2);
    }

    if (g_hwndTaskbar) {
        SetWindowPos(g_hwndTaskbar, HWND_TOPMOST, tbX, tbY, tbW, tbH, SWP_NOACTIVATE);
        if (g_cfg.tbRoundCorners && g_cfg.lookMode == MODE_MODERN) {
            HRGN hRgn = CreateRoundRectRgn(0, 0, tbW, tbH, 14, 14);
            SetWindowRgn(g_hwndTaskbar, hRgn, TRUE);
        } else {
            SetWindowRgn(g_hwndTaskbar, NULL, TRUE);
        }
        InvalidateRect(g_hwndTaskbar, NULL, TRUE);
    }

    /* Position Reveal Bar */
    if (g_hwndRevealBar) {
        if (g_cfg.revealBarEnabled) {
            int rw = (g_cfg.revealBarWidth > scrW) ? scrW : g_cfg.revealBarWidth;
            int rh = g_cfg.revealBarHeight;
            int rx = (scrW - rw) / 2;
            if (g_cfg.revealBarAlign == ALIGN_LEFT) rx = 0;
            else if (g_cfg.revealBarAlign == ALIGN_RIGHT) rx = scrW - rw;
            int ry = (g_cfg.revealBarPos == 0) ? (scrH - rh) : 0;

            SetWindowPos(g_hwndRevealBar, HWND_TOPMOST, rx, ry, rw, rh, SWP_NOACTIVATE | SWP_SHOWWINDOW);
            SetLayeredWindowAttributes(g_hwndRevealBar, 0, (BYTE)g_cfg.revealBarOpacity, LWA_ALPHA);
        } else {
            ShowWindow(g_hwndRevealBar, SW_HIDE);
        }
    }

    /* Position Start Panel */
    if (g_hwndStartPanel) {
        int spW = 520, spH = 440;
        if (g_cfg.startPanelSize == 0) { spW = 420; spH = 360; }
        else if (g_cfg.startPanelSize == 2) { spW = 680; spH = 520; }
        else if (g_cfg.startPanelSize == 3) { spW = scrW; spH = scrH - h; }

        int spX = tbX;
        /* Crucial requirement: If Right-aligned taskbar, Start Panel anchors on Right corner */
        if (g_cfg.tbAlignment == ALIGN_RIGHT || g_cfg.tbPosition == POS_RIGHT) {
            spX = scrW - spW - gap;
        } else if (g_cfg.tbAlignment == ALIGN_MIDDLE) {
            spX = (scrW - spW) / 2;
        }
        int spY = (g_cfg.tbPosition == POS_TOP) ? (tbY + tbH + 4) : (tbY - spH - 4);
        if (spY < 0) spY = 0;

        SetWindowPos(g_hwndStartPanel, HWND_TOPMOST, spX, spY, spW, spH, SWP_NOACTIVATE);
        SetLayeredWindowAttributes(g_hwndStartPanel, 0, (BYTE)g_cfg.startPanelAlpha, LWA_ALPHA);
    }
}

/* Forward declaration of Settings window creator */
void OpenSettingsWindow(void);

LRESULT CALLBACK TaskbarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static RECT rcStartBtn, rcShowDesk, rcSettingsBtn;
    static RECT rcPinned[MAX_APPS];

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            /* Double Buffer */
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBmp);

            /* Background */
            bool isClassic98 = (g_cfg.theme == THEME_WIN98 && g_cfg.lookMode == MODE_CLASSIC_OG);
            FillRoundRectSolid(memDC, rc, (g_cfg.tbRoundCorners && !isClassic98) ? 12 : 0,
                               g_cfg.tbColor, g_cfg.accentColor, g_cfg.effectOnTaskbar);

            /* Classic Win98 3D highlight line */
            if (isClassic98) {
                DrawEdge(memDC, &rc, EDGE_RAISED, BF_RECT);
            }

            SetBkMode(memDC, TRANSPARENT);
            HFONT hFont = CreateFontA(-(g_cfg.fontSize + 3), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
                                      DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, g_cfg.fontName);
            SelectObject(memDC, hFont);
            COLORREF txtCol = (g_cfg.theme == THEME_WIN98 && g_cfg.lookMode == MODE_CLASSIC_OG) ? RGB(0, 0, 0) : RGB(245, 245, 245);
            SetTextColor(memDC, txtCol);

            bool rightAligned = (g_cfg.tbAlignment == ALIGN_RIGHT);

            /*
             * Crucial Alignment Requirement from specification:
             * When Alignment == RIGHT, Show Desktop, Time/Date, and Preview buttons move to LEFT side,
             * and Start Button moves to the RIGHT corner!
             */
            int curLeft = 6;
            int curRight = rc.right - 6;

            if (!rightAligned) {
                /* Standard Left/Middle Layout: Start Button on Left, Clock & Show Desktop on Right */
                SetRect(&rcStartBtn, curLeft, 4, curLeft + 78, rc.bottom - 4);
                FillRoundRectSolid(memDC, rcStartBtn, isClassic98 ? 0 : 8, g_cfg.startBtnColor, RGB(255,255,255), isClassic98);
                DrawTextA(memDC, (g_cfg.theme == THEME_UBUNTU) ? "Ubuntu" : "Start", -1, &rcStartBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                curLeft += 84;

                /* Quick Settings Gear Button */
                SetRect(&rcSettingsBtn, curLeft, 4, curLeft + 34, rc.bottom - 4);
                FillRoundRectSolid(memDC, rcSettingsBtn, 6, RGB(55, 60, 70), g_cfg.accentColor, true);
                DrawTextA(memDC, "Cfg", -1, &rcSettingsBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                curLeft += 40;

                /* Right side: Show Desktop slice */
                SetRect(&rcShowDesk, curRight - 14, 2, curRight, rc.bottom - 2);
                FillRoundRectSolid(memDC, rcShowDesk, 2, g_cfg.accentColor, RGB(255,255,255), true);
                curRight -= 18;

                /* Clock & Date */
                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char timeBuf[64];
                if (g_cfg.tbShowDayInClock)
                    strftime(timeBuf, sizeof(timeBuf), "%H:%M %a %d/%m", t);
                else
                    strftime(timeBuf, sizeof(timeBuf), "%H:%M  %d/%m", t);

                RECT rcClock = { curRight - 115, 2, curRight, rc.bottom - 2 };
                DrawTextA(memDC, timeBuf, -1, &rcClock, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                curRight -= 120;

                /* System App Tray Icons */
                for (int i = 0; i < g_cfg.appCount; i++) {
                    if (!g_cfg.apps[i].inAppTray) continue;
                    RECT rcTray = { curRight - 26, (rc.bottom - 22)/2, curRight - 4, (rc.bottom + 22)/2 };
                    FillRoundRectSolid(memDC, rcTray, 4, g_cfg.apps[i].tileColor, 0, false);
                    DrawTextA(memDC, g_cfg.apps[i].name, 1, &rcTray, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    curRight -= 28;
                }
            } else {
                /* RIGHT-ALIGNED FLIPPED LAYOUT:
                 * Show Desktop + Date/Time on the far LEFT; Start Button on the far RIGHT */
                SetRect(&rcShowDesk, curLeft, 2, curLeft + 14, rc.bottom - 2);
                FillRoundRectSolid(memDC, rcShowDesk, 2, g_cfg.accentColor, RGB(255,255,255), true);
                curLeft += 18;

                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char timeBuf[64];
                strftime(timeBuf, sizeof(timeBuf), g_cfg.tbShowDayInClock ? "%H:%M %a %d/%m" : "%H:%M %d/%m", t);
                RECT rcClock = { curLeft, 2, curLeft + 115, rc.bottom - 2 };
                DrawTextA(memDC, timeBuf, -1, &rcClock, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                curLeft += 120;

                /* Start Button on Far Right */
                SetRect(&rcStartBtn, curRight - 78, 4, curRight, rc.bottom - 4);
                FillRoundRectSolid(memDC, rcStartBtn, isClassic98 ? 0 : 8, g_cfg.startBtnColor, RGB(255,255,255), isClassic98);
                DrawTextA(memDC, "Start", -1, &rcStartBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                curRight -= 84;

                SetRect(&rcSettingsBtn, curRight - 34, 4, curRight, rc.bottom - 4);
                FillRoundRectSolid(memDC, rcSettingsBtn, 6, RGB(55, 60, 70), g_cfg.accentColor, true);
                DrawTextA(memDC, "Cfg", -1, &rcSettingsBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                curRight -= 40;
            }

            /* Pinned Taskbar Shortcuts */
            for (int i = 0; i < g_cfg.appCount; i++) {
                SetRectEmpty(&rcPinned[i]);
                if (!g_cfg.apps[i].inTaskbar) continue;
                if (curLeft + 38 > curRight) break;
                SetRect(&rcPinned[i], curLeft, 6, curLeft + 34, rc.bottom - 6);
                FillRoundRectSolid(memDC, rcPinned[i], 6, g_cfg.apps[i].tileColor, RGB(220,220,220), false);
                DrawTextA(memDC, g_cfg.apps[i].name, 2, &rcPinned[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                curLeft += 38;
            }

            /* Customizable Divider Line between Shortcuts & Running Applications (Thin / Mid / Bold) */
            int divThick = (g_cfg.tbDividerStyle == 0) ? 1 : (g_cfg.tbDividerStyle == 1 ? 2 : 4);
            RECT rcDiv = { curLeft + 2, 6, curLeft + 2 + divThick, rc.bottom - 6 };
            FillRoundRectSolid(memDC, rcDiv, 0, g_cfg.accentColor, 0, false);
            curLeft += divThick + 8;

            /* Running Applications */
            int btnW = g_cfg.tbShowWindowTitle ? 135 : 42;
            for (int i = 0; i < g_runningCount; i++) {
                if (curLeft + btnW > curRight) break;
                RECT rcRun = { curLeft, 5, curLeft + btnW, rc.bottom - 5 };
                g_runningWins[i].btnRect = rcRun;

                COLORREF runBg = (i == g_hoveredRunningIdx) ? RGB(75, 85, 105) : RGB(45, 50, 60);
                FillRoundRectSolid(memDC, rcRun, isClassic98 ? 0 : 6, runBg, RGB(120,120,120), isClassic98);

                if (g_runningWins[i].hIcon) {
                    DrawIconEx(memDC, rcRun.left + 6, (rc.bottom - g_cfg.tbIconSize)/2,
                               g_runningWins[i].hIcon, g_cfg.tbIconSize, g_cfg.tbIconSize, 0, NULL, DI_NORMAL);
                }

                if (g_cfg.tbShowWindowTitle) {
                    RECT rcTxt = { rcRun.left + g_cfg.tbIconSize + 10, rcRun.top, rcRun.right - 4, rcRun.bottom };
                    DrawTextA(memDC, g_runningWins[i].title, -1, &rcTxt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                }

                /* Running Application Underline Indicator */
                if (g_cfg.tbRunUnderline) {
                    COLORREF ulCol = g_cfg.underlineAdaptIconColor ? RGB(0, 215, 255) : g_cfg.accentColor;
                    RECT rcUl = { rcRun.left + 6, rcRun.bottom - g_cfg.tbUnderlineHeight, rcRun.right - 6, rcRun.bottom };
                    FillRoundRectSolid(memDC, rcUl, 2, ulCol, 0, false);
                }
                curLeft += btnW + 6;
            }

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(hFont);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            int newHover = -1;
            for (int i = 0; i < g_runningCount; i++) {
                if (PtInRect(&g_runningWins[i].btnRect, pt)) {
                    newHover = i;
                    break;
                }
            }
            if (newHover != g_hoveredRunningIdx) {
                g_hoveredRunningIdx = newHover;
                InvalidateRect(hwnd, NULL, FALSE);

                if (g_hoveredRunningIdx >= 0 && g_cfg.showHoverPreview) {
                    RECT rcTb;
                    GetWindowRect(hwnd, &rcTb);
                    int px = rcTb.left + g_runningWins[g_hoveredRunningIdx].btnRect.left;
                    int py = (g_cfg.tbPosition == POS_TOP) ? (rcTb.bottom + 6) : (rcTb.top - g_cfg.previewHeight - 6);
                    SetWindowPos(g_hwndPreview, HWND_TOPMOST, px, py, g_cfg.previewWidth, g_cfg.previewHeight, SWP_NOACTIVATE | SWP_SHOWWINDOW);
                    InvalidateRect(g_hwndPreview, NULL, TRUE);
                } else {
                    ShowWindow(g_hwndPreview, SW_HIDE);
                }
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (PtInRect(&rcStartBtn, pt)) {
                g_startOpen = !g_startOpen;
                ShowWindow(g_hwndStartPanel, g_startOpen ? SW_SHOWNOACTIVATE : SW_HIDE);
                InvalidateRect(g_hwndStartPanel, NULL, TRUE);
                return 0;
            }
            if (PtInRect(&rcSettingsBtn, pt)) {
                OpenSettingsWindow();
                return 0;
            }
            if (PtInRect(&rcShowDesk, pt)) {
                /* Toggle minimize all running windows */
                for (int i = 0; i < g_runningCount; i++) {
                    ShowWindow(g_runningWins[i].hwnd, SW_MINIMIZE);
                }
                return 0;
            }
            for (int i = 0; i < g_cfg.appCount; i++) {
                if (g_cfg.apps[i].inTaskbar && PtInRect(&rcPinned[i], pt)) {
                    ShellExecuteA(NULL, "open", g_cfg.apps[i].execPath, NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                }
            }
            for (int i = 0; i < g_runningCount; i++) {
                if (PtInRect(&g_runningWins[i].btnRect, pt)) {
                    HWND target = g_runningWins[i].hwnd;
                    if (IsIconic(target)) ShowWindow(target, SW_RESTORE);
                    SetForegroundWindow(target);
                    return 0;
                }
            }
            return 0;
        }

        case WM_RBUTTONUP:
            OpenSettingsWindow();
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* =========================================================================
 * 5. LIVE PREVIEW WINDOW & REVEAL BAR WNDPROC
 * ========================================================================= */
LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRoundRectSolid(hdc, rc, 8, RGB(28, 32, 40), g_cfg.accentColor, true);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        if (g_hoveredRunningIdx >= 0 && g_hoveredRunningIdx < g_runningCount) {
            RECT rcTitle = { 8, 6, rc.right - 8, 28 };
            DrawTextA(hdc, g_runningWins[g_hoveredRunningIdx].title, -1, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
            RECT rcThumb = { 10, 32, rc.right - 10, rc.bottom - 10 };
            FillRoundRectSolid(hdc, rcThumb, 4, RGB(45, 52, 66), RGB(90, 95, 110), true);
            DrawIconEx(hdc, (rc.right - 32)/2, (rc.bottom - 16)/2, g_runningWins[g_hoveredRunningIdx].hIcon, 32, 32, 0, NULL, DI_NORMAL);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK RevealBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRoundRectSolid(hdc, rc, g_cfg.revealBarLCorner ? 0 : 4, g_cfg.revealBarColor, RGB(255,255,255), false);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_MOUSEMOVE) {
        /* Reveal Taskbar when cursor touches the Reveal Bar */
        ShowWindow(g_hwndTaskbar, SW_SHOWNOACTIVATE);
        SetWindowPos(g_hwndTaskbar, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        g_tbTemporarilyShown = true;
        g_tbHideTick = GetTickCount() + (g_cfg.tbVisibleSeconds * 1000);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* =========================================================================
 * 6. START PANEL WINDOW (PROFILE, SEARCH, APP LIST, PINNED TILES, SIDEBAR)
 * ========================================================================= */
LRESULT CALLBACK StartPanelWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static RECT rcSideSettings, rcSidePower, rcSideRun, rcSideCollapseList, rcSideCollapseTiles, rcAddAppBtn;
    static RECT rcAppListItems[MAX_APPS], rcTileItems[MAX_APPS];

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            FillRoundRectSolid(hdc, rc, g_cfg.roundAllCorners ? 12 : 0, g_cfg.startPanelColor, g_cfg.accentColor, true);

            /* 1. Vertical Bar on Left */
            RECT rcVert = { 2, 2, 48, rc.bottom - 2 };
            if (!g_cfg.vertBarTransparent) {
                FillRoundRectSolid(hdc, rcVert, g_cfg.roundAllCorners ? 10 : 0, g_cfg.vertBarColor, 0, false);
            }

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(245, 245, 245));
            HFONT hFont = CreateFontA(-13, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, g_cfg.fontName);
            SelectObject(hdc, hFont);

            /* Sidebar Buttons: Collapse List, Collapse Tiles, Run, Settings, Power */
            SetRect(&rcSideCollapseList, 6, 56, 44, 88);
            FillRoundRectSolid(hdc, rcSideCollapseList, 6, RGB(60, 65, 75), 0, false);
            DrawTextA(hdc, "List", -1, &rcSideCollapseList, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcSideCollapseTiles, 6, 94, 44, 126);
            FillRoundRectSolid(hdc, rcSideCollapseTiles, 6, RGB(60, 65, 75), 0, false);
            DrawTextA(hdc, "Tile", -1, &rcSideCollapseTiles, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcSideRun, 6, rc.bottom - 118, 44, rc.bottom - 84);
            FillRoundRectSolid(hdc, rcSideRun, 6, RGB(50, 90, 130), 0, false);
            DrawTextA(hdc, "Run", -1, &rcSideRun, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcSideSettings, 6, rc.bottom - 78, 44, rc.bottom - 44);
            FillRoundRectSolid(hdc, rcSideSettings, 6, g_cfg.accentColor, 0, false);
            DrawTextA(hdc, "Set", -1, &rcSideSettings, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcSidePower, 6, rc.bottom - 38, 44, rc.bottom - 6);
            FillRoundRectSolid(hdc, rcSidePower, 6, RGB(190, 45, 45), 0, false);
            DrawTextA(hdc, "Off", -1, &rcSidePower, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            /* 2. Top Profile Header & Search + "+" Add App Button */
            RECT rcAvatar = { 58, 10, 92, 44 };
            FillRoundRectSolid(hdc, rcAvatar, 17, g_cfg.accentColor, RGB(255,255,255), true);
            DrawTextA(hdc, "U", -1, &rcAvatar, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT rcUser = { 100, 12, 260, 42 };
            DrawTextA(hdc, g_cfg.userName, -1, &rcUser, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcAddAppBtn, rc.right - 110, 10, rc.right - 12, 42);
            FillRoundRectSolid(hdc, rcAddAppBtn, 6, g_cfg.accentColor, RGB(255,255,255), true);
            DrawTextA(hdc, "+ Add App", -1, &rcAddAppBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            /* 3. Start App List (if not collapsed) */
            int contentLeft = 58;
            if (!g_cfg.collapseAppList) {
                int y = 56;
                for (int i = 0; i < g_cfg.appCount; i++) {
                    SetRectEmpty(&rcAppListItems[i]);
                    if (!g_cfg.apps[i].inStartList) continue;
                    if (y + 32 > rc.bottom - 10) break;
                    SetRect(&rcAppListItems[i], contentLeft, y, contentLeft + 175, y + 28);
                    FillRoundRectSolid(hdc, rcAppListItems[i], 5, RGB(42, 46, 54), 0, false);
                    RECT txtRc = rcAppListItems[i];
                    txtRc.left += 10;
                    DrawTextA(hdc, g_cfg.apps[i].name, -1, &txtRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                    y += 32;
                }
                contentLeft += 188;
            }

            /* 4. Pinned Resizable Tiles Grid (if not collapsed) */
            if (!g_cfg.collapsePinnedTiles) {
                int tileSz = g_cfg.pinnedTileSize;
                int tx = contentLeft, ty = 56;
                for (int i = 0; i < g_cfg.appCount; i++) {
                    SetRectEmpty(&rcTileItems[i]);
                    if (!g_cfg.apps[i].inPinnedTiles) continue;
                    if (tx + tileSz > rc.right - 12) {
                        tx = contentLeft;
                        ty += tileSz + 8;
                    }
                    if (ty + tileSz > rc.bottom - 12) break;

                    SetRect(&rcTileItems[i], tx, ty, tx + tileSz, ty + tileSz);
                    COLORREF tCol = g_cfg.pinnedRandomColors ? g_cfg.apps[i].tileColor : g_cfg.accentColor;
                    FillRoundRectSolid(hdc, rcTileItems[i], g_cfg.roundAllCorners ? 8 : 0, tCol, RGB(220,220,220), true);
                    DrawTextA(hdc, g_cfg.apps[i].name, -1, &rcTileItems[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                    tx += tileSz + 8;
                }
            }

            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (PtInRect(&rcSideCollapseList, pt)) {
                g_cfg.collapseAppList = !g_cfg.collapseAppList;
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
            if (PtInRect(&rcSideCollapseTiles, pt)) {
                g_cfg.collapsePinnedTiles = !g_cfg.collapsePinnedTiles;
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
            if (PtInRect(&rcSideSettings, pt) || PtInRect(&rcAddAppBtn, pt)) {
                g_startOpen = false;
                ShowWindow(hwnd, SW_HIDE);
                OpenSettingsWindow();
                return 0;
            }
            if (PtInRect(&rcSideRun, pt)) {
                ShellExecuteA(NULL, "open", "cmd.exe", NULL, NULL, SW_SHOWNORMAL);
                return 0;
            }
            if (PtInRect(&rcSidePower, pt)) {
                if (MessageBoxA(hwnd, "Exit Winlator Custom Shell and restore Wine Taskbar?", "Power", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    HWND hWineTray = FindWindowA("Shell_TrayWnd", NULL);
                    if (hWineTray) ShowWindow(hWineTray, SW_SHOW);
                    PostQuitMessage(0);
                }
                return 0;
            }
            for (int i = 0; i < g_cfg.appCount; i++) {
                if ((g_cfg.apps[i].inStartList && PtInRect(&rcAppListItems[i], pt)) ||
                    (g_cfg.apps[i].inPinnedTiles && PtInRect(&rcTileItems[i], pt))) {
                    g_startOpen = false;
                    ShowWindow(hwnd, SW_HIDE);
                    ShellExecuteA(NULL, "open", g_cfg.apps[i].execPath, NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                }
            }
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* =========================================================================
 * 7. DESKTOP MODE WINDOW (GRID SHORTCUTS, WIDGETS, WALLPAPER, CURSOR OVERLAY)
 * ========================================================================= */
LRESULT CALLBACK DesktopWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static RECT rcDeskIcons[MAX_APPS];
    static int dragIdx = -1;

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            /* Solid or Themed Desktop Backdrop */
            HBRUSH bgBrush = CreateSolidBrush(g_cfg.desktopBgColor);
            FillRect(hdc, &rc, bgBrush);
            DeleteObject(bgBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            HFONT hFont = CreateFontA(-(g_cfg.desktopFontSize + 3), 0, 0, 0, FW_MEDIUM, 0, 0, 0,
                                      DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, g_cfg.desktopFont);
            SelectObject(hdc, hFont);

            /* Desktop Shortcuts Grid */
            if (g_cfg.showDesktopIcons) {
                int grid = g_cfg.desktopGridSize;
                for (int i = 0; i < g_cfg.appCount; i++) {
                    SetRectEmpty(&rcDeskIcons[i]);
                    if (!g_cfg.apps[i].onDesktop) continue;
                    int gx = 16 + (g_cfg.apps[i].gridX * grid);
                    int gy = 16 + (g_cfg.apps[i].gridY * grid);
                    SetRect(&rcDeskIcons[i], gx, gy, gx + grid - 8, gy + grid - 8);

                    RECT icoBox = { gx + (grid - 44)/2, gy + 6, gx + (grid + 28)/2, gy + 42 };
                    FillRoundRectSolid(hdc, icoBox, 8, g_cfg.apps[i].tileColor, RGB(255,255,255), true);
                    DrawTextA(hdc, g_cfg.apps[i].name, 2, &icoBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    RECT lblBox = { gx, gy + 46, gx + grid - 8, gy + grid - 4 };
                    DrawTextA(hdc, g_cfg.apps[i].name, -1, &lblBox, DT_CENTER | DT_TOP | DT_WORDBREAK);
                }
            }

            /* Desktop Clock & Theme Info Widget */
            if (g_cfg.showClockWidget) {
                RECT rcWidget = { rc.right - 250, 24, rc.right - 24, 130 };
                FillRoundRectSolid(hdc, rcWidget, 14, g_cfg.tbColor, g_cfg.accentColor, true);

                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char clkStr[64];
                strftime(clkStr, sizeof(clkStr), "%H:%M:%S\n%A, %d %b %Y", t);

                RECT rcWTxt = { rcWidget.left + 12, rcWidget.top + 14, rcWidget.right - 12, rcWidget.bottom - 30 };
                DrawTextA(hdc, clkStr, -1, &rcWTxt, DT_CENTER);

                char themeInfo[96];
                snprintf(themeInfo, sizeof(themeInfo), "Theme: %s (%s)",
                         THEME_NAMES[g_cfg.theme], g_cfg.lookMode == MODE_CLASSIC_OG ? "Classic OG" : "Modern");
                RECT rcSub = { rcWidget.left + 8, rcWidget.bottom - 26, rcWidget.right - 8, rcWidget.bottom - 6 };
                DrawTextA(hdc, themeInfo, -1, &rcSub, DT_CENTER | DT_SINGLELINE);
            }

            /* Custom Software Cursor Preview Overlay if enabled */
            if (g_cfg.customCursorEnabled) {
                POINT cpt;
                GetCursorPos(&cpt);
                RECT rcCur = { cpt.x - 8, cpt.y - 8, cpt.x + 8, cpt.y + 8 };
                FillRoundRectSolid(hdc, rcCur, 8, g_cfg.cursorColor, g_cfg.cursorBorderColor, true);
            }

            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            if (g_startOpen) {
                g_startOpen = false;
                ShowWindow(g_hwndStartPanel, SW_HIDE);
            }
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            for (int i = 0; i < g_cfg.appCount; i++) {
                if (g_cfg.apps[i].onDesktop && PtInRect(&rcDeskIcons[i], pt)) {
                    if (!g_cfg.lockDesktopIcons) dragIdx = i;
                    break;
                }
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (dragIdx >= 0) {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (g_cfg.desktopSnapToGrid) {
                    g_cfg.apps[dragIdx].gridX = (pt.x - 16) / g_cfg.desktopGridSize;
                    g_cfg.apps[dragIdx].gridY = (pt.y - 16) / g_cfg.desktopGridSize;
                }
                dragIdx = -1;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            for (int i = 0; i < g_cfg.appCount; i++) {
                if (g_cfg.apps[i].onDesktop && PtInRect(&rcDeskIcons[i], pt)) {
                    ShellExecuteA(NULL, "open", g_cfg.apps[i].execPath, NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                }
            }
            return 0;
        }

        case WM_RBUTTONUP:
            OpenSettingsWindow();
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* =========================================================================
 * 8. COMPREHENSIVE TABBED SETTINGS WINDOW (MODERN TOGGLES & SLIDERS)
 * ========================================================================= */
typedef struct {
    const char *label;
    bool *boolPtr;
    RECT toggleRect;
} ToggleItem;

static ToggleItem g_tabToggles[16];
static int g_toggleCount = 0;

void PopulateCurrentTabToggles(int tab) {
    g_toggleCount = 0;
    #define ADD_TOGGLE(lbl, ptr) do { g_tabToggles[g_toggleCount].label = lbl; g_tabToggles[g_toggleCount].boolPtr = ptr; g_toggleCount++; } while(0)

    if (tab == 0) { /* Taskbar Layout & Visibility */
        ADD_TOGGLE("Auto-adjust Taskbar Width to Running Apps", &g_cfg.tbAutoWidth);
        ADD_TOGGLE("Show Window Titles on Running App Buttons", &g_cfg.tbShowWindowTitle);
        ADD_TOGGLE("Round Taskbar Corners", &g_cfg.tbRoundCorners);
        ADD_TOGGLE("Running Application Underline Indicator", &g_cfg.tbRunUnderline);
        ADD_TOGGLE("Hide Winlator Wine's Built-in Taskbar", &g_cfg.hideWineTaskbar);
        ADD_TOGGLE("Adapt Underline Color from App Icon", &g_cfg.underlineAdaptIconColor);
        ADD_TOGGLE("Auto-start with Winlator Container", &g_cfg.autoStartWithContainer);
        ADD_TOGGLE("Show Live App Preview Box on Hover", &g_cfg.showHoverPreview);
        ADD_TOGGLE("Show Day of Week in Taskbar Clock", &g_cfg.tbShowDayInClock);
    } else if (tab == 1) { /* TB Reveal Bar */
        ADD_TOGGLE("Enable TB Reveal Bar at Screen Edge", &g_cfg.revealBarEnabled);
        ADD_TOGGLE("Reveal Bar L-Shape Corner Mode", &g_cfg.revealBarLCorner);
    } else if (tab == 2) { /* Start Panel & Tiles */
        ADD_TOGGLE("Vertical Sidebar Transparent Background", &g_cfg.vertBarTransparent);
        ADD_TOGGLE("Start Panel Transparent Background", &g_cfg.startPanelTransparent);
        ADD_TOGGLE("Pinned Tiles Different Background Colors", &g_cfg.pinnedRandomColors);
        ADD_TOGGLE("Pinned Tiles Box/Slide Rearrange Animation", &g_cfg.pinnedSlideAnim);
        ADD_TOGGLE("Collapse Start App List", &g_cfg.collapseAppList);
        ADD_TOGGLE("Collapse Start Pinned Tiles", &g_cfg.collapsePinnedTiles);
    } else if (tab == 3) { /* Cursor, Keyboard & Window Skin */
        ADD_TOGGLE("Enable Custom Cursor Overlay (.cur/.ani/.png/.ico)", &g_cfg.customCursorEnabled);
        ADD_TOGGLE("Auto-Open Android Keyboard on Input Click", &g_cfg.kbAutoOpenEnabled);
        ADD_TOGGLE("Trigger Keyboard when Cursor is Capital 'I' (IBeam)", &g_cfg.kbTriggerOnIBeam);
        ADD_TOGGLE("Trigger Keyboard when Vertical Line Caret Blinks", &g_cfg.kbTriggerOnCaretBlink);
        ADD_TOGGLE("Trigger Keyboard on MS Office / Canvas App Click", &g_cfg.kbTriggerOnOfficeClick);
        ADD_TOGGLE("Enable Window / Title Bar Look Changer & Transparency", &g_cfg.skinWindowsEnabled);
    } else if (tab == 4) { /* Personalization & Effects */
        ADD_TOGGLE("Drop Shadows on Shell Elements", &g_cfg.dropShadow);
        ADD_TOGGLE("Round All Interface Corners", &g_cfg.roundAllCorners);
        ADD_TOGGLE("Apply Visual Effects to: 1. Taskbar", &g_cfg.effectOnTaskbar);
        ADD_TOGGLE("Apply Visual Effects to: 2. Start Panel", &g_cfg.effectOnStartPanel);
        ADD_TOGGLE("Apply Visual Effects to: 3. Hover Preview Box", &g_cfg.effectOnPreview);
        ADD_TOGGLE("Apply Visual Effects to: 4. Window Title Bars", &g_cfg.effectOnTitleBar);
        ADD_TOGGLE("Apply Visual Effects to: 5. Installed Applications", &g_cfg.effectOnInstalledApps);
    } else if (tab == 5) { /* Desktop Mode & Backup */
        ADD_TOGGLE("Enable Full Desktop Mode (Theme-Aware)", &g_cfg.desktopModeEnabled);
        ADD_TOGGLE("Snap Desktop Shortcut Icons to Grid", &g_cfg.desktopSnapToGrid);
        ADD_TOGGLE("Show Desktop Shortcut Icons", &g_cfg.showDesktopIcons);
        ADD_TOGGLE("Show Desktop Clock & Theme Widget", &g_cfg.showClockWidget);
        ADD_TOGGLE("Lock Movable Desktop Shortcuts", &g_cfg.lockDesktopIcons);
    }
    #undef ADD_TOGGLE
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static RECT rcTabs[6];
    static RECT rcThemeNext, rcModeToggle, rcPosCycle, rcAlignCycle, rcHeightPlus, rcHeightMinus;
    static RECT rcBackupBtn, rcRestoreBtn, rcResetBtn, rcAddShortcutBtn;

    switch (msg) {
        case WM_CREATE:
            PopulateCurrentTabToggles(g_activeSettingsTab);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            /* Theme-aware Settings Backdrop */
            FillRoundRectSolid(hdc, rc, 0, g_cfg.startPanelColor, g_cfg.accentColor, true);

            SetBkMode(hdc, TRANSPARENT);
            COLORREF txtCol = (g_cfg.theme == THEME_WIN98 && g_cfg.lookMode == MODE_CLASSIC_OG) ? RGB(0, 0, 0) : RGB(245, 245, 245);
            SetTextColor(hdc, txtCol);

            HFONT hFont = CreateFontA(-14, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, g_cfg.fontName);
            SelectObject(hdc, hFont);

            /* Top Navigation Tabs */
            const char *tabNames[6] = {
                "1. Taskbar", "2. Reveal Bar", "3. Start Panel",
                "4. Cursor/KB/Win", "5. Themes/FX", "6. Desktop/Backup"
            };
            int tabW = (rc.right - 20) / 6;
            for (int i = 0; i < 6; i++) {
                SetRect(&rcTabs[i], 10 + (i * tabW), 10, 8 + ((i + 1) * tabW), 42);
                COLORREF tBg = (i == g_activeSettingsTab) ? g_cfg.accentColor : g_cfg.tbColor;
                FillRoundRectSolid(hdc, rcTabs[i], 6, tBg, RGB(180,180,180), true);
                DrawTextA(hdc, tabNames[i], -1, &rcTabs[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            /* Quick Action Bar for Theme, OG/Modern Mode, Position & Alignment */
            int topY = 54;
            char infoStr[128];
            snprintf(infoStr, sizeof(infoStr), "Theme: %s (Click to Switch)", THEME_NAMES[g_cfg.theme]);
            SetRect(&rcThemeNext, 14, topY, 270, topY + 32);
            FillRoundRectSolid(hdc, rcThemeNext, 6, g_cfg.tbColor, g_cfg.accentColor, true);
            DrawTextA(hdc, infoStr, -1, &rcThemeNext, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            snprintf(infoStr, sizeof(infoStr), "Look: %s", g_cfg.lookMode == MODE_CLASSIC_OG ? "Classic OG" : "Modern");
            SetRect(&rcModeToggle, 280, topY, 430, topY + 32);
            FillRoundRectSolid(hdc, rcModeToggle, 6, g_cfg.tbColor, g_cfg.accentColor, true);
            DrawTextA(hdc, infoStr, -1, &rcModeToggle, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            const char *posNames[4] = {"Bottom", "Top", "Left", "Right"};
            snprintf(infoStr, sizeof(infoStr), "Pos: %s", posNames[g_cfg.tbPosition]);
            SetRect(&rcPosCycle, 440, topY, 560, topY + 32);
            FillRoundRectSolid(hdc, rcPosCycle, 6, g_cfg.tbColor, g_cfg.accentColor, true);
            DrawTextA(hdc, infoStr, -1, &rcPosCycle, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            const char *alignNames[3] = {"Left", "Middle", "Right (Flipped)"};
            snprintf(infoStr, sizeof(infoStr), "Align: %s", alignNames[g_cfg.tbAlignment]);
            SetRect(&rcAlignCycle, 570, topY, 740, topY + 32);
            FillRoundRectSolid(hdc, rcAlignCycle, 6, g_cfg.tbColor, g_cfg.accentColor, true);
            DrawTextA(hdc, infoStr, -1, &rcAlignCycle, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            /* Render Modern Toggle Switches for Active Tab */
            int rowY = 102;
            for (int i = 0; i < g_toggleCount; i++) {
                RECT rcRow = { 16, rowY, rc.right - 16, rowY + 32 };
                FillRoundRectSolid(hdc, rcRow, 6, g_cfg.tbColor, 0, false);

                RECT rcLbl = { 28, rowY, rc.right - 90, rowY + 32 };
                DrawTextA(hdc, g_tabToggles[i].label, -1, &rcLbl, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                SetRect(&g_tabToggles[i].toggleRect, rc.right - 76, rowY + 5, rc.right - 30, rowY + 27);
                DrawModernToggle(hdc, rc.right - 76, rowY + 5, *(g_tabToggles[i].boolPtr));
                rowY += 38;
            }

            /* Bottom Adjust Bars & Backup/Restore/Reset Controls */
            int botY = rc.bottom - 50;
            snprintf(infoStr, sizeof(infoStr), "TB Height: %dpx [-]", g_cfg.tbHeight);
            SetRect(&rcHeightMinus, 16, botY, 165, botY + 34);
            FillRoundRectSolid(hdc, rcHeightMinus, 6, g_cfg.tbColor, g_cfg.accentColor, true);
            DrawTextA(hdc, infoStr, -1, &rcHeightMinus, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcHeightPlus, 172, botY, 240, botY + 34);
            FillRoundRectSolid(hdc, rcHeightPlus, 6, g_cfg.tbColor, g_cfg.accentColor, true);
            DrawTextA(hdc, "[+ Height]", -1, &rcHeightPlus, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcAddShortcutBtn, 250, botY, 395, botY + 34);
            FillRoundRectSolid(hdc, rcAddShortcutBtn, 6, g_cfg.accentColor, RGB(255,255,255), true);
            DrawTextA(hdc, "+ Add Wine App", -1, &rcAddShortcutBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcBackupBtn, 410, botY, 515, botY + 34);
            FillRoundRectSolid(hdc, rcBackupBtn, 6, RGB(40, 130, 70), RGB(255,255,255), true);
            DrawTextA(hdc, "Backup INI", -1, &rcBackupBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcRestoreBtn, 522, botY, 630, botY + 34);
            FillRoundRectSolid(hdc, rcRestoreBtn, 6, RGB(45, 110, 175), RGB(255,255,255), true);
            DrawTextA(hdc, "Restore INI", -1, &rcRestoreBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SetRect(&rcResetBtn, 638, botY, 745, botY + 34);
            FillRoundRectSolid(hdc, rcResetBtn, 6, RGB(185, 50, 50), RGB(255,255,255), true);
            DrawTextA(hdc, "Reset All", -1, &rcResetBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            for (int i = 0; i < 6; i++) {
                if (PtInRect(&rcTabs[i], pt)) {
                    g_activeSettingsTab = i;
                    PopulateCurrentTabToggles(g_activeSettingsTab);
                    InvalidateRect(hwnd, NULL, TRUE);
                    return 0;
                }
            }

            if (PtInRect(&rcThemeNext, pt)) {
                ThemeType next = (ThemeType)((g_cfg.theme + 1) % THEME_COUNT);
                if (next == THEME_NONE) next = THEME_WIN98;
                ApplyThemePreset(&g_cfg, next, g_cfg.lookMode);
                RepositionShellWindows();
                InvalidateRect(g_hwndDesktop, NULL, TRUE);
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            if (PtInRect(&rcModeToggle, pt)) {
                ThemeLookMode nextM = (g_cfg.lookMode == MODE_CLASSIC_OG) ? MODE_MODERN : MODE_CLASSIC_OG;
                ApplyThemePreset(&g_cfg, g_cfg.theme, nextM);
                RepositionShellWindows();
                InvalidateRect(g_hwndDesktop, NULL, TRUE);
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            if (PtInRect(&rcPosCycle, pt)) {
                g_cfg.tbPosition = (BarPosition)((g_cfg.tbPosition + 1) % 4);
                RepositionShellWindows();
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            if (PtInRect(&rcAlignCycle, pt)) {
                g_cfg.tbAlignment = (BarAlignment)((g_cfg.tbAlignment + 1) % 3);
                RepositionShellWindows();
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            if (PtInRect(&rcHeightMinus, pt)) {
                if (g_cfg.tbHeight > 28) g_cfg.tbHeight -= 4;
                RepositionShellWindows();
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            if (PtInRect(&rcHeightPlus, pt)) {
                if (g_cfg.tbHeight < 76) g_cfg.tbHeight += 4;
                RepositionShellWindows();
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            if (PtInRect(&rcAddShortcutBtn, pt)) {
                OPENFILENAMEA ofn = {0};
                char szFile[MAX_PATH] = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "Executables (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&ofn) && g_cfg.appCount < MAX_APPS) {
                    AppShortcut *ap = &g_cfg.apps[g_cfg.appCount];
                    const char * slash = strrchr(szFile, '\\');
                    strncpy(ap->name, slash ? slash + 1 : szFile, 63);
                    strncpy(ap->execPath, szFile, MAX_PATH - 1);
                    ap->tileColor = g_cfg.accentColor;
                    ap->gridX = 1;
                    ap->gridY = g_cfg.appCount % 6;
                    ap->inStartList = true;
                    ap->inPinnedTiles = true;
                    ap->inTaskbar = true;
                    ap->onDesktop = true;
                    g_cfg.appCount++;
                    InvalidateRect(g_hwndTaskbar, NULL, TRUE);
                    InvalidateRect(g_hwndDesktop, NULL, TRUE);
                }
                return 0;
            }

            if (PtInRect(&rcBackupBtn, pt)) {
                SaveConfigToFile(&g_cfg, CONFIG_FILE);
                MessageBoxA(hwnd, "All settings backed up to winlator_shell_config.ini!", "Backup Complete", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            if (PtInRect(&rcRestoreBtn, pt)) {
                if (LoadConfigFromFile(&g_cfg, CONFIG_FILE)) {
                    ApplyWineTaskbarVisibility();
                    RepositionShellWindows();
                    InvalidateRect(g_hwndDesktop, NULL, TRUE);
                    InvalidateRect(hwnd, NULL, TRUE);
                    MessageBoxA(hwnd, "Settings restored from winlator_shell_config.ini!", "Restore Complete", MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxA(hwnd, "No backup file found yet. Click Backup INI first.", "Restore", MB_OK | MB_ICONWARNING);
                }
                return 0;
            }

            if (PtInRect(&rcResetBtn, pt)) {
                LoadDefaultConfig(&g_cfg);
                ApplyWineTaskbarVisibility();
                RepositionShellWindows();
                InvalidateRect(g_hwndDesktop, NULL, TRUE);
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            for (int i = 0; i < g_toggleCount; i++) {
                if (PtInRect(&g_tabToggles[i].toggleRect, pt)) {
                    *(g_tabToggles[i].boolPtr) = !(*(g_tabToggles[i].boolPtr));
                    ApplyWineTaskbarVisibility();
                    UpdateAutoStartRegistry();
                    ShowWindow(g_hwndDesktop, g_cfg.desktopModeEnabled ? SW_SHOW : SW_HIDE);
                    RepositionShellWindows();
                    InvalidateRect(g_hwndDesktop, NULL, TRUE);
                    InvalidateRect(hwnd, NULL, TRUE);
                    return 0;
                }
            }
            return 0;
        }

        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void OpenSettingsWindow(void) {
    if (!g_hwndSettings) {
        g_hwndSettings = CreateWindowExA(WS_EX_TOPMOST, "WinlatorSettingsCls",
            "Winlator Taskbar, Theme & Desktop Settings",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            100, 70, 775, 535, NULL, NULL, g_hInst, NULL);
    }
    ShowWindow(g_hwndSettings, SW_SHOW);
    SetForegroundWindow(g_hwndSettings);
}

/* =========================================================================
 * 9. MAIN ENTRY POINT & GLOBAL HEARTBEAT TIMER
 * ========================================================================= */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;
    LoadDefaultConfig(&g_cfg);
    LoadConfigFromFile(&g_cfg, CONFIG_FILE);

    if (g_cfg.startupDelaySec > 0) {
        Sleep(g_cfg.startupDelaySec * 1000);
    }

    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA) };
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.lpfnWndProc = DesktopWndProc;
    wc.lpszClassName = "WinlatorDesktopCls";
    wc.style = CS_DBLCLKS;
    RegisterClassExA(&wc);

    wc.lpfnWndProc = TaskbarWndProc;
    wc.lpszClassName = "WinlatorTaskbarCls";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExA(&wc);

    wc.lpfnWndProc = RevealBarWndProc;
    wc.lpszClassName = "WinlatorRevealCls";
    RegisterClassExA(&wc);

    wc.lpfnWndProc = StartPanelWndProc;
    wc.lpszClassName = "WinlatorStartCls";
    RegisterClassExA(&wc);

    wc.lpfnWndProc = PreviewWndProc;
    wc.lpszClassName = "WinlatorPreviewCls";
    RegisterClassExA(&wc);

    wc.lpfnWndProc = SettingsWndProc;
    wc.lpszClassName = "WinlatorSettingsCls";
    RegisterClassExA(&wc);

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    /* Create Desktop Window */
    g_hwndDesktop = CreateWindowExA(WS_EX_TOOLWINDOW, "WinlatorDesktopCls", "WinlatorDesktop",
        WS_POPUP | WS_VISIBLE, 0, 0, scrW, scrH, NULL, NULL, hInstance, NULL);
    SetWindowPos(g_hwndDesktop, HWND_BOTTOM, 0, 0, scrW, scrH, SWP_NOACTIVATE);

    /* Create Taskbar */
    g_hwndTaskbar = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "WinlatorTaskbarCls", "WinlatorTaskbar",
        WS_POPUP | WS_VISIBLE, 0, scrH - g_cfg.tbHeight, scrW, g_cfg.tbHeight, NULL, NULL, hInstance, NULL);

    /* Create TB Reveal Bar */
    g_hwndRevealBar = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED, "WinlatorRevealCls", "WinlatorReveal",
        WS_POPUP, 0, scrH - 5, 240, 5, NULL, NULL, hInstance, NULL);

    /* Create Start Panel */
    g_hwndStartPanel = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED, "WinlatorStartCls", "WinlatorStart",
        WS_POPUP, 0, scrH - 480, 520, 440, NULL, NULL, hInstance, NULL);

    /* Create Live Hover Preview Box */
    g_hwndPreview = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "WinlatorPreviewCls", "WinlatorPreview",
        WS_POPUP, 0, 0, g_cfg.previewWidth, g_cfg.previewHeight, NULL, NULL, hInstance, NULL);

    ApplyWineTaskbarVisibility();
    UpdateAutoStartRegistry();
    RefreshRunningWindows();
    RepositionShellWindows();

    /* Main 500ms System Timer for Clock, Running Windows, Reveal Auto-Hide & Android Keyboard Check */
    SetTimer(g_hwndTaskbar, 1, 500, NULL);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        if (msg.message == WM_TIMER) {
            RefreshRunningWindows();
            CheckAndTriggerAndroidKeyboard();

            if (g_tbTemporarilyShown && GetTickCount() > g_tbHideTick) {
                g_tbTemporarilyShown = false;
                if (g_cfg.fullscreenMode == 3) {
                    ShowWindow(g_hwndTaskbar, SW_HIDE);
                }
            }
            InvalidateRect(g_hwndTaskbar, NULL, FALSE);
            if (g_cfg.showClockWidget && g_cfg.desktopModeEnabled) {
                InvalidateRect(g_hwndDesktop, NULL, FALSE);
            }
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    SaveConfigToFile(&g_cfg, CONFIG_FILE);
    return 0;
}