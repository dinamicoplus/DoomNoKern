#pragma once
#include <kernel/console.h>

/* Exporta un ops listo para console_set_backend(&CONSOLE_TEXT) */
extern const console_ops_t CONSOLE_TEXT;

/* Si quieres usarlo directo, también puedes exponer helpers aquí */
static inline void vga_disable_cursor(void);

static inline void vga_enable_cursor(uint8_t start, uint8_t end);