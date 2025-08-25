/**
 * @file idt.c
 * @brief Configuración de la Interrupt Descriptor Table (IDT) para x86
 */
#include <arch/x86/io.h>
#include <arch/x86/idt.h>
#include <arch/x86/faults.h>

static struct idt_entry idt[IDT_ENTRIES];

void idt_set_gate(int n, uint32_t handler, uint16_t sel, uint8_t type_attr){
    idt[n].offset_lo = (uint16_t)(handler & 0xFFFF);
    idt[n].selector  = sel;
    idt[n].zero      = 0;
    idt[n].type_attr = type_attr;      // 0x8E = present|DPL0|32-bit interrupt gate
    idt[n].offset_hi = (uint16_t)(handler >> 16);
}

static void lidt(void *idtr){
    __asm__ volatile("lidtl (%0)" :: "r"(idtr));
}

/* Remapea PIC: maestro base=0x20, esclavo base=0x28, enmascara todo */
void pic_remap_mask_all(void){
    outb(0x20, 0x11); outb(0xA0, 0x11);   // ICW1
    outb(0x21, 0x20); outb(0xA1, 0x28);   // ICW2
    outb(0x21, 0x04); outb(0xA1, 0x02);   // ICW3
    outb(0x21, 0x01); outb(0xA1, 0x01);   // ICW4
    outb(0x21, 0xFF);                     // máscara total
    outb(0xA1, 0xFF);
}

void pic_unmask(uint8_t irq){
    uint16_t port = (irq < 8) ? 0x21 : 0xA1;
    uint8_t  bit  = irq & 7;
    uint8_t  m    = inb(port);
    m &= (uint8_t)~(1u << bit);
    outb(port, m);
}

void interrupts_init(void){
    for (int i=0; i<IDT_ENTRIES; i++)
        idt[i] = (struct idt_entry){0};

    pic_remap_mask_all();

    /* Faults básicas */
    idt_set_gate(0x06, (uint32_t)isr6_stub,  KERNEL_CS, 0x8E); // #UD
    idt_set_gate(0x07, (uint32_t)isr7_stub,  KERNEL_CS, 0x8E); // #NM
    idt_set_gate(0x10, (uint32_t)isr10_stub, KERNEL_CS, 0x8E); // #MF
    idt_set_gate(0x0D, (uint32_t)isr13_stub, KERNEL_CS, 0x8E); // #GP
    idt_set_gate(0x0E, (uint32_t)isr14_stub, KERNEL_CS, 0x8E); // #PF

    /* IRQs que sí usamos */
    idt_set_gate(0x20, (uint32_t)irq0_stub,  KERNEL_CS, 0x8E); // PIT
    idt_set_gate(0x21, (uint32_t)irq1_stub,  KERNEL_CS, 0x8E); // KBD

    struct idt_ptr idtr = { .limit = sizeof(idt)-1, .base = (uint32_t)idt };
    lidt(&idtr);

    pic_unmask(0);   // timer
    pic_unmask(1);   // teclado
}