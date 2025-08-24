/* linker32.ld — ELF i386, base 1MiB */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

KERNEL_BASE = 0x00100000;

SECTIONS
{
  . = KERNEL_BASE;

  .text : ALIGN(16) {
    KEEP(*(.multiboot))
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

  /DISCARD/ : { *(.eh_frame) *(.comment) *(.note .note.*) }
}
