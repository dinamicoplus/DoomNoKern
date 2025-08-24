/* linker32.ld — kernel ELF i386 (freestanding) */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

KERNEL_BASE = 0x00100000;   /* 1 MiB */

SECTIONS
{
  . = KERNEL_BASE;

  __kernel_start = .;

  .text : ALIGN(16) {
    /* Mantén los headers Multiboot */
    KEEP(*(.multiboot))
    KEEP(*(.multiboot2))
    *(.text .text.*)
  }

  .rodata : ALIGN(16) { *(.rodata .rodata.*) }
  .data   : ALIGN(16) { *(.data .data.*) }

  .bss (NOLOAD) : ALIGN(16) {
    *(.bss .bss.*)
    *(COMMON)
  }

  . = ALIGN(16);
  _end = .;
  __kernel_end = .;

  /DISCARD/ : { *(.comment) *(.eh_frame) *(.note .note.*) }
}
