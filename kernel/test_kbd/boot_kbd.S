/* boot_kbd.S — i386, Multiboot1, IRQ1 teclado -> VGA texto
   Ensamblar con clang/zig cc (GAS/AT&T). */

.set KBD_VEC, 0x21          /* IRQ1 tras remap (PIC master offset 0x20) */

.section .multiboot
.align 4
.set MB_MAGIC,  0x1BADB002
.set MB_FLAGS,  0x00000003          /* ALIGN | MEMINFO */
.set MB_CHECK, -(MB_MAGIC + MB_FLAGS)
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECK

.section .text
.global _start

/* -------- util I/O -------- */
.macro outb port, reg
    mov    $\port, %dx
    out    %al, %dx
.endm

.macro inb port
    mov    $\port, %dx
    in     %dx, %al
.endm

/* pequeño delay para PIC */
.io_wait:
    push   %dx
    mov    $0x80, %dx
    mov    $0, %al
    out    %al, %dx
    pop    %dx
    ret

/* -------- inicio -------- */
_start:
    cli

    /* segmentos planos típicos de GRUB: CS=0x08, DS/ES/SS=0x10 */
    mov    $0x10, %ax
    mov    %ax, %ds
    mov    %ax, %es
    mov    %ax, %ss

    /* pila */
    mov    $stack_top, %esp

    /* limpiar pantalla y cursor */
    call   clear_screen
    movl   $0, cursor_row
    movl   $0, cursor_col

    /* remapear PIC a 0x20/0x28 */
    mov    $0x20, %dx
    mov    $0x11, %al        /* ICW1 */
    out    %al, %dx
    call   .io_wait
    mov    $0xA0, %dx
    mov    $0x11, %al
    out    %al, %dx
    call   .io_wait

    mov    $0x21, %dx        /* ICW2 master: 0x20 */
    mov    $0x20, %al
    out    %al, %dx
    call   .io_wait
    mov    $0xA1, %dx        /* ICW2 slave: 0x28 */
    mov    $0x28, %al
    out    %al, %dx
    call   .io_wait

    mov    $0x21, %dx        /* ICW3 master: IR2 = slave */
    mov    $0x04, %al
    out    %al, %dx
    call   .io_wait
    mov    $0xA1, %dx        /* ICW3 slave: ID en IR2 */
    mov    $0x02, %al
    out    %al, %dx
    call   .io_wait

    mov    $0x21, %dx        /* ICW4 8086 mode */
    mov    $0x01, %al
    out    %al, %dx
    call   .io_wait
    mov    $0xA1, %dx
    mov    $0x01, %al
    out    %al, %dx
    call   .io_wait

    /* enmascara todo salvo IRQ1 (teclado) en master; todo en slave */
    mov    $0x21, %dx
    mov    $0xFD, %al        /* 11111101b -> IRQ1 habilitado */
    out    %al, %dx
    mov    $0xA1, %dx
    mov    $0xFF, %al
    out    %al, %dx

    /* IDT: pon el gate del IRQ1 */
    call   idt_init

    /* habilita interrupciones */
    sti

.hang:
    hlt
    jmp    .hang

/* -------- IDT -------- */
.type set_gate,@function
set_gate: /* args: edi = ptr entrada, eax = handler addr */
    /* entrada de 8 bytes:
       offset[15:0], selector, zero, type, offset[31:16] */
    movw   %ax,   0(%edi)        /* offset low */
    movw   $0x08, 2(%edi)        /* CS selector */
    movb   $0x00, 4(%edi)
    movb   $0x8E, 5(%edi)        /* P=1, DPL=0, 32-bit interrupt gate */
    shr    $16,  %eax
    movw   %ax,   6(%edi)        /* offset high */
    ret

idt_init:
    /* limpia IDT a cero */
    lea    idt_table, %edi
    mov    $((256*8)/4), %ecx
    xor    %eax, %eax
    rep    stosl

    /* set gate para KBD_VEC */
    lea    idt_table, %edi
    add    $(KBD_VEC*8), %edi
    mov    $kbd_isr, %eax
    call   set_gate

    /* cargar IDT */
    lea    idt_desc, %eax
    lidt   (%eax)
    ret

