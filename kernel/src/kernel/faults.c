// src/kernel/faults.c — manejadores C para faults/IRQ de diagnóstico
#include <stdint.h>
#include <stdio.h>

static void hex32(uint32_t x){
    const char*h="0123456789ABCDEF";
    for(int i=7;i>=0;i--) putchar(h[(x>>(i*4))&0xF]);
}
static void hex16(uint16_t x){
    const char*h="0123456789ABCDEF";
    for(int i=3;i>=0;i--) putchar(h[(x>>(i*4))&0xF]);
}

/* Handler común para faults SIN #MF (recibe vec y err “normalizado”) */
void fault_handler(uint32_t vec, uint32_t err){
    printf("\nEXCEPTION vec="); hex32(vec);
    printf(" err=");            hex32(err);
    printf("\nHALT\n");
    for(;;) __asm__ __volatile__("hlt");
}

/* #MF (x87 FP error): leer CW/SW/TW con FNSTENV, limpiar y (si quieres) continuar.
   De momento mostramos info y nos paramos para depurar. */
struct fenv16 {
    uint16_t cw, sw, tw, ipoff, cssel, dataoff, datasel;
    uint16_t pad[4];
};

void mf_handler(void){
    struct fenv16 env;
    __asm__ volatile("fnstenv %0" : "=m"(env));  /* también limpia ES */
    __asm__ volatile("fwait");

    printf("\n#MF cw="); hex16(env.cw);
    printf(" sw=");      hex16(env.sw);
    printf(" tw=");      hex16(env.tw);
    printf(" ip=");      hex16(env.cssel); putchar(':'); hex16(env.ipoff);
    printf(" dp=");      hex16(env.datasel); putchar(':'); hex16(env.dataoff);
    printf("\n");

    /* Reestablece CW “benigna” por si venía agresiva */
    {
        uint16_t cw = 0x037F;        /* excepciones enmascaradas */
        __asm__ volatile("fldcw %0"::"m"(cw));
    }

    for(;;) __asm__ __volatile__("hlt");
}