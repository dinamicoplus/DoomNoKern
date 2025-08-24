// vga13.c — set 320x200x256 (modo 0x13) en protected mode (sin int10h)
#include <stdint.h>

static inline uint8_t inb(uint16_t p){ uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }

#define VGA_MEM      ((volatile uint8_t*)0xA0000)
#define VGA_W        320
#define VGA_H        200

static void vga_write_seq(uint8_t idx, uint8_t val){ outb(0x3C4, idx); outb(0x3C5, val); }
static void vga_write_crt(uint8_t idx, uint8_t val){ outb(0x3D4, idx); outb(0x3D5, val); }
static void vga_write_gc (uint8_t idx, uint8_t val){ outb(0x3CE, idx); outb(0x3CF, val); }
static void vga_write_ac (uint8_t idx, uint8_t val){ (void)inb(0x3DA); outb(0x3C0, idx); outb(0x3C0, val); }

void vga_disable_cursor(void){ outb(0x3D4,0x0A); outb(0x3D5,0x20); }

void vga_set_mode13(void){
    // Misc Output: seleccionar reloj/puertos para 320x200
    outb(0x3C2, 0x63);

    // Sequencer
    vga_write_seq(0x00, 0x03); // reset
    vga_write_seq(0x01, 0x01); // clocking mode
    vga_write_seq(0x02, 0x0F); // map mask
    vga_write_seq(0x03, 0x00); // char map select
    vga_write_seq(0x04, 0x0E); // memory mode (chain4)

    // Desbloquear CRTC (bit7 de reg 0x11)
    outb(0x3D4, 0x11);
    uint8_t v = inb(0x3D5); outb(0x3D5, v & ~0x80);

    // CRTC (valores estándar de 13h)
    static const uint8_t crt[] = {
        0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,0x00,0x41,
        0x00,0x00,0x00,0x00,0x00,0x00,0x9C,0x8E,0x8F,0x28,
        0x1F,0x96,0xB9,0xA3,0xFF
    };
    for (uint8_t i=0;i<25;i++) vga_write_crt(i, crt[i]);

    // Graphics Controller
    vga_write_gc(0x00,0x00); vga_write_gc(0x01,0x00);
    vga_write_gc(0x02,0x00); vga_write_gc(0x03,0x00);
    vga_write_gc(0x04,0x00); vga_write_gc(0x05,0x40); // sequential, write mode 0
    vga_write_gc(0x06,0x05); // A0000 segment
    vga_write_gc(0x07,0x0F); vga_write_gc(0x08,0xFF);

    // Attribute Controller: paleta 0..15 identidad y modo gráfico 256c
    for (uint8_t i=0;i<16;i++) vga_write_ac(i, i);
    vga_write_ac(0x10, 0x41); // mode control: graphics, no blink
    vga_write_ac(0x11, 0x00); // overscan
    vga_write_ac(0x12, 0x0F); // color plane enable
    vga_write_ac(0x13, 0x00); // horiz pixel panning
    vga_write_ac(0x14, 0x00); // color select
    (void)inb(0x3DA); outb(0x3C0, 0x20); // re-enable video

    vga_disable_cursor();
}

// Helpers simples para dibujar
void vga13_clear(uint8_t color){
    for (int i=0;i<VGA_W*VGA_H;i++) VGA_MEM[i] = color;
}
void vga13_putpixel(int x,int y,uint8_t c){
    if ((unsigned)x<VGA_W && (unsigned)y<VGA_H) VGA_MEM[y*VGA_W + x] = c;
}
