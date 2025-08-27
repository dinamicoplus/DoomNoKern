#pragma once
#include <stddef.h>
#include <stdint.h>
#include <kernel/stdin.h>   // para stdin_ops_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*clear)(void);
    void (*putc)(char);
    void (*write)(const char* s, size_t n);
} console_ops_t;

extern const console_ops_t CONSOLE_TEXT;
extern const console_ops_t CONSOLE_VGA13;

/* stdio modes */
typedef enum {
    CONSOLE_STDIO_NONE = 0,      /* no tocar stdio */
    CONSOLE_STDIO_UNBUFFERED,    /* stdout/stderr sin buffer */
    CONSOLE_STDIO_LINE_BUFFERED  /* stdout line buffered, stderr unbuffered */
} console_stdio_mode_t;

/* Inicialización básica (sólo backend de salida) */
void console_init(const console_ops_t* ops);
void console_set_backend(const console_ops_t* ops);
void console_clear(void);
void console_putc(char c);
void console_write(const char* s, size_t n);

/* Bootstrap completo: salida+entrada+stdio */
void console_init_all(const console_ops_t* out_ops,
                      const stdin_ops_t*    in_ops,
                      console_stdio_mode_t  stdio_mode);

#ifdef __cplusplus
}
#endif