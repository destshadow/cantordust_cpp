#!/bin/bash
# CantorDust++ — Setup automatico
# Testato su: Ubuntu 22.04/24.04, Debian 12, WSL2

set -e  # stop on error

echo "╔══════════════════════════════════╗"
echo "║   CantorDust++ Setup Script      ║"
echo "╚══════════════════════════════════╝"

# --- Rileva OS ---
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    OS="unknown"
fi

echo "[1/4] Installazione dipendenze di sistema..."

if [[ "$OS" == "ubuntu" || "$OS" == "debian" ]]; then
    sudo apt update -q
    sudo apt install -y \
        build-essential \
        g++ \
        cmake \
        libglu1-mesa-dev \
        libpng-dev \
        libx11-dev \
        python3 \
        python3-pip \
        python3-venv \
        wget
elif [[ "$OS" == "fedora" ]]; then
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        mesa-libGLU-devel \
        libpng-devel \
        libX11-devel \
        python3 \
        python3-pip \
        wget
elif [[ "$OS" == "arch" || "$OS" == "manjaro" ]]; then
    sudo pacman -Sy --noconfirm \
        base-devel \
        cmake \
        glu \
        libpng \
        libx11 \
        python \
        python-pip \
        wget
else
    echo "⚠️  OS non riconosciuto: $OS"
    echo "   Installa manualmente: g++ libglu-dev libpng-dev libx11-dev python3"
fi

echo "[2/4] Download librerie single-header..."
mkdir -p libs
cd libs

if [ ! -f "olcPixelGameEngine.h" ]; then
    wget -q https://raw.githubusercontent.com/OneLoneCoder/olcPixelGameEngine/master/olcPixelGameEngine.h
    echo "  ✅ olcPixelGameEngine.h"
else
    echo "  ✓  olcPixelGameEngine.h (già presente)"
fi

if [ ! -f "stb_image_write.h" ]; then
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
    echo "  ✅ stb_image_write.h"
else
    echo "  ✓  stb_image_write.h (già presente)"
fi

if [ ! -f "stb_image.h" ]; then
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    echo "  ✅ stb_image.h"
else
    echo "  ✓  stb_image.h (già presente)"
fi

cd ..

echo "[3/4] Setup ambiente Python..."
mkdir -p python/nn dataset

if [ ! -d ".venv" ]; then
    python3 -m venv .venv
    echo "  ✅ venv creato"
fi

source .venv/bin/activate
pip install -q -r requirements.txt
echo "  ✅ dipendenze Python installate"
deactivate

echo "[4/4] Build..."
make

echo ""
echo "╔══════════════════════════════════╗"
echo "║   Setup completato!              ║"
echo "║                                  ║"
echo "║   Avvia con:                     ║"
echo "║   ./cantordust /path/to/file     ║"
echo "╚══════════════════════════════════╝"
