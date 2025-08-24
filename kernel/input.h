// input.h — interfaz de teclado PS/2 (set 1) para kernel i386
// Proporciona inicialización, ISR y lectura no bloqueante para stdin.

#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>   // size_t

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el driver de entrada:
   - Desenmascara IRQ1 en la PIC.
   - No habilita interrupciones globales (no hace 'sti'). */
void input_init(void);

/* Rutina de servicio de interrupción para el teclado (IRQ1).
   - Llama a esta función desde el manejador del vector de IDT que asigne a IRQ1.
   - Si usas el mapeo estándar de la PIC, suele ser 0x21 (33 decimal). */
void kbd_isr(void);

/* Obtiene un carácter si hay uno disponible (no bloqueante).
   Devuelve:
     >= 0 : carácter ASCII
       -1 : si el buffer está vacío */
int kbd_getchar(void);

/* Lee hasta 'max' bytes disponibles del buffer de teclado (no bloqueante).
   Devuelve la cantidad leída (0 si no había datos). */
size_t kbd_read(char *dst, size_t max);

/* Tamaño del buffer interno (por si quieres mostrar estado/diagnóstico).
   No es obligatorio usarlo. */
#define KBD_BUF_SIZE 256

#ifdef __cplusplus
} // extern "C"
#endif

#endif // INPUT_H
