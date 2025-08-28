#!/usr/bin/env bash
set -euo pipefail

# --- Parámetros del build ---
BUILD_DIR="build-fatfs-i686"
TARGET_TRIPLE="i686-elf"          # tripleta GNU para newlib (target)
PREFIX_DIR="kernel"               # instalación relativa a la raíz del repo
ZIG_TARGET="x86-freestanding"     # sólo si usamos Zig
