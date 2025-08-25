/**
 * @file pit.c
 * @brief Implementación del driver para el Programmable Interval Timer (PIT)
 */
#include <drivers/pit.h>
#include <arch/x86/io.h>

// PIT (8253/8254) ports
#define PIT_CH0_DATA    0x40
#define PIT_MODE_CMD    0x43
#define PIT_FREQ        1193182     // Base frequency ~1.193182 MHz

// PIC ports
#define PIC1_CMD        0x20
#define PIC_EOI         0x20

// Contador global de ticks
volatile uint32_t pit_ticks = 0;

void pit_init(uint32_t hz) {
    // Calcular divisor para la frecuencia deseada
    uint32_t divisor = PIT_FREQ / hz;
    if (divisor > 65535) divisor = 65535;
    
    // Modo 3 (square wave), acceso 16-bit, canal 0
    outb(PIT_MODE_CMD, 0x36);
    
    // Escribir divisor (LSB primero, luego MSB)
    outb(PIT_CH0_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0_DATA, (uint8_t)(divisor >> 8));
}

void pit_isr(void) {
    // Incrementar contador de ticks
    pit_ticks++;
    
    // Enviar EOI al PIC maestro
    outb(PIC1_CMD, PIC_EOI);
}