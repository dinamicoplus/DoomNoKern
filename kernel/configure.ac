AC_INIT([kernel], [0.1])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([.])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

dnl --- Opciones/vars ---
AC_ARG_VAR([TARGET_TRIPLE], [GNU target triple (default i686-elf)])
: ${TARGET_TRIPLE=i686-elf}

AC_ARG_VAR([ZTGT], [Zig/Clang target triple for --target (default x86-freestanding)])
: ${ZTGT=x86-freestanding}

dnl Permite overridear rutas de include/lib si quieres
AC_ARG_VAR([INCDIR],  [Include dir (default include)])
: ${INCDIR=include}
AC_ARG_VAR([LIBDIR],  [Lib dir (default lib)])
: ${LIBDIR=lib}
AC_ARG_VAR([SRCDIR],  [Src dir (default src)])
: ${SRCDIR=src}
AC_ARG_VAR([BUILDDIR],[Build dir (default build)])
: ${BUILDDIR=build}

dnl --- Detección de herramientas ---
AC_PATH_PROG([ZIG],   [zig])
AC_PATH_PROG([CLANG], [clang])
AC_PATH_PROG([LLVM_AR],     [llvm-ar])
AC_PATH_PROG([LLVM_RANLIB], [llvm-ranlib])

dnl Selección del toolchain:
if test -n "$ZIG"; then
  CC="$ZIG cc -target $ZTGT"
  AS="$ZIG cc -target $ZTGT -c"
  AR="$ZIG ar"
  RANLIB="$ZIG ranlib"
  echo "Using Zig toolchain (target=$ZTGT)"
elif test -n "$CLANG"; then
  CC="$CLANG --target=$TARGET_TRIPLE"
  AS="$CLANG --target=$TARGET_TRIPLE -c"
  AR="${LLVM_AR:-${TARGET_TRIPLE}-ar}"
  RANLIB="${LLVM_RANLIB:-${TARGET_TRIPLE}-ranlib}"
  echo "Using Clang/LLVM toolchain (target=$TARGET_TRIPLE)"
else
  CC="${TARGET_TRIPLE}-gcc"
  AS="${TARGET_TRIPLE}-gcc -c"
  AR="${TARGET_TRIPLE}-ar"
  RANLIB="${TARGET_TRIPLE}-ranlib"
  echo "Using ${TARGET_TRIPLE}-gcc toolchain"
fi

dnl --- Flags por defecto (permiten override vía entorno) ---
: ${CFLAGS="-g -O2 -ffreestanding -fno-stack-protector -fno-builtin -m80387 -MMD -MP"}
: ${ASFLAGS="-ffreestanding -fno-stack-protector -m80387"}
: ${LDFLAGS="-nostdlib -nostartfiles -static -Wl,--gc-sections -Wl,-T,linker32.ld"}

dnl Añade includes que tu Makefile esperaba
CFLAGS="$CFLAGS -I$INCDIR -I$INCDIR/newlib -I$INCDIR/fatfs"

dnl Si usamos Zig o Clang con --target, ya incorporamos el target arriba;
dnl si quieres forzar también en LDFLAGS/ASFLAGS para Clang, puedes añadir:
case "$CC" in
  *"--target="*) : ;;
  *) : ;;
esac

AC_SUBST([TARGET_TRIPLE])
AC_SUBST([ZTGT])
AC_SUBST([SRCDIR])
AC_SUBST([INCDIR])
AC_SUBST([BUILDDIR])
AC_SUBST([LIBDIR])

AC_SUBST([CC])
AC_SUBST([AS])
AC_SUBST([AR])
AC_SUBST([RANLIB])
AC_SUBST([CFLAGS])
AC_SUBST([ASFLAGS])
AC_SUBST([LDFLAGS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
