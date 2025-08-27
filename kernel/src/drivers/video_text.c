// src/drivers/video_text.c — VGA texto 80x25 con cursor HW
#include <stdint.h>
#include <stddef.h>
#include <kernel/console.h>
#include <arch/x86/io.h>     // inb/outb

#define VGA_MEM   ((volatile uint16_t*)0xB8000)
#define VGA_W     80
#define VGA_H     25

static uint8_t  vga_attr = 0x07;     // gris sobre negro
static int      vga_row  = 0;
static int      vga_col  = 0;

static inline uint16_t vga_entry(char ch, uint8_t attr){
    return (uint16_t)ch | ((uint16_t)attr << 8);
}

/* ---- Cursor hardware (tuyas, usando io.h) ---- */
static inline void vga_disable_cursor(void){
    outb(0x3D4, 0x0A);
    uint8_t val = inb(0x3D5) | 0x20;   // set bit5 = disable
    outb(0x3D5, val);
}

static inline void vga_enable_cursor(uint8_t start, uint8_t end){
    outb(0x3D4, 0x0A);
    uint8_t val = (inb(0x3D5) & 0xC0) | (start & 0x1F);
    outb(0x3D5, val);
    outb(0x3D4, 0x0B);
    outb(0x3D5, end & 0x1F);
}

static inline void vga_update_cursor(int row, int col){
    uint16_t pos = (uint16_t)(row * VGA_W + col);
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));     // low
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(pos >> 8));       // high
}

/* ---- Backend console: clear/putc/write ---- */
static void vga_clear(void){
    uint16_t blank = vga_entry(' ', vga_attr);
    for (int r=0; r<VGA_H; ++r)
        for (int c=0; c<VGA_W; ++c)
            VGA_MEM[r*VGA_W + c] = blank;

    vga_row = 0; vga_col = 0;
    vga_enable_cursor(14, 15);           // cursor visible (ajusta si quieres)
    vga_update_cursor(0, 0);
}

static void vga_backspace(void) {  
    if (vga_col > 0) {
        vga_col--;
        //VGA_MEM[vga_row*VGA_W + vga_col] = vga_entry(' ', vga_attr);
        vga_update_cursor(vga_row, vga_col);
    }
    return;
}

static void vga_putc(char ch){
    if (ch == '\r'){ vga_col = 0; vga_update_cursor(vga_row, vga_col); return; }
    if (ch == '\n'){
        vga_col = 0;
        if (++vga_row >= VGA_H) vga_row = 0;     // wrap simple (sin scroll)
        vga_update_cursor(vga_row, vga_col);
        return;
    }
    if (ch == '\b') { vga_backspace(); return; }

    VGA_MEM[vga_row*VGA_W + vga_col] = vga_entry(ch, vga_attr);
    if (++vga_col >= VGA_W){
        vga_col = 0;
        if (++vga_row >= VGA_H) vga_row = 0;
    }
    vga_update_cursor(vga_row, vga_col);
}

static void vga_write(const char* s, size_t n){
    for (size_t i=0; i<n; ++i) vga_putc(s[i]);
}

/* ---- Exporta el backend para console_set_backend(&CONSOLE_TEXT) ---- */
const console_ops_t CONSOLE_TEXT = {
    .clear = vga_clear,
    .putc  = vga_putc,
    .write = vga_write,
};