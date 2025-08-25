/* src/arch/faults.S — CPU faults (x86, 32-bit) */
.global isr6_stub, isr7_stub, isr13_stub, isr14_stub, isr10_stub
.extern fault_handler
.extern mf_handler

/* Selectores de segmento (ajusta a tu GDT) */
.set KERNEL_DS, 0x10

/* -------- #MF (x87 Floating-Point Error), vector 0x10 --------
   No empuja error-code; llamamos a mf_handler() directamente. */
isr10_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $KERNEL_DS, %ax
    mov %ax, %ds
    mov %ax, %es
    cld
    call mf_handler
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    iret

/* Macros:
   - MAKE_ISR_NOERR vec : para excepciones SIN error code
        -> push 0 (err), push vec, llamar fault_handler(vec, err)
   - MAKE_ISR_ERR vec   : para excepciones CON error code
        -> lee err (está en top of stack al entrar), lo pasa a C,
           y ANTES de iret descarta el error code hardware con "add $4, %esp".
*/
.macro SAVE_REGS
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $KERNEL_DS, %ax
    mov %ax, %ds
    mov %ax, %es
    cld
.endm

.macro RESTORE_REGS
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
.endm

.macro MAKE_ISR_NOERR vec
.global isr\vec\()_stub
isr\vec\()_stub:
    SAVE_REGS
    push $0              /* err = 0 */
    push $\vec           /* vec */
    call fault_handler   /* void fault_handler(uint32_t vec, uint32_t err) */
    add $8, %esp         /* limpia args */
    RESTORE_REGS
    iret
.endm

.macro MAKE_ISR_ERR vec
.global isr\vec\()_stub
isr\vec\()_stub:
    /* Pila al entrar (sin cambiar nada):
       [err][EIP][CS][EFLAGS]  <- ESP
       Capturamos err antes de ensuciar offsets. */
    mov (%esp), %eax     /* eax = err hardware */
    SAVE_REGS
    push %eax            /* err */
    push $\vec           /* vec */
    call fault_handler
    add $8, %esp         /* limpia args */
    RESTORE_REGS
    add $4, %esp         /* *** descarta error-code hardware ***
                            ahora en top quedan [EIP][CS][EFLAGS] */
    iret
.endm

/* #UD(6), #NM(7): sin error code */
MAKE_ISR_NOERR 6
MAKE_ISR_NOERR 7

/* #GP(13), #PF(14): con error code */
MAKE_ISR_ERR 13
MAKE_ISR_ERR 14