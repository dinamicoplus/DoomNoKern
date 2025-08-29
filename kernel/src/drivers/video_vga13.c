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

static uint8_t g_paletteRGB[256*3];

static inline uint8_t to_dac6(uint8_t x){ return (uint8_t)((x * 63 + 127) / 255); } // 0..255 -> 0..63

void vga13_build_palette(void){
    int i = 0;

    // 0..15: reserva (negro en este ejemplo)
    for (; i < 16; ++i) {
        g_paletteRGB[i*3+0] = 0;
        g_paletteRGB[i*3+1] = 0;
        g_paletteRGB[i*3+2] = 0;
    }

    // 16..231: cubo 6x6x6 (R,G,B en 0..5)
    for (int r=0; r<6; ++r)
    for (int g=0; g<6; ++g)
    for (int b=0; b<6; ++b) {
        uint8_t R = to_dac6((uint8_t)(r * 51)); // 0,51,102,153,204,255
        uint8_t G = to_dac6((uint8_t)(g * 51));
        uint8_t B = to_dac6((uint8_t)(b * 51));
        g_paletteRGB[i*3+0] = R;
        g_paletteRGB[i*3+1] = G;
        g_paletteRGB[i*3+2] = B;
        ++i;
    }

    // 232..255: escala de grises (24 niveles)
    for (int k=0; i < 256; ++i, ++k) {
        uint8_t v8  = (uint8_t)((k * 255) / 23);
        uint8_t v6  = to_dac6(v8);
        g_paletteRGB[i*3+0] = v6;
        g_paletteRGB[i*3+1] = v6;
        g_paletteRGB[i*3+2] = v6;
    }

    // Cargar al DAC (0..255), cada valor ya está 0..63
    vga13_set_palette_range(0, g_paletteRGB, 256);
}

// Quantización: 32bpp (0xAARRGGBB) -> índice 0..255 del cubo/grises
uint8_t rgb32_to_index(uint32_t argb){
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >>  8) & 0xFF;
    uint8_t b = (argb >>  0) & 0xFF;

    // map 0..255 -> 0..5
    uint8_t R = (uint8_t)((r * 5 + 127) / 255);
    uint8_t G = (uint8_t)((g * 5 + 127) / 255);
    uint8_t B = (uint8_t)((b * 5 + 127) / 255);

    return (uint8_t)(16 + R*36 + G*6 + B); // índice dentro del cubo
}

// Dibuja un píxel 32bpp en VGA 256c
static inline void vga13_putpixel_rgb32(int x, int y, uint32_t argb){
    uint8_t idx = rgb32_to_index(argb);
    vga13_putpixel(x, y, idx);
}