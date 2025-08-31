#pragma once
#include <kernel/console.h>

/* Exporta un ops listo para console_set_backend(&CONSOLE_TEXT) */
extern const console_ops_t CONSOLE_TEXT;

/* Si quieres usarlo directo, también puedes exponer helpers aquí */
void vga_disable_cursor(void);

void vga_enable_cursor(uint8_t start, uint8_t end);

void vga_init_text_mode(void);