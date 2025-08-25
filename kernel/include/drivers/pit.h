/**
 * @file pit.h
 * @brief Interfaz para el Programmable Interval Timer (PIT) 8253/8254
 */
#ifndef DRIVERS_PIT_H
#define DRIVERS_PIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el PIT a una frecuencia específica
 * @param hz Frecuencia en hercios (Hz)
 */
void pit_init(uint32_t hz);

/**
 * @brief Rutina de servicio de interrupción para el PIT (IRQ0)
 */
void pit_isr(void);

/**
 * @brief Contador de ticks desde el inicio del sistema
 */
extern volatile uint32_t pit_ticks;

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_PIT_H */