/* -------- ISR teclado (IRQ1 -> vector 0x21) -------- */
.global kbd_isr
kbd_isr:
    pusha

    /* lee scancode */
    mov    $0x60, %dx
    in     %dx, %al
    movzx  %al, %ebx           /* EBX = scancode */

    /* ignorar extendidos (0xE0) y "release" (bit7=1) */
    cmp    $0xE0, %al
    je     .eoi
    test   $0x80, %al
    jnz    .eoi

    /* traducir scancode -> AL=ascii (0 si no mapeado) */
    push   %ebx
    call   scancode_to_ascii
    add    $4, %esp
    test   %al, %al
    jz     .eoi                 /* nada que imprimir */

    /* escribir en VGA según char */
    cmp    $'\n', %al
    je     .newline
    cmp    $'\b', %al
    je     .backspace

    /* pos = row*80 + col -> dirección = 0xB8000 + pos*2 */
    mov    cursor_row, %ecx
    imul   $80, %ecx, %ecx
    mov    cursor_col, %edx
    add    %edx, %ecx           /* ECX = pos */
    lea    (%ecx,%ecx,1), %ecx  /* pos*2 */
    mov    $0xB8000, %edi
    add    %ecx, %edi           /* EDI = dirección del par (char,attr) */

    mov    %al, %ah
    mov    $0x07, %al           /* attr en AL, char en AH -> queremos [lo=char, hi=attr] */
    xchg   %al, %ah             /* ahora AL=char, AH=attr */
    movw   %ax, (%edi)

    /* avanzar cursor */
    incb    cursor_col
    cmpb    $80, cursor_col
    jl     .eoi
    movl   $0,  cursor_col
    incb    cursor_row
    cmpb    $25, cursor_row
    jl     .eoi
    movl   $0, cursor_row
    jmp    .eoi

.newline:
    movl   $0, cursor_col
    incb    cursor_row
    cmpb    $25, cursor_row
    jl     .eoi
    movl   $0, cursor_row
    jmp    .eoi

.backspace:
    /* retrocede y borra */
    cmpb    $0, cursor_col
    je     .eoi
    decb   cursor_col
    mov    cursor_row, %ecx
    imul   $80, %ecx, %ecx
    add    cursor_col, %ecx
    lea    (%ecx,%ecx,1), %ecx
    mov    $0xB8000, %edi
    add    %ecx, %edi
    movw   $(' ' | (0x07<<8)), (%edi)
    jmp    .eoi

.eoi:
    /* EOI al PIC master */
    mov    $0x20, %dx
    mov    $0x20, %al
    out    %al, %dx

    popa
    iret

/* -------- tabla de traducción (set 1, min) -------- */
/* scancode -> ascii; solo unas teclas básicas */
.type scancode_to_ascii,@function
scancode_to_ascii:
    push   %ebp
    mov    %esp, %ebp
    mov    8(%ebp), %ebx       /* EBX = scancode (zero-extended) */

    mov    $0, %eax

    /* dígitos fila superior */
    cmp    $0x02, %bl; je .c1
    cmp    $0x03, %bl; je .c2
    cmp    $0x04, %bl; je .c3
    cmp    $0x05, %bl; je .c4
    cmp    $0x06, %bl; je .c5
    cmp    $0x07, %bl; je .c6
    cmp    $0x08, %bl; je .c7
    cmp    $0x09, %bl; je .c8
    cmp    $0x0A, %bl; je .c9
    cmp    $0x0B, %bl; je .c0

    /* alfabeto principal */
    cmp    $0x10, %bl; je .cq
    cmp    $0x11, %bl; je .cw
    cmp    $0x12, %bl; je .ce
    cmp    $0x13, %bl; je .cr
    cmp    $0x14, %bl; je .ct
    cmp    $0x15, %bl; je .cy
    cmp    $0x16, %bl; je .cu
    cmp    $0x17, %bl; je .ci
    cmp    $0x18, %bl; je .co
    cmp    $0x19, %bl; je .cp

    cmp    $0x1E, %bl; je .ca
    cmp    $0x1F, %bl; je .cs
    cmp    $0x20, %bl; je .cd
    cmp    $0x21, %bl; je .cf
    cmp    $0x22, %bl; je .cg
    cmp    $0x23, %bl; je .ch
    cmp    $0x24, %bl; je .cj
    cmp    $0x25, %bl; je .ck
    cmp    $0x26, %bl; je .cl

    cmp    $0x2C, %bl; je .cz
    cmp    $0x2D, %bl; je .cx
    cmp    $0x2E, %bl; je .cc
    cmp    $0x2F, %bl; je .cv
    cmp    $0x30, %bl; je .cb
    cmp    $0x31, %bl; je .cn
    cmp    $0x32, %bl; je .cm

    /* especiales */
    cmp    $0x39, %bl; je .cspace
    cmp    $0x1C, %bl; je .cNL
    cmp    $0x0E, %bl; je .cBS

    jmp    .done

