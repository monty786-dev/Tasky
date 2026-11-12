/* protocol.h - wire format between Win32Bridge.exe (Wine) and Android taskbar.
 * Keep this file byte-identical on both sides. */
#ifndef WLC_PROTOCOL_H
#define WLC_PROTOCOL_H

#define WLC_MAGIC        0x574C4342u   /* 'WLCB' */
#define WLC_VERSION      1
#define WLC_PORT         47921

/* message types */
enum {
    MSG_PING         = 1,
    MSG_PONG         = 2,
    MSG_WIN_LIST_REQ = 10,   /* Android -> Wine */
    MSG_WIN_LIST     = 11,   /* Wine -> Android */
    MSG_WIN_ACTIVATE = 20,   /* Android -> Wine; payload: u64 hwnd */
    MSG_WIN_MINIMIZE = 21,
    MSG_WIN_CLOSE    = 22,
    MSG_WIN_THUMB_REQ= 30,   /* Android -> Wine; payload: u64 hwnd, u32 w, u32 h */
    MSG_WIN_THUMB    = 31,   /* Wine -> Android; payload: u64 hwnd, u32 w, u32 h, BMP bytes */
    MSG_SETTINGS     = 40,   /* Android -> Wine; payload: UTF-16LE json-ish key=value; */
    MSG_LOG          = 50
};

#pragma pack(push, 1)
typedef struct {
    unsigned int  magic;
    unsigned int  version;
    unsigned int  type;
    unsigned int  length;    /* payload bytes that follow */
} WlcHeader;

typedef struct {
    unsigned long long hwnd;
    unsigned int  flags;     /* 1=active, 2=minimized, 4=maximized, 8=no-skin */
    unsigned int  titleLen;  /* UTF-16 code units, no terminator */
    unsigned int  clsLen;
    unsigned int  procLen;
    unsigned int  iconRGBA;  /* reserved: 0 = caller fetches from exe */
} WlcWinEntry;

typedef struct {
    unsigned int  w, h;
} WlcThumbReq;
#pragma pack(pop)

#endif
