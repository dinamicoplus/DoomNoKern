# Rutas
export ZIG="$(which zig)"
export ZTGT="x86-freestanding"
export TARGET_TRIPLE="i686-elf"
export SYSROOT="$HOME/repositories/zig-qemu-kernel/sysroot"             # el de newlib
export LIBDIR="$SYSROOT/$TARGET_TRIPLE/lib"
export INCDIR="$SYSROOT/$TARGET_TRIPLE/include"

# Compilar C (kernel + stubs)
$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c stubs.c -o stubs.o

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c kern.c -o kern.o

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c ramdisk.c -o ramdisk.o

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c cpio.c -o cpio.o


# Compilar asm 32-bit
$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector \
  -m80387 -c boot32.S -o boot.o

# Enlace (pasa las .a directamente; evita -lc filtrado)
$ZIG cc -target $ZTGT -nostdlib -nostartfiles -static \
  -Wl,--gc-sections -Wl,-T,linker32.ld  \
  boot.o stubs.o kern.o ramdisk.o cpio.o\
  "$LIBDIR/libc.a" "$LIBDIR/libm.a" "$LIBDIR/libfatfs.a" \
  -o kernel.elf
