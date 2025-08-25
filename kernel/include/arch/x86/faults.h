#pragma once
#include <stdint.h>

/* C handlers (definidos en faults.c) */
void fault_handler(uint32_t vec, uint32_t err);
void mf_handler(void);

/* ASM stubs (definidos en faults.S / irq_stubs.S) */
void isr6_stub(void);   /* #UD */
void isr7_stub(void);   /* #NM */
void isr10_stub(void);  /* #MF */
void isr13_stub(void);  /* #GP */
void isr14_stub(void);  /* #PF */

void irq0_stub(void);
void irq1_stub(void);