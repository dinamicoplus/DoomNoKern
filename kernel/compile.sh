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

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c idt.c -o idt.o

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c input.c -o input.o

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c pit.c -o pit.o

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c vga13.c -o vga13.o

# Compilar asm 32-bit
$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector \
  -m80387 -c boot32.S -o boot.o

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector \
  -m80387 -c irq_stubs.S -o irq_stubs.o

# Enlace (pasa las .a directamente; evita -lc filtrado)
$ZIG cc -target $ZTGT -nostdlib -nostartfiles -static \
  -Wl,--gc-sections -Wl,-T,linker32.ld  \
	irq_stubs.o idt.o pit.o vga13.o\
  boot.o stubs.o kern.o ramdisk.o cpio.o input.o\
  "$LIBDIR/libc.a" "$LIBDIR/libm.a" "$LIBDIR/libfatfs.a" \
  -o kernel.elf
