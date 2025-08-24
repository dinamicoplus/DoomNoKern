/* boot.S */
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .text
.global _start
.extern kernel_main

_start:
    mov $stack_top, %esp
    call kernel_main

.hang:
    hlt
    jmp .hang

.section .bss
.align 16
.lcomm stack, 0x4000
.global stack_top
stack_top:
