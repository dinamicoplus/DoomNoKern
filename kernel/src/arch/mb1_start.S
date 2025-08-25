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

.global _start
_start:
    jmp _mb1_start

_mb1_start:
    /* Ya estamos en 32-bit (Limine/GRUB). EAX=2BADB002, EBX=mbi */
    cli
    cld

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