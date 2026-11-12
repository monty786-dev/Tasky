/* bridge.h - TCP IPC layer + window enumeration for Win32Bridge.exe.
 * Self-contained: does not include the GUI headers of the main app. */
#ifndef WLC_BRIDGE_H
#define WLC_BRIDGE_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")

typedef struct {
    SOCKET sock;         /* INVALID_SOCKET when disconnected */
    WlcHeader hdr;
    unsigned char *buf;  /* reassembly buffer */
    int bufLen, bufCap;
} Bridge;

static Bridge g_br = { INVALID_SOCKET };

/* ---------- low-level framing ---------- */
static int BridgeSend(Bridge *b, unsigned int type, const void *payload, unsigned int len) {
    WlcHeader h = { WLC_MAGIC, WLC_VERSION, type, len };
    if (b->sock == INVALID_SOCKET) return -1;
    if (send(b->sock, (const char *)&h, sizeof h, 0) != sizeof h) return -1;
    if (len && send(b->sock, (const char *)payload, len, 0) != (int)len) return -1;
    return 0;
}

/* returns 1 = full frame ready, 0 = need more, -1 = error */
static int BridgeReadFrame(Bridge *b, unsigned int *outType,
                           unsigned char **outPayload, unsigned int *outLen) {
    int n;
    for (;;) {
        if (b->bufLen < (int)sizeof(WlcHeader)) {
            n = recv(b->sock, (char *)b->buf + b->bufLen, b->bufCap - b->bufLen, 0);
            if (n <= 0) return -1;
            b->bufLen += n;
            continue;
        }
        WlcHeader *h = (WlcHeader *)b->buf;
        if (h->magic != WLC_MAGIC || h->version != WLC_VERSION) return -1;
        unsigned int need = sizeof(WlcHeader) + h->length;
        if ((unsigned int)b->bufLen < need) {
            if (need > (unsigned int)b->bufCap) {
                b->bufCap = need;
                b->buf = (unsigned char *)realloc(b->buf, b->bufCap);
            }
            n = recv(b->sock, (char *)b->buf + b->bufLen, b->bufCap - b->bufLen, 0);
            if (n <= 0) return -1;
            b->bufLen += n;
            continue;
        }
        *outType = h->type;
        *outLen  = h->length;
        *outPayload = b->buf + sizeof(WlcHeader);
        /* caller must call BridgeConsume() when done with the payload */
        return 1;
    }
}

static void BridgeConsume(Bridge *b) {
    WlcHeader *h = (WlcHeader *)b->buf;
    int used = sizeof(WlcHeader) + h->length;
    memmove(b->buf, b->buf + used, b->bufLen - used);
    b->bufLen -= used;
}

/* ---------- connect / accept ---------- */
/* Android client connects first; Wine side listens. */
static int BridgeListen(void) {
    WSADATA wsa;
    SOCKET ls;
    struct sockaddr_in a;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ls == INVALID_SOCKET) return -1;
    int on = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof on);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(WLC_PORT);
    if (bind(ls, (struct sockaddr *)&a, sizeof a) != 0) { closesocket(ls); return -1; }
    listen(ls, 1);
    /* non-blocking accept polled from the main loop */
    u_long nb = 1;
    ioctlsocket(ls, FIONBIO, &nb);
    g_br.sock = ls;  /* temporary: ls is the listener until a client connects */
    return 0;
}

static void BridgePollAccept(void) {
    if (g_br.sock == INVALID_SOCKET) return;
    SOCKET c = accept(g_br.sock, NULL, NULL);
    if (c == INVALID_SOCKET) return;
    closesocket(g_br.sock);   /* drop the listener */
    g_br.sock = c;
    u_long nb = 1; ioctlsocket(c, FIONBIO, &nb);
    g_br.bufCap = 65536;
    g_br.buf = (unsigned char *)malloc(g_br.bufCap);
    g_br.bufLen = 0;
    SetWindowLongPtrW(GetConsoleWindow(), GWL_USERDATA, 1);
}

