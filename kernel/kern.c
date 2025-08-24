// kern.c
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "fat12_img.h"
#include "stubs.h"
#include "input.h"
#include "pit.h"
#include "vga13.h"

extern int kbd_getchar(void);  // de input.c (no bloqueante: -1 si no hay tecla)

// lee una línea bloqueando (eco en pantalla), termina en '\0' sin incluir '\n'
static int getline_kbd(char *buf, size_t max) {
    size_t len = 0;
    for (;;) {
        int k = kbd_getchar();
        if (k < 0) { __asm__ __volatile__("hlt"); continue; }   // espera activa suave
        char c = (char)k;

        if (c == '\r') c = '\n';
        if (c == '\n') { putchar('\n'); buf[len] = '\0'; return (int)len; }

        if (c == '\b') {                    // backspace
            if (len) { len--; fputs("\b \b", stdout); }
            continue;
        }

        if (len + 1 < max) {                // deja sitio para '\0'
            buf[len++] = c;
            putchar(c);                      // eco
        }
        // si se llena, ignora más caracteres hasta Enter
    }
}

// ejemplo: pedir un entero usando sscanf
int scan_int(const char *prompt, int *out_value) {
    char line[64];
    fputs(prompt, stdout);
    int n = getline_kbd(line, sizeof(line));
    if (n <= 0) return -1;
    if (sscanf(line, "%d", out_value) == 1) return 0;
    return -1;
}

void interrupts_init(void);
/*
// uso en tu kernel_main
void kernel_main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
		clear_scrn();

    interrupts_init();       // instala IDT + remapea PIC + desenmascara IRQ1
		pit_init(100);
    __asm__ __volatile__("sti");   // <--- habilita interrupciones globales

    int x;
    if (scan_int("Introduce un entero: ", &x) == 0) {
        printf("Leido: %d\n", x);
    } else {
        puts("Entrada invalida.");
    }

    uint32_t start = pit_ticks;
    uint32_t target = start + 5u * 100u;   // 5 s * 100 Hz = 500 ticks
    while (pit_ticks < target) {
        __asm__ __volatile__("hlt");       // duerme hasta la siguiente IRQ (timer/teclado)
    }

    puts("Han pasado 5 segundos!");

    for(;;) __asm__ __volatile__("hlt");
}
*/
void kernel_main(void){
    vga_set_mode13();
    vga13_clear(0);                 // fondo negro
    // dibujar algo
    for (int x=0;x<320;x++) vga13_putpixel(x,100,(uint8_t)(x&255));

    // si usas PIT: espera 5 s y pinta un rectángulo
    extern volatile uint32_t pit_ticks;
    extern void pit_init(uint32_t hz);
    extern void interrupts_init(void);

    interrupts_init(); pit_init(100); __asm__ __volatile__("sti");
    uint32_t target = pit_ticks + 500;
    while (pit_ticks < target) __asm__ __volatile__("hlt");

    // rectángulo
    for (int y=80;y<120;y++) for (int x=120;x<200;x++) vga13_putpixel(x,y,12); // rojo claro
    for(;;) __asm__ __volatile__("hlt");
}
