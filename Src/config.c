#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>
#include <stdbool.h>

#define MAX_APPS 64
#define CONFIG_FILE "winlator_shell_config.ini"

typedef enum {
    THEME_NONE = 0,
    THEME_WIN98,
    THEME_WIN7,
    THEME_WIN8,
    THEME_WIN10,
    THEME_UBUNTU,
    THEME_ARCH,
    THEME_MINT,
    THEME_XFCE,
    THEME_CINNAMON,
    THEME_KDE,
    THEME_COUNT
} ThemeType;

typedef enum {
    MODE_CLASSIC_OG = 0,
    MODE_MODERN = 1
} ThemeLookMode;

typedef enum {
    POS_BOTTOM = 0,
    POS_TOP,
    POS_LEFT,
    POS_RIGHT
} BarPosition;

typedef enum {
    ALIGN_LEFT = 0,
    ALIGN_MIDDLE,
    ALIGN_RIGHT
} BarAlignment;

typedef enum {
    COLOR_MODE_NONE = 0,
    COLOR_MODE_DARK,
    COLOR_MODE_LIGHT,
    COLOR_MODE_AMOLED,
    COLOR_MODE_ROSE,
    COLOR_MODE_SAPPHIRE,
    COLOR_MODE_AERO
} ColorMode;

typedef struct {
    char name[64];
    char execPath[MAX_PATH];
    COLORREF tileColor;
    int tileW;
    int tileH;
    int gridX;
    int gridY;
    bool inStartList;
    bool inPinnedTiles;
    bool inTaskbar;
    bool inAppTray;
    bool onDesktop;
} AppShortcut;

typedef struct {
    /* 1. Theme & Personalization */
    ThemeType theme;
    ThemeLookMode lookMode; /* 0 = Classic OG, 1 = Modern */
    ColorMode colorMode;
    int effectType;         /* 0=None, 1=Glass, 2=iPhone Blur, 3=3D Bevel */
    int blurIntensity;      /* 0-100 */
    COLORREF accentColor;
    int glassShine;         /* 0-100 */
    int glassGloss;         /* 0-100 */
    bool dropShadow;
    bool roundAllCorners;
    bool effectOnTaskbar;
    bool effectOnStartPanel;
    bool effectOnPreview;
    bool effectOnTitleBar;
    bool effectOnInstalledApps;
    int iconPresetIndex;    /* 0-9 (10 presets) */
    char fontName[64];
    int fontSize;           /* 8-24 */
    int uiScale;            /* 80-150 % */

    /* 2. Taskbar Layout */
    BarPosition tbPosition;
    BarAlignment tbAlignment;
    int tbHeight;           /* 28-80 px */
    bool tbAutoWidth;
    int tbIconSize;         /* 16-48 px */
    bool tbShowWindowTitle;
    bool tbRoundCorners;
    int tbDetachGap;        /* 0-24 px floating margin */
    COLORREF tbColor;
    bool tbRunUnderline;
    int tbUnderlineHeight;  /* 2-8 px */
    int tbDividerStyle;     /* 0=Thin (1px), 1=Mid (2px), 2=Bold (4px) */
    bool tbShowDayInClock;

    /* 3. Taskbar Visibility & Wine Integration */
    bool hideWineTaskbar;
    bool underlineAdaptIconColor;
    int fullscreenMode;     /* 0=Keep Visible, 1=Over App, 2=Floating Icon, 3=Hide Bar */
    int fullscreenAppliesTo;/* 0=Fullscreen, 1=Maximized, 2=Both */
    bool autoStartWithContainer;
    int startupDelaySec;    /* 0-30 s */
    bool showHoverPreview;
    int previewWidth;       /* 120-320 */
    int previewHeight;      /* 80-220 */

    /* 4. TB Reveal Bar */
    bool revealBarEnabled;
    COLORREF revealBarColor;
    int revealBarPos;       /* 0=Bottom, 1=Top */
    BarAlignment revealBarAlign;
    bool revealBarLCorner;
    int revealBarOpacity;   /* 20-255 */
    int revealBarHeight;    /* 2-20 px */
    int revealBarWidth;     /* 50-1920 px */
    int tbVisibleSeconds;   /* 1-15 s */
    int revealAnimation;    /* 0=Blink, 1=Pop-up, 2=Slide Left, 3=Slide Right, 4=Slide Bottom */

    /* 5. Start Button & Start Panel */
    int startPanelSize;     /* 0=Normal, 1=Mid, 2=Large, 3=Fullscreen */
    char startBtnIconPath[MAX_PATH];
    COLORREF startBtnColor;
    char userName[64];
    char userPicPath[MAX_PATH];
    COLORREF vertBarColor;
    bool vertBarTransparent;
    COLORREF startPanelColor;
    bool startPanelTransparent;
    int startPanelAlpha;    /* 50-255 */
    bool pinnedRandomColors;
    int pinnedTileAlpha;    /* 50-255 */
    bool pinnedSlideAnim;
    int pinnedTileSize;     /* 48-120 px */
    bool collapseAppList;
    bool collapsePinnedTiles;

    /* 6. Custom Cursor */
    bool customCursorEnabled;
    char cursorFilePath[MAX_PATH];
    COLORREF cursorColor;
    COLORREF cursorBorderColor;
    int cursorBorderWidth;

    /* 7. Winlator Android Soft-Keyboard Auto-Trigger */
    bool kbAutoOpenEnabled;
    bool kbTriggerOnIBeam;
    bool kbTriggerOnCaretBlink;
    bool kbTriggerOnOfficeClick;
    char kbBridgeCommand[MAX_PATH];

    /* 8. Window / Title Bar Look Changer */
    bool skinWindowsEnabled;
    int windowTransparency; /* 80-255 */
    int skinTargetMode;     /* 0=Title Bar Only, 1=Entire Window */

    /* 9. Desktop Mode */
    bool desktopModeEnabled;
    int desktopGridSize;    /* 64-128 px */
    bool desktopSnapToGrid;
    int desktopIconCorner;  /* 0=Top-Left, 1=Bottom-Left, 2=Top-Right, 3=Bottom-Right */
    bool showDesktopIcons;
    bool showClockWidget;
    bool showAppWidget;
    char wallpaperPath[MAX_PATH];
    COLORREF desktopBgColor;
    int wallpaperFit;       /* 0=Crop, 1=Fit, 2=Stretch */
    bool lockDesktopIcons;
    char desktopFont[64];
    int desktopFontSize;
    int desktopUiScale;

    /* 10. Registered Applications */
    int appCount;
    AppShortcut apps[MAX_APPS];
} AppConfig;

extern AppConfig g_cfg;

void LoadDefaultConfig(AppConfig *cfg);
bool SaveConfigToFile(const AppConfig *cfg, const char *filename);
bool LoadConfigFromFile(AppConfig *cfg, const char *filename);
void ApplyThemePreset(AppConfig *cfg, ThemeType theme, ThemeLookMode mode);

#endif /* CONFIG_H */