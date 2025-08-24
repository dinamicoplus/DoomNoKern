/* irq_stubs.S — wrapper para IRQ1 (teclado) */
.global irq0_stub
.global irq1_stub
.extern pit_isr
.extern kbd_isr

/* Segmento de datos del kernel (asume GDT típica: CS=0x08, DS=0x10) */
.set KERNEL_DS, 0x10

irq0_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $KERNEL_DS, %ax
    mov %ax, %ds
    mov %ax, %es
    call pit_isr
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    iret

irq1_stub:
    /* La CPU ya empujó EIP, CS, EFLAGS */
    pusha                      /* eax,ecx,edx,ebx,esp,ebp,esi,edi */
    push %ds
    push %es
    push %fs
    push %gs

    /* Asegura DS/ES válidos para C */
    mov $KERNEL_DS, %ax
    mov %ax, %ds
    mov %ax, %es

    call kbd_isr               /* tu rutina C lee 0x60, mete en buffer y EOI */

    /* Restaurar y salir */
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    iret
