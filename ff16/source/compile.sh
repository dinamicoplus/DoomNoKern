# Rutas
export ZIG="$(which zig)"
export ZTGT="x86-freestanding"
export TARGET_TRIPLE="i686-elf"
export SYSROOT="$HOME/repositories/zig-qemu-kernel/sysroot"             # el de newlib
export INCDIR="$SYSROOT/$TARGET_TRIPLE/include"
export LIBDIR="$SYSROOT/$TARGET_TRIPLE/lib"

# Compilar C (kernel + stubs)
$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c diskio.c -o diskio.o

$ZIG cc -target $ZTGT -ffreestanding -fno-stack-protector -fno-builtin \
  -m80387 -O2 -I"$INCDIR" -c ff.c -o ff.o

$ZIG ar rcs libfatfs.a diskio.o ff.o
cp libfatfs.a "$LIBDIR/libfatfs.a"
cp *.h "$INCDIR"
