/**
 * @file vga13.c
 * @brief Implementación del driver para modo gráfico VGA 13h
 */
#include <drivers/video_vga13.h>
#include <arch/x86/io.h>

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
        0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,0x00,0xC1,
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

void vga13_hline(int x, int y, int w, uint8_t color) {
    if (y < 0 || y >= VGA_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > VGA_W) w = VGA_W - x;
    if (w <= 0) return;
    
    volatile uint8_t* dst = VGA_MEM + (y * VGA_W) + x;
    for (int i = 0; i < w; i++) *dst++ = color;
}

void vga13_vline(int x, int y, int h, uint8_t color) {
    if (x < 0 || x >= VGA_W) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > VGA_H) h = VGA_H - y;
    if (h <= 0) return;
    
    volatile uint8_t* dst = VGA_MEM + (y * VGA_W) + x;
    for (int i = 0; i < h; i++) {
        *dst = color;
        dst += VGA_W;
    }
}

void vga13_rect_fill(int x, int y, int w, int h, uint8_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > VGA_W) w = VGA_W - x;
    if (y + h > VGA_H) h = VGA_H - y;
    if (w <= 0 || h <= 0) return;
    
    volatile uint8_t* dst = VGA_MEM + (y * VGA_W) + x;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) dst[i] = color;
        dst += VGA_W;
    }
}

void vga13_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r);
    outb(0x3C9, g);
    outb(0x3C9, b);
}

void vga13_set_palette_range(uint8_t start, const uint8_t* rgb, int count) {
    outb(0x3C8, start);
    for (int i = 0; i < count * 3; i++) {
        outb(0x3C9, rgb[i]);
    }
}