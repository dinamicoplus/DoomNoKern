/* mb1_start.S — entrada Multiboot1 minimal y robusta (32-bit) */
.section .multiboot
.align 4
.set MB_MAGIC,  0x1BADB002
.set MB_FLAGS,  0x00000003            /* ALIGN | MEMINFO */
.set MB_CHECK, -(MB_MAGIC + MB_FLAGS)
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECK

.section .text
.global _mb1_start
.extern __bss_start
.extern __bss_end
.extern __stack_top
.extern kernel_main

/* gdt.S (o al principio de tu boot.S) */
    .p2align 3
gdt:
    .quad 0x0000000000000000          /* 0x00: null */
    .quad 0x00CF9A000000FFFF          /* 0x08: code 32-bit, base=0, limit=4GiB */
    .quad 0x00CF92000000FFFF          /* 0x10: data 32-bit, base=0, limit=4GiB */
gdt_end:

gdt_ptr:
    .word gdt_end - gdt - 1
    .long gdt

/* Selectores */
.set GDT_CS, 0x08
.set GDT_DS, 0x10

.global _start
_start:
    jmp _mb1_start

_mb1_start:
    /* Ya estamos en 32-bit (Limine/GRUB). EAX=2BADB002, EBX=mbi */
    cli
    cld

    /* 1) Cargar GDT propia (plana) y recargar TODOS los segment registers */
    lgdt gdt_ptr

    mov $GDT_DS, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* Recarga CS con far jump */
    ljmp $GDT_CS, $1f
1:
    /* Pila propia y alineada */
    mov $__stack_top, %esp
    and $-16, %esp

    /* Limpia .bss */
    mov $__bss_start, %edi
    mov $__bss_end,   %ecx
    sub %edi, %ecx                /* ECX = size */
    xor %eax, %eax
    rep stosb

    /* x87: MP=1, NE=1, EM=0, TS=0 (no tocamos CR4 todavía) */
    mov %cr0, %eax
    and $~(1<<2), %eax            /* EM=0 */
    and $~(1<<3), %eax            /* TS=0 */
    or  $(1<<1), %eax             /* MP=1 */
    or  $(1<<5), %eax             /* NE=1 (#MF en #10) */
    mov %eax, %cr0
    fninit
    sub $8, %esp
    movw $0x037F, (%esp)          /* enmascara excepciones x87 */
    fldcw (%esp)
    add $8, %esp

    mov %cr4, %eax
    or  $(1<<9),  %eax      /* OSFXSR=1: habilita XMM/SSE */
    or  $(1<<10), %eax      /* OSXMMEXCPT=1: excepciones XMM por #XF */
    mov %eax, %cr4

    /* Llama a C */
    call kernel_main

.hang:
    hlt
    jmp .hang