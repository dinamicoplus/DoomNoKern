// pit.h
#pragma once
#include <stdint.h>

void pit_init(uint32_t hz);
void pit_isr(void);                   // llamada desde el stub de IRQ0

extern volatile uint32_t pit_ticks;   // ticks desde boot (incrementa en IRQ0)
