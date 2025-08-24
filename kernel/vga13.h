// vga13.h — 320x200x256 (modo 13h) en protected mode (sin BIOS)
#ifndef VGA13_H_
#define VGA13_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Dimensiones y framebuffer
#define VGA13_W   320
#define VGA13_H   200
#define VGA13_FB  ((volatile uint8_t*)0xA0000)  // planar chain-4, 64 KiB

// Inicialización / control
void vga_set_mode13(void);          // programa registros VGA a 320x200x256
void vga_disable_cursor(void);      // oculta el cursor hardware (CRTC)

// Primitivas básicas
void vga13_clear(uint8_t color);                    // rellena toda la pantalla
void vga13_putpixel(int x, int y, uint8_t color);   // pixel (con bounds check)

// (Opcional) utilidades rápidas
void vga13_hline(int x, int y, int w, uint8_t color);
void vga13_vline(int x, int y, int h, uint8_t color);
void vga13_rect_fill(int x, int y, int w, int h, uint8_t color);

// Paleta (valores 0..63 por componente)
void vga13_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
// Carga un bloque de 'count' entradas empezando en 'start'.
// rgb apunta a tripletas {r,g,b} (cada una 0..63).
void vga13_set_palette_range(uint8_t start, const uint8_t* rgb, int count);

#ifdef __cplusplus
}
#endif
#endif
