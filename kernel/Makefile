# =====================
# Variables generales
# =====================
TARGET_TRIPLE = i686-elf
ZTGT          = x86-freestanding

SRCDIR        = src
INCDIR        = include
BUILDDIR      = build
LIBDIR        = lib

CC            = /usr/bin/clang --target=i686-elf
AS            = /usr/bin/clang --target=i686-elf -c
AR            = /usr/bin/llvm-ar
RANLIB        = /usr/bin/llvm-ranlib
CFLAGS        = -g -O2 -ffreestanding -fno-stack-protector -fno-builtin -m80387 -MMD -MP -Iinclude -Iinclude/newlib -Iinclude/fatfs
ASFLAGS       = -ffreestanding -fno-stack-protector -m80387
LDFLAGS       = -nostdlib -nostartfiles -static -Wl,--gc-sections -Wl,-T,linker32.ld -Wl,-z,noexecstack -Wl,--build-id=none

# =====================
# Directorios de origen
# =====================
KERNEL_SRC    = $(SRCDIR)/kernel
DRIVERS_SRC   = $(SRCDIR)/drivers
ARCH_SRC      = $(SRCDIR)/arch

# =====================
# Directorios de build
# =====================
KERNEL_BUILD  = $(BUILDDIR)/kernel
DRIVERS_BUILD = $(BUILDDIR)/drivers
ARCH_BUILD    = $(BUILDDIR)/arch

# =====================
# Librerías
# =====================
LIBS          = $(LIBDIR)/libm.a $(LIBDIR)/libfatfs.a
LIBC          = $(LIBDIR)/libc.a

# =====================
# Fuentes
# =====================
KERNEL_C      = $(wildcard $(KERNEL_SRC)/*.c)
DRIVERS_C     = $(wildcard $(DRIVERS_SRC)/*.c)
ARCH_C        = $(wildcard $(ARCH_SRC)/*.c)
ARCH_S        = $(wildcard $(ARCH_SRC)/*.S)
MAIN_C        = $(SRCDIR)/main.c

# =====================
# Objetos
# =====================
KERNEL_OBJ    = $(patsubst $(KERNEL_SRC)/%.c,$(KERNEL_BUILD)/%.o,$(KERNEL_C))
DRIVERS_OBJ   = $(patsubst $(DRIVERS_SRC)/%.c,$(DRIVERS_BUILD)/%.o,$(DRIVERS_C))
ARCH_OBJ_C    = $(patsubst $(ARCH_SRC)/%.c,$(ARCH_BUILD)/%.o,$(ARCH_C))
ARCH_OBJ_S    = $(patsubst $(ARCH_SRC)/%.S,$(ARCH_BUILD)/%.o,$(ARCH_S))
MAIN_OBJ      = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(MAIN_C))

OBJS          = $(KERNEL_OBJ) $(DRIVERS_OBJ) $(ARCH_OBJ_C) $(ARCH_OBJ_S) $(MAIN_OBJ)
DEPS          = $(OBJS:.o=.d)

# =====================
# Target final
# =====================
KERNEL        = kernel.elf

# =====================
# Reglas
# =====================
.PHONY: all clean
all: $(KERNEL)

$(KERNEL): $(OBJS)
	@echo "Enlazando $@..."
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) $(LIBC) -o $@

# --- Compilar C ---
$(KERNEL_BUILD)/%.o: $(KERNEL_SRC)/%.c | $(KERNEL_BUILD)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(DRIVERS_BUILD)/%.o: $(DRIVERS_SRC)/%.c | $(DRIVERS_BUILD)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCH_BUILD)/%.o: $(ARCH_SRC)/%.c | $(ARCH_BUILD)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(MAIN_OBJ): $(MAIN_C) | $(BUILDDIR)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# --- Compilar ASM ---
$(ARCH_BUILD)/%.o: $(ARCH_SRC)/%.S | $(ARCH_BUILD)
	@echo "Ensamblando $<..."
	$(AS) $(ASFLAGS) $< -o $@

# --- Crear directorios ---
$(KERNEL_BUILD) $(DRIVERS_BUILD) $(ARCH_BUILD):
	mkdir -p $@

# --- Limpiar ---
clean:
	@echo "Limpiando..."
	rm -rf $(BUILDDIR) $(KERNEL)

# Incluir dependencias de headers
-include $(DEPS)
