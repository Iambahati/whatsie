#!/bin/bash
# setup.sh — install build dependencies and compile WhatSie (Qt5/qmake, Ubuntu/Debian)

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()    { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── OS check ────────────────────────────────────────────────────────────────
if ! command -v apt-get &>/dev/null; then
    error "This script requires apt-get. Only Ubuntu/Debian is supported."
fi

# ── Dependency list ─────────────────────────────────────────────────────────
DEPS=(
    build-essential
    libx11-dev
    qt5-qmake
    qtbase5-dev
    qtwebengine5-dev      # pulls in webenginewidgets, positioning, network, QML, webchannel
    qtpositioning5-dev    # QT += positioning
)

# ── Check which are missing ─────────────────────────────────────────────────
MISSING=()
for pkg in "${DEPS[@]}"; do
    if ! dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "install ok installed"; then
        MISSING+=("$pkg")
    fi
done

if [ ${#MISSING[@]} -gt 0 ]; then
    info "Installing missing packages: ${MISSING[*]}"
    sudo apt-get update -qq
    sudo apt-get install -y "${MISSING[@]}"
else
    info "All build dependencies are already installed."
fi

# ── Verify qmake resolves to the Qt5 binary ─────────────────────────────────
QMAKE_BIN=/usr/lib/qt5/bin/qmake
if [ ! -x "$QMAKE_BIN" ]; then
    # Fallback: find any qt5 qmake
    QMAKE_BIN=$(find /usr/lib/qt5 /usr/bin -name "qmake" -o -name "qmake-qt5" 2>/dev/null | head -1)
    [ -x "$QMAKE_BIN" ] || error "Could not locate a Qt5 qmake binary after installing packages."
fi
info "Using qmake: $QMAKE_BIN ($($QMAKE_BIN --version | head -1))"

# ── Regenerate Makefile ──────────────────────────────────────────────────────
info "Generating Makefile..."
rm -f .qmake.stash Makefile
"$QMAKE_BIN" -o Makefile src/WhatsApp.pro

# ── Build ────────────────────────────────────────────────────────────────────
JOBS=$(nproc)
info "Building with $JOBS parallel jobs..."
make -j"$JOBS"

info "Build complete. Binary: $SCRIPT_DIR/whatsie"
echo
echo "Next steps:"
echo "  Run locally:        ./whatsie"
echo "  Install system-wide: sudo make install"
echo "  Desktop integration: bash install_desktop.sh"
