#!/usr/bin/env bash
set -euo pipefail

# --- Parámetros del build ---
BUILD_DIR="build-newlib-i686"
TARGET_TRIPLE="i686-elf"          # tripleta GNU para newlib (target)
PREFIX_DIR="kernel"               # instalación relativa a la raíz del repo
ZIG_TARGET="x86-freestanding"     # sólo si usamos Zig

# --- Preparar dirs ---
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
PREFIX="$(realpath "$PWD/../$PREFIX_DIR")"

# --- Selección condicional del toolchain ---
if command -v zig >/dev/null 2>&1; then
  echo "→ Usando Zig como toolchain para $TARGET_TRIPLE"
  export CC_FOR_TARGET="zig cc -target $ZIG_TARGET"
  export AS_FOR_TARGET="zig cc -target $ZIG_TARGET -c"
  export AR_FOR_TARGET="zig ar"
  export RANLIB_FOR_TARGET="zig ranlib"
  export CFLAGS_FOR_TARGET="-ffreestanding -fno-builtin -fno-stack-protector -fno-pic \
                            -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-exceptions \
                            -Os -pipe -fno-omit-frame-pointer -mno-red-zone -mno-mmx -mno-sse -mno-sse2"

elif command -v clang >/dev/null 2>&1; then
  echo "→ Usando Clang/LLVM como toolchain para $TARGET_TRIPLE"
  export CC_FOR_TARGET="clang --target=$TARGET_TRIPLE"
  export AS_FOR_TARGET="clang --target=$TARGET_TRIPLE -c"
  export AR_FOR_TARGET="llvm-ar"
  export RANLIB_FOR_TARGET="llvm-ranlib"
  export CFLAGS_FOR_TARGET="-ffreestanding -fno-builtin -fno-stack-protector -fno-pic \
                            -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-exceptions \
                            -Os -pipe -fno-omit-frame-pointer -mno-red-zone -mno-mmx -mno-sse -mno-sse2"

else
  echo "→ Usando toolchain por defecto de configure (${TARGET_TRIPLE}-gcc, etc.)"
fi

# Evitar que flags del *host* contaminen el *target*
unset CFLAGS || true
unset CXXFLAGS || true
unset CPPFLAGS || true
unset LDFLAGS || true

# --- Configure ---
../newlib/configure \
  --target="$TARGET_TRIPLE" \
  --prefix="$PREFIX" \
  --disable-nls \
  --disable-werror \
  --disable-multilib \
  --disable-newlib-supplied-syscalls \
  --enable-newlib-retargetable-locking \
  --disable-libgloss

# --- Paralelismo portable ---
if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
  JOBS="$(sysctl -n hw.ncpu)"
else
  JOBS=4
fi

# --- Build & install ---
make -j"$JOBS" MAKEINFO=true
make install

echo
echo "✔ Newlib instalada en: $PREFIX"
