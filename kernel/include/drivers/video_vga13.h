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
 * @brief Deshabilita el cursor hardware
 */
void vga_disable_cursor(void);

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

extern const console_ops_t CONSOLE_VGA13;

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_VGA13_H */