.c1:  mov $'1', %al; jmp .done
.c2:  mov $'2', %al; jmp .done
.c3:  mov $'3', %al; jmp .done
.c4:  mov $'4', %al; jmp .done
.c5:  mov $'5', %al; jmp .done
.c6:  mov $'6', %al; jmp .done
.c7:  mov $'7', %al; jmp .done
.c8:  mov $'8', %al; jmp .done
.c9:  mov $'9', %al; jmp .done
.c0:  mov $'0', %al; jmp .done

.cq:  mov $'q', %al; jmp .done
.cw:  mov $'w', %al; jmp .done
.ce:  mov $'e', %al; jmp .done
.cr:  mov $'r', %al; jmp .done
.ct:  mov $'t', %al; jmp .done
.cy:  mov $'y', %al; jmp .done
.cu:  mov $'u', %al; jmp .done
.ci:  mov $'i', %al; jmp .done
.co:  mov $'o', %al; jmp .done
.cp:  mov $'p', %al; jmp .done

.ca:  mov $'a', %al; jmp .done
.cs:  mov $'s', %al; jmp .done
.cd:  mov $'d', %al; jmp .done
.cf:  mov $'f', %al; jmp .done
.cg:  mov $'g', %al; jmp .done
.ch:  mov $'h', %al; jmp .done
.cj:  mov $'j', %al; jmp .done
.ck:  mov $'k', %al; jmp .done
.cl:  mov $'l', %al; jmp .done

.cz:  mov $'z', %al; jmp .done
.cx:  mov $'x', %al; jmp .done
.cc:  mov $'c', %al; jmp .done
.cv:  mov $'v', %al; jmp .done
.cb:  mov $'b', %al; jmp .done
.cn:  mov $'n', %al; jmp .done
.cm:  mov $'m', %al; jmp .done

.cspace: mov $' ',  %al; jmp .done
.cNL:    mov $'\n', %al; jmp .done
.cBS:    mov $'\b', %al; jmp .done

.done:
    mov    %ebp, %esp
    pop    %ebp
    ret

/* -------- util: limpiar pantalla -------- */
clear_screen:
    push   %edi
    push   %ecx
    mov    $0xB8000, %edi
    mov    $(80*25), %ecx
    mov    $0x0720, %ax         /* ' ' (0x20) con atributo 0x07 */
1:  mov    %ax, (%edi)
    add    $2, %edi
    loop   1b
    pop    %ecx
    pop    %edi
    ret

/* -------- datos/IDT/pila -------- */
.section .data
.align 4
cursor_row: .long 0
cursor_col: .long 0

.align 4
idt_desc:
    .word idt_end - idt_table - 1
    .long idt_table

.align 8
idt_table:
    .space 256*8, 0
idt_end:

.section .bss
.balign 16
stack_space: .space 16384
.balign 16
stack_top:
