/* boot32.S — Multiboot1 + entrada 32-bit con pila correcta en .bss */
.section .multiboot
.align 4
.set MB_MAGIC,  0x1BADB002
.set MB_FLAGS,  0x00000003         /* ALIGN|MEMINFO */
.set MB_CHECK, -(MB_MAGIC + MB_FLAGS)
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECK

.section .text
.global _start
.extern kernel_main

_start:
    cli

    /* Cargar una pila válida (tope de la reserva en .bss) */
    mov $stack_top, %esp
    and $-16, %esp                  /* alinear por si acaso */

    call kernel_main

.hang:
    hlt
    jmp .hang

/* ---------- Pila en .bss (NO usar .comm aquí) ---------- */
.section .bss
.balign 16
stack_bottom:
    .space 16384                    /* 16 KiB */
.balign 16
stack_top:
