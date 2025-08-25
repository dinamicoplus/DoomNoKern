#pragma once
#include <stdint.h>

#define IDT_ENTRIES 256
#define KERNEL_CS   0x08   /* ajusta si tu GDT usa otros selectores */

struct idt_entry {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* idt.c */
void idt_set_gate(int n, uint32_t handler, uint16_t sel, uint8_t type_attr);
void interrupts_init(void);

/* PIC helpers (expuestos por si quieres usarlos en otros lugares) */
void pic_remap_mask_all(void);
void pic_unmask(uint8_t irq);