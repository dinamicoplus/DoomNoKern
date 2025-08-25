#include <kernel/console.h>
#include <kernel/system.h>
#include <arch/x86/idt.h>
#include <stdio.h>
#include <disk/fat12_img.h>
#include <arch/x86/io.h>

extern int main(void);   // tu main() en src/main.c

static inline void soft_test_irq1(void){
    __asm__ __volatile__("int $0x21");
}

void kernel_main(void){
    interrupts_init();
    console_init_all(&CONSOLE_TEXT, &STDIN_PS2, CONSOLE_STDIO_UNBUFFERED);
    enable_interrupts();
    console_clear();
    soft_test_irq1();  // DEBUG: prueba de IRQ1 (teclado)
    while(1);
    //main();
}