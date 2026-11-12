/* Win32Bridge.c - headless companion to the Android taskbar.
 *
 * Build (Linux cross-compile):
 *   x86_64-w64-mingw32-gcc -O2 -municode -o Win32Bridge.exe Win32Bridge.c -lws2_32 -luser32 -lgdi32
 *
 * Runs inside the Winlator container. Listens on 127.0.0.1:47921, waits for the
 * Android taskbar to connect, then streams window info and honours commands.
 * Does NOT paint anything: the Android side owns all visuals.
 */
#include <windows.h>
#include "protocol.h"
#include "bridge.h"

static volatile LONG g_quit = 0;

static BOOL WINAPI ConsoleCtrl(DWORD t) { (void)t; InterlockedExchange(&g_quit, 1); return TRUE; }

/* Optional: strip WS_CAPTION from a window so Wine does not draw its own title bar.
 * The Android taskbar paints one instead. Toggle via MSG_SETTINGS "hidecaption=1". */
static void StripCaption(HWND h) {
    LONG st = GetWindowLongW(h, GWL_STYLE);
    if ((st & WS_CAPTION) == WS_CAPTION) {
        SetWindowLongW(h, GWL_STYLE, st & ~WS_CAPTION);
        SetWindowPos(h, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

static DWORD WINAPI StripAllThread(LPVOID lp) {
    (void)lp;
    /* called once at startup when taskbar mode is on */
    EnumWindows((WNDENUMPROC)[](HWND h, LPARAM p) -> BOOL {
        (void)p;
        if (!IsWindowVisible(h)) return TRUE;
        LONG st = GetWindowLongW(h, GWL_STYLE);
        if ((st & WS_CHILD) || !(st & WS_CAPTION)) return TRUE;
        StripCaption(h);
        return TRUE;
    }, 0);
    return 0;
}

int wmain(int argc, wchar_t **argv) {
    (void)argc; (void)argv;
    SetConsoleCtrlHandler(ConsoleCtrl, TRUE);

    if (BridgeListen() != 0) {
        fwprintf(stderr, L"Win32Bridge: cannot bind %d\n", WLC_PORT);
        return 1;
    }
    wprintf(L"Win32Bridge listening on 127.0.0.1:%d\n", WLC_PORT);

    int stripped = 0;
    while (!g_quit) {
        BridgePollAccept();
        BridgePump();
        if (!stripped && g_br.sock != INVALID_SOCKET) {
            StripAllThread(NULL);
            stripped = 1;
        }
        Sleep(16);
    }
    BridgeClose();
    return 0;
}
