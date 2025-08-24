// input.c — teclado PS/2 (set 1) para QEMU/PC clásico (i386)
// - IRQ1 (vector 0x21): leer puerto 0x60 y traducir a ASCII
// - Soporta Shift y CapsLock
// - Buffer circular para stdin

#include <stdint.h>
#include <stddef.h>

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64
#define PIC1_CMD        0x20
#define PIC1_DATA       0x21
#define PIC_EOI         0x20

// --------- IO puertos ---------
static inline uint8_t inb(uint16_t port) {
    uint8_t v; __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// --------- Buffer de teclado ---------
#define KBD_BUF_SIZE 256
static char kbd_buf[KBD_BUF_SIZE];
static volatile unsigned kbd_head=0, kbd_tail=0;

static void kbd_put(char c) {
    unsigned next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next != kbd_tail) { kbd_buf[kbd_head] = c; kbd_head = next; }
}
static int kbd_get(void) { // -1 si vacío
    if (kbd_head == kbd_tail) return -1;
    char c = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return (int)c;
}

// --------- Estado de modificadores ---------
static int s_left_shift = 0, s_right_shift = 0;
static int s_caps = 0;

// --------- Tablas scancode set 1 (make codes 0x00..0x58 aprox.) ---------
static const char sc_unshift[128] = {
    /*0x00*/ 0,   27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', /* Backspace */
    /*0x10*/ '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,   /* Enter, Ctrl */
    /*0x20*/ 'a','s','d','f','g','h','j','k','l',';','\'','`', 0,  '\\','z','x',
    /*0x30*/ 'c','v','b','n','m',',','.','/', 0,    0,   0,    ' ', 0,   0,   0,   0,
    /*0x40*/ 0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-',
    /*0x50*/ '4','5','6','+','1','2','3','0','.', 0,0,0,0,0,0,0
};
static const char sc_shift[128] = {
    /*0x00*/ 0,   27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    /*0x10*/ '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    /*0x20*/ 'A','S','D','F','G','H','J','K','L',':','"','~', 0,  '|','Z','X',
    /*0x30*/ 'C','V','B','N','M','<','>','?', 0,    0,   0,    ' ', 0,   0,   0,   0,
    /*0x40*/ 0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-',
    /*0x50*/ '4','5','6','+','1','2','3','0','.', 0,0,0,0,0,0,0
};

// --------- API pública ---------

// Desenmascara IRQ1 en la PIC (asume PIC por defecto, sin APIC)
void input_init(void) {
    uint8_t imr = inb(PIC1_DATA);
    imr &= ~(1u << 1);       // bit 1 = IRQ1
    outb(PIC1_DATA, imr);
}

// Llamar desde tu ISR para el vector 0x21 (IRQ1)
void kbd_isr(void) {
    uint8_t status = inb(KBD_STATUS_PORT);
    // Bit 0 = Output buffer full
    if (status & 1) {
        static int seen_e0 = 0; // ignoramos extendidos por simplicidad
        uint8_t sc = inb(KBD_DATA_PORT);

        if (sc == 0xE0) { seen_e0 = 1; goto eoi; } // extendido (teclas cursores, etc.)
        if (sc & 0x80) { // break (soltada)
            uint8_t make = sc & 0x7F;
            if (make == 0x2A) s_left_shift  = 0;   // LShift
            if (make == 0x36) s_right_shift = 0;   // RShift
            // ignorar otras
        } else { // make (pulsada)
            if (sc == 0x2A) { s_left_shift  = 1; goto eoi; }   // LShift
            if (sc == 0x36) { s_right_shift = 1; goto eoi; }   // RShift
            if (sc == 0x3A) { s_caps ^= 1;       goto eoi; }   // CapsLock (toggle)

            char ch = 0;
            int shift = (s_left_shift || s_right_shift) ? 1 : 0;

            // Elegir tabla
            if (shift) ch = (sc < 128) ? sc_shift[sc] : 0;
            else       ch = (sc < 128) ? sc_unshift[sc] : 0;

            // Aplicar CapsLock sólo a letras
            if (!shift && s_caps) {
                if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
            } else if (shift && s_caps) {
                if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
            }

            if (ch) {
                // Traducir Enter a '\n' ya viene así en tabla
                kbd_put(ch);
            }
        }
    }
eoi:
    // End Of Interrupt al PIC maestro
    outb(PIC1_CMD, PIC_EOI);
}

// Lee 1 carácter si hay (no bloqueante). Devuelve -1 si vacío.
int kbd_getchar(void) {
    return kbd_get();
}

// Lee hasta 'max' bytes disponibles (no bloqueante). Devuelve cantidad leída.
size_t kbd_read(char *dst, size_t max) {
    size_t n = 0;
    while (n < max) {
        int c = kbd_get();
        if (c < 0) break;
        dst[n++] = (char)c;
    }
    return n;
}
