#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Inicializa el backend (opcional). Devuelve 0 si ok. */
    int   (*init)(void);
    /* Lee un carácter si hay (-1 si vacío). */
    int   (*getchar)(void);
    /* Lee hasta 'max' bytes disponibles (no bloqueante). */
    size_t (*read)(char* dst, size_t max);
    /* Apaga/libera (opcional). */
    void  (*shutdown)(void);
} stdin_ops_t;

/* Backend por defecto: teclado PS/2 */
extern const stdin_ops_t STDIN_PS2;

void stdin_init(const stdin_ops_t* ops);   // llama ops->init si existe
void stdin_set_backend(const stdin_ops_t* ops);
int   stdin_getchar(void);
size_t stdin_read(char* dst, size_t max);

#ifdef __cplusplus
}
#endif