// idt.c (corregido: máscara estricta; solo IRQ1 abierta)
#include <stdint.h>

#define IDT_ENTRIES 256
#define KERNEL_CS   0x08      // ajusta si tu GDT usa otros selectores

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

static struct idt_entry idt[IDT_ENTRIES];

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }

static void idt_set_gate(int n, uint32_t handler, uint16_t sel, uint8_t type_attr){
    idt[n].offset_lo = (uint16_t)(handler & 0xFFFF);
    idt[n].selector  = sel;
    idt[n].zero      = 0;
    idt[n].type_attr = type_attr;      // 0x8E = present|DPL0|32-bit interrupt gate
    idt[n].offset_hi = (uint16_t)(handler >> 16);
}

static void lidt(void *idtr){
    __asm__ volatile("lidtl (%0)" :: "r"(idtr));
}

/* Remapea PIC: maestro base=0x20, esclavo base=0x28, y ENMASCARA TODO */
static void pic_remap_mask_all(void){
    outb(0x20, 0x11);               // ICW1
    outb(0xA0, 0x11);
    outb(0x21, 0x20);               // ICW2: offsets
    outb(0xA1, 0x28);
    outb(0x21, 0x04);               // ICW3
    outb(0xA1, 0x02);
    outb(0x21, 0x01);               // ICW4
    outb(0xA1, 0x01);

    // Máscara completa: ninguna IRQ habilitada
    outb(0x21, 0xFF);               // IMR maestro
    outb(0xA1, 0xFF);               // IMR esclavo
}

/* Desenmascara una IRQ concreta (0..15) */
static void pic_unmask(uint8_t irq){
    uint16_t port = (irq < 8) ? 0x21 : 0xA1;
    uint8_t  bit  = irq & 7;
    uint8_t  m    = inb(port);
    m &= (uint8_t)~(1u << bit);
    outb(port, m);
}

/* API pública: inicializa IDT con IRQ1->irq1_stub y abre SOLO IRQ1 */
extern void irq0_stub(void);
extern void irq1_stub(void);
void interrupts_init(void){
    // limpia IDT
    for (int i=0;i<IDT_ENTRIES;i++) idt[i] = (struct idt_entry){0};

    // remapea PIC y deja TODO enmascarado
    pic_remap_mask_all();

    // instala entrada 0x21 (IRQ1 teclado)
		idt_set_gate(0x20, (uint32_t)irq0_stub, KERNEL_CS, 0x8E);
    idt_set_gate(0x21, (uint32_t)irq1_stub, KERNEL_CS, 0x8E);

    // carga IDT
    struct idt_ptr idtr = { .limit = sizeof(idt)-1, .base = (uint32_t)idt };
    lidt(&idtr);

    // abre únicamente IRQ1 (teclado)
		pic_unmask(0);
    pic_unmask(1);
}
