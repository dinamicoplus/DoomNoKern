#include <stdint.h>
#include <stddef.h>
#include <arch/x86/io.h>
#include <drivers/pit.h>
#include <drivers/video_vga13.h>
#include <kernel/console.h>
#include <kernel/system.h>
#include <stdio.h>

int main() {
    vga_set_mode13();
    vga13_clear(0);  // fondo negro
    
    // Dibujar una línea de gradiente
    for (int x = 0; x < 320; x++) {
        vga13_putpixel(x, 100, (uint8_t)(x & 255));
    }

    char c = getchar();
    for (int y = 60; y < 140; y++) {
        for (int x = 100; x < 220; x++) {
            vga13_putpixel(x, y, 24);  // rojo claro
        }
    }

    // Esperar 5 segundos usando el timer
    uint32_t target = pit_ticks + 500;  // 5 segundos a 100 Hz
    while (pit_ticks < target) {
        halt_cpu();  // espera eficiente
    }
    
    // Dibujar un rectángulo
    for (int y = 80; y < 120; y++) {
        for (int x = 120; x < 200; x++) {
            vga13_putpixel(x, y, 12);  // rojo claro
        }
    }
    
    // Bucle infinito
    for (;;) {
        halt_cpu();
    }
}