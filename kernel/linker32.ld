/* linker32.ld — kernel ELF i386 (freestanding) */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_mb1_start)

KERNEL_BASE = 0x00100000;   /* 1 MiB */

SECTIONS
{
  . = KERNEL_BASE;

  __kernel_start = .;

  .text : ALIGN(16) {
    KEEP(*(.multiboot))
    KEEP(*(.multiboot2))
    *(.text .text.*)
  }

  .rodata : ALIGN(16) { *(.rodata .rodata.*) }
  .data   : ALIGN(16) { *(.data .data.*) }

  .bss (NOLOAD) : ALIGN(16) {
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    __bss_end = .;
  }

  /* Coloca la pila al final, en su propia sección */
  .stack (NOLOAD) : ALIGN(16) {
    __stack_bottom = .;
    . = . + 256K;             /* reserva 64 KiB */
    __stack_top = .;
  }
  . = ALIGN(16);
  _end = .;
  __kernel_end = .;

  /DISCARD/ : { *(.comment) *(.eh_frame) *(.note .note.*) }
}