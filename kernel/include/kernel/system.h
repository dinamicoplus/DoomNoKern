/**
 * @file system.h
 * @brief Funciones básicas del sistema
 */
#ifndef KERNEL_SYSTEM_H
#define KERNEL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el sistema de interrupciones
 * 
 * Configura la IDT, remapea la PIC y desenmascara IRQ0 e IRQ1
 */

/**
 * @brief Habilita las interrupciones
 */
static inline void enable_interrupts(void) {
    __asm__ __volatile__("sti");
}

/**
 * @brief Deshabilita las interrupciones
 */
static inline void disable_interrupts(void) {
    __asm__ __volatile__("cli");
}

/**
 * @brief Espera hasta la siguiente interrupción
 */
static inline void halt_cpu(void) {
    __asm__ __volatile__("hlt");
}

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_SYSTEM_H */