static void BridgeClose(void) {
    if (g_br.sock != INVALID_SOCKET) closesocket(g_br.sock);
    if (g_br.buf) free(g_br.buf);
    memset(&g_br, 0, sizeof g_br);
    g_br.sock = INVALID_SOCKET;
    WSACleanup();
}

/* ---------- window enumeration ---------- */
typedef struct { HWND hwnd; HWND owner; } WinRec;
static WinRec g_wins[512];
static int g_nwins;

static BOOL CALLBACK BridgeEnumProc(HWND h, LPARAM lp) {
    (void)lp;
    if (g_nwins >= 512) return FALSE;
    if (!IsWindowVisible(h)) return TRUE;
    LONG st = GetWindowLongW(h, GWL_STYLE);
    LONG ex = GetWindowLongW(h, GWL_EXSTYLE);
    if ((st & WS_CHILD)) return TRUE;
    if (!(st & WS_CAPTION)) return TRUE;
    if ((ex & WS_EX_TOOLWINDOW) && !(ex & WS_EX_APPWINDOW)) return TRUE;
    wchar_t cls[64] = { 0 };
    GetClassNameW(h, cls, 64);
    if (!wcscmp(cls, L"Progman") || !wcscmp(cls, L"Shell_TrayWnd") ||
        !wcscmp(cls, L"WorkerW") || !wcscmp(cls, L"#32768")) return TRUE;
    g_wins[g_nwins].hwnd = h;
    g_wins[g_nwins].owner = GetWindow(h, GW_OWNER);
    g_nwins++;
    return TRUE;
}

static void BridgeSendWindowList(void) {
    int i;
    g_nwins = 0;
    EnumWindows(BridgeEnumProc, 0);

    /* first pass: compute payload size */
    unsigned int total = 4;  /* u32 count */
    for (i = 0; i < g_nwins; i++) {
        wchar_t t[256] = { 0 }, c[64] = { 0 }, p[64] = { 0 };
        GetWindowTextW(g_wins[i].hwnd, t, 256);
        GetClassNameW(g_wins[i].hwnd, c, 64);
        DWORD pid = 0;
        GetWindowThreadProcessId(g_wins[i].hwnd, &pid);
        _snwprintf(p, 63, L"pid %lu", (unsigned long)pid);
        total += sizeof(WlcWinEntry) + (unsigned int)(wcslen(t) + wcslen(c) + wcslen(p)) * 2;
    }
    unsigned char *buf = (unsigned char *)malloc(total);
    unsigned char *q = buf;
    *(unsigned int *)q = (unsigned int)g_nwins; q += 4;
    for (i = 0; i < g_nwins; i++) {
        wchar_t t[256] = { 0 }, c[64] = { 0 }, p[64] = { 0 };
        GetWindowTextW(g_wins[i].hwnd, t, 256);
        GetClassNameW(g_wins[i].hwnd, c, 64);
        DWORD pid = 0;
        GetWindowThreadProcessId(g_wins[i].hwnd, &pid);
        _snwprintf(p, 63, L"pid %lu", (unsigned long)pid);
        LONG st = GetWindowLongW(g_wins[i].hwnd, GWL_STYLE);
        WlcWinEntry e;
        e.hwnd = (unsigned long long)(ULONG_PTR)g_wins[i].hwnd;
        e.flags = 0;
        if (GetForegroundWindow() == g_wins[i].hwnd) e.flags |= 1;
        if (IsIconic(g_wins[i].hwnd))            e.flags |= 2;
        if (IsZoomed(g_wins[i].hwnd))            e.flags |= 4;
        e.titleLen = (unsigned int)wcslen(t);
        e.clsLen   = (unsigned int)wcslen(c);
        e.procLen  = (unsigned int)wcslen(p);
        e.iconRGBA = 0;
        memcpy(q, &e, sizeof e); q += sizeof e;
        memcpy(q, t, e.titleLen * 2); q += e.titleLen * 2;
        memcpy(q, c, e.clsLen   * 2); q += e.clsLen   * 2;
        memcpy(q, p, e.procLen  * 2); q += e.procLen  * 2;
    }
    BridgeSend(&g_br, MSG_WIN_LIST, buf, total);
    free(buf);
}

