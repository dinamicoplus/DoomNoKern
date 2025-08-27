#include <kernel/console.h>
#include <drivers/keyboard.h>
#include <kernel/system.h>
#include <arch/x86/idt.h>
#include <stdio.h>
#include <disk/fat12_img.h>
#include <arch/x86/io.h>
#include <drivers/pit.h>

extern int main(void);   // tu main() en src/main.c

static inline void soft_test_irq1(void){
    __asm__ __volatile__("int $0x21");
}

void kernel_main(void){
    interrupts_init();
    console_init_all(&CONSOLE_TEXT, &STDIN_PS2, CONSOLE_STDIO_UNBUFFERED);
    kbd_set_layout(KBD_LAYOUT_ES);
    enable_interrupts();
    console_clear();
    pit_init(100);  // 100 Hz
    main();
}