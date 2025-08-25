/* src/arch/irq_stubs.S — IRQ0 (timer) y IRQ1 (teclado) */
.global irq0_stub, irq1_stub
.extern pit_isr
.extern kbd_isr

.set KERNEL_DS, 0x10

.macro IRQ_STUB target
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $KERNEL_DS, %ax
    mov %ax, %ds
    mov %ax, %es
    cld
    call \target
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    iret
.endm

irq0_stub: IRQ_STUB pit_isr
irq1_stub: IRQ_STUB kbd_isr