/* thumbnail: PrintWindow into a top-down 32-bit DIB, send raw BGRA */
static void BridgeSendThumb(HWND h, unsigned int tw, unsigned int th) {
    if (tw == 0 || th == 0 || tw > 1920 || th > 1080) return;
    HDC hdc = GetDC(NULL);
    HDC mem = CreateCompatibleDC(hdc);
    BITMAPINFO bi;
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = tw;
    bi.bmiHeader.biHeight = -(LONG)th;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void *bits = NULL;
    HBITMAP bmp = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    HGDIOBJ old = SelectObject(mem, bmp);
    SetStretchBltMode(mem, HALFTONE);
    RECT r; GetWindowRect(h, &r);
    int sw = r.right - r.left, sh = r.bottom - r.top;
    if (sw > 0 && sh > 0) StretchBlt(mem, 0, 0, tw, th, GetWindowDC(h), 0, 0, sw, sh, SRCCOPY);
    SelectObject(mem, old);
    unsigned int payloadLen = (unsigned int)sizeof(unsigned long long) + 8 + tw * th * 4;
    unsigned char *p = (unsigned char *)malloc(payloadLen);
    *(unsigned long long *)p = (unsigned long long)(ULONG_PTR)h;
    *(unsigned int *)(p + 8) = tw;
    *(unsigned int *)(p + 12) = th;
    memcpy(p + 16, bits, tw * th * 4);
    BridgeSend(&g_br, MSG_WIN_THUMB, p, payloadLen);
    free(p);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(NULL, hdc);
}

/* ---------- command dispatch ---------- */
static void BridgeDispatch(unsigned int type, const unsigned char *p, unsigned int n) {
    (void)n;
    switch (type) {
    case MSG_PING:
        BridgeSend(&g_br, MSG_PONG, NULL, 0);
        break;
    case MSG_WIN_LIST_REQ:
        BridgeSendWindowList();
        break;
    case MSG_WIN_ACTIVATE: {
        HWND h = (HWND)(ULONG_PTR)*(unsigned long long *)p;
        if (IsWindow(h)) { ShowWindow(h, IsIconic(h) ? SW_RESTORE : SW_SHOW); SetForegroundWindow(h); }
        break; }
    case MSG_WIN_MINIMIZE: {
        HWND h = (HWND)(ULONG_PTR)*(unsigned long long *)p;
        if (IsWindow(h)) ShowWindow(h, SW_MINIMIZE);
        break; }
    case MSG_WIN_CLOSE: {
        HWND h = (HWND)(ULONG_PTR)*(unsigned long long *)p;
        if (IsWindow(h)) PostMessageW(h, WM_CLOSE, 0, 0);
        break; }
    case MSG_WIN_THUMB_REQ: {
        HWND h = (HWND)(ULONG_PTR)*(unsigned long long *)p;
        unsigned int w = *(unsigned int *)(p + 8), hh = *(unsigned int *)(p + 12);
        if (IsWindow(h)) BridgeSendThumb(h, w, hh);
        break; }
    case MSG_SETTINGS:
        /* payload is UTF-16LE "k=v;k=v;" - parse and apply */
        break;
    }
}

static void BridgePump(void) {
    unsigned int type, len;
    unsigned char *p;
    int r;
    if (g_br.sock == INVALID_SOCKET) return;
    r = BridgeReadFrame(&g_br, &type, &p, &len);
    if (r < 0) { BridgeClose(); return; }
    if (r == 0) return;
    BridgeDispatch(type, p, len);
    BridgeConsume(&g_br);
}

#endif
