/**
 * @file input.h
 * @brief Interfaz de teclado PS/2 (set 1) para kernel i386
 */
#ifndef DRIVERS_INPUT_H
#define DRIVERS_INPUT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el driver de entrada (desenmascara IRQ1)
 */
void kbd_init(void);

/**
 * @brief Rutina de servicio de interrupción para el teclado (IRQ1)
 */
void kbd_isr(void);

/**
 * @brief Obtiene un carácter si hay uno disponible (no bloqueante)
 * @return Carácter ASCII (≥0) o -1 si el buffer está vacío
 */
int kbd_getchar(void);

/**
 * @brief Lee hasta 'max' bytes disponibles del buffer de teclado (no bloqueante)
 * @param dst Buffer destino
 * @param max Tamaño máximo a leer
 * @return Cantidad de bytes leídos
 */
size_t kbd_read(char *dst, size_t max);

/**
 * @brief Tamaño del buffer interno de teclado
 */
#define KBD_BUF_SIZE 256

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INPUT_H */