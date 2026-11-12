#!/usr/bin/env bash
# wine_tune.sh - apply WLC recommended tuning to a Winlator Wine prefix.
# Usage: WINEPREFIX=~/.wine bash wine_tune.sh
set -euo pipefail
export WINEDEBUG=-all

wine reg add 'HKCU\Software\Wine\Explorer' /v ShowSystray /t REG_DWORD /d 0 /f
wine reg add 'HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer' \
    /v NoTrayItemsDisplay /t REG_DWORD /d 1 /f
wine reg add 'HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer' \
    /v NoToolbarsOnTaskbar /t REG_DWORD /d 1 /f

# box64 rcfile fragment (Winlator reads $WINEPREFIX/box64rc)
RC="$WINEPREFIX/box64rc"
cat >> "$RC" <<'EOF'
[BOX64]
DYNAREC_BIGBLOCK=2
DYNACACHE=1
DYNACACHE_LIMIT=512
DYNAREC_WEAKBARRIER=1
DYNAREC_FASTNAN=1
DYNAREC_FASTROUND=1
MAXCPU=4
EOF

echo "WLC: tuning applied to $WINEPREFIX"
