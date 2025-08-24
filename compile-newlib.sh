# Directorio de build separado
mkdir -p build-newlib-i686
cd build-newlib-i686

export TARGET_TRIPLE=i686-elf        # nombre "GNU" del target para newlib
export ZIG_TARGET=x86-freestanding  # nombre del target para zig cc

../newlib-4.5.0.20241231/configure \
  --target="$TARGET_TRIPLE" \
  --prefix="$PREFIX" \
  --disable-nls \
  --disable-werror \
  --disable-multilib \
  --disable-newlib-supplied-syscalls \
  --enable-newlib-retargetable-locking \
  --disable-libgloss

# Compilar e instalar la libc y libm del target usando Zig como toolchain
make -j"$(sysctl -n hw.ncpu)" \
  CC_FOR_TARGET="$ZIG cc -target $ZIG_TARGET" \
  AS_FOR_TARGET="$ZIG cc -target $ZIG_TARGET -c" \
  AR_FOR_TARGET="$ZIG ar" \
  RANLIB_FOR_TARGET="$ZIG ranlib" \
  CFLAGS_FOR_TARGET="-ffreestanding -fno-builtin -fno-stack-protector -fno-pic \
                     -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-exceptions \
                     -Os -pipe -fno-omit-frame-pointer -mno-red-zone -mno-mmx -mno-sse -mno-sse2" \
  MAKEINFO=true

make install \
  CC_FOR_TARGET="$ZIG cc -target $ZIG_TARGET" \
  AS_FOR_TARGET="$ZIG cc -target $ZIG_TARGET -c" \
  AR_FOR_TARGET="$ZIG ar" \
  RANLIB_FOR_TARGET="$ZIG ranlib"
