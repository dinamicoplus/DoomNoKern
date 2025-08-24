// kernel.c (freestanding)
#include <stdint.h>
#define VGA_TEXT ((volatile uint16_t*)0xB8000)

static inline uint16_t vga_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}

void puts_at(const char* s, int row) {
    volatile uint16_t* v = VGA_TEXT + row * 80;
    uint8_t attr = 0x07; // gris sobre negro
    while (*s) *v++ = vga_entry(*s++, attr);
}

void kernel_main(void) {
    // Limpia primera línea
    for (int i=0; i<80; ++i) VGA_TEXT[i] = vga_entry(' ', 0x07);
    puts_at("Hola, bare-metal en PC (VGA texto) :)", 0);
    for(;;) { __asm__ __volatile__("hlt"); }
}
