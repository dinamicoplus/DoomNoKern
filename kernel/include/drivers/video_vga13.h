/**
 * @file vga13.h
 * @brief Interfaz para modo gráfico VGA 13h (320x200x256 colores)
 */
#ifndef DRIVERS_VGA13_H
#define DRIVERS_VGA13_H

#include <stdint.h>
#include <kernel/console.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dimensiones del modo gráfico
 */
#define VGA13_W   320
#define VGA13_H   200

/**
 * @brief Dirección base del framebuffer VGA
 */
#define VGA13_FB  ((volatile uint8_t*)0xA0000)  // planar chain-4, 64 KiB

/**
 * @brief Establece el modo gráfico 13h (320x200x256 colores)
 */
void vga_set_mode13(void);
/**
 * @brief Limpia la pantalla con un color específico
 * @param color Índice de color (0-255)
 */
void vga13_clear(uint8_t color);

/**
 * @brief Dibuja un pixel en coordenadas específicas
 * @param x Coordenada X
 * @param y Coordenada Y
 * @param color Índice de color (0-255)
 */
void vga13_putpixel(int x, int y, uint8_t color);

/**
 * @brief Dibuja una línea horizontal
 * @param x Coordenada X inicial
 * @param y Coordenada Y
 * @param w Ancho de la línea en píxeles
 * @param color Índice de color (0-255)
 */
void vga13_hline(int x, int y, int w, uint8_t color);

/**
 * @brief Dibuja una línea vertical
 * @param x Coordenada X
 * @param y Coordenada Y inicial
 * @param h Alto de la línea en píxeles
 * @param color Índice de color (0-255)
 */
void vga13_vline(int x, int y, int h, uint8_t color);

/**
 * @brief Dibuja un rectángulo relleno
 * @param x Coordenada X inicial
 * @param y Coordenada Y inicial
 * @param w Ancho del rectángulo
 * @param h Alto del rectángulo
 * @param color Índice de color (0-255)
 */
void vga13_rect_fill(int x, int y, int w, int h, uint8_t color);

/**
 * @brief Configura una entrada de la paleta de colores
 * @param index Índice del color (0-255)
 * @param r Componente rojo (0-63)
 * @param g Componente verde (0-63)
 * @param b Componente azul (0-63)
 */
void vga13_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Configura un rango de la paleta de colores
 * @param start Índice inicial
 * @param rgb Puntero a array de tripletas RGB (valores 0-63)
 * @param count Número de colores a configurar
 */
void vga13_set_palette_range(uint8_t start, const uint8_t* rgb, int count);

void vga13_build_palette(void);

uint8_t rgb32_to_index(uint32_t argb);

/**
 * @brief Builds an enhanced 8x8x4 RGB color palette using all 256 colors
 */
void vga13_build_enhanced_palette(void);

/**
 * @brief Converts 32-bit RGBA color to enhanced palette index (0-255)
 * @param rgba 32-bit RGBA color (0xRRGGBBAA)
 * @return Palette index in the enhanced 8x8x4 color cube
 */
uint8_t rgb32_to_enhanced_index(uint32_t rgba);

/**
 * @brief Draws a pixel with 32-bit RGBA color using the enhanced palette
 * @param x Coordinate X
 * @param y Coordinate Y
 * @param argb 32-bit RGBA color (0xRRGGBBAA)
 */
static inline void vga13_putpixel_enhanced_rgb32(int x, int y, uint32_t argb);

static inline void vga13_putpixel_rgb32(int x, int y, uint32_t argb);

extern const console_ops_t CONSOLE_VGA13;

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_VGA13_H */