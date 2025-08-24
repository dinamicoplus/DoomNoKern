// pit.c — PIT 8253/8254 en modo rate generator, IRQ0 cada (1/hz) s
#include <stdint.h>

#define PIT_FREQ 1193182u
#define PIT_CH0  0x40
#define PIT_CMD  0x43

#define PIC1_CMD 0x20
#define PIC_EOI  0x20

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }

volatile uint32_t pit_ticks = 0;

void pit_init(uint32_t hz){
    if (hz == 0) hz = 100;
    uint16_t div = (uint16_t)(PIT_FREQ / hz);

    // Canal 0, low/high, modo 2 (rate generator), binario
    outb(PIT_CMD, 0x34);
    outb(PIT_CH0, (uint8_t)(div & 0xFF));
    outb(PIT_CH0, (uint8_t)(div >> 8));
}

void pit_isr(void){
    pit_ticks++;
    // EOI al PIC maestro
    outb(PIC1_CMD, PIC_EOI);
}
