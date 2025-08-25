/**
 * @file keyboard.c
 * @brief Driver de teclado PS/2 (set 1) para QEMU/PC clásico (i386)
 */
#include <drivers/keyboard.h>
#include <arch/x86/io.h>
#include <stdint.h>
#include <stdio.h>

#define DEBUG_LINE puts("\nDEBUG"); while(1); /* DEBUG */

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64
#define PIC1_CMD        0x20
#define PIC1_DATA       0x21
#define PIC_EOI         0x20

// --------- Buffer de teclado ---------
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

// ===== US (tu tabla original) =====
static const char sc_us_unshift[128] = {
    /*0x00*/ 0,   27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    /*0x10*/ '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    /*0x20*/ 'a','s','d','f','g','h','j','k','l',';','\'','`', 0,  '\\','z','x',
    /*0x30*/ 'c','v','b','n','m',',','.','/', 0,    0,   0,    ' ', 0,   0,   0,   0,
    /*0x40*/ 0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-',
    /*0x50*/ '4','5','6','+','1','2','3','0','.', 0,0,0,0,0,0,0
};
static const char sc_us_shift[128] = {
    /*0x00*/ 0,   27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    /*0x10*/ '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    /*0x20*/ 'A','S','D','F','G','H','J','K','L',':','"','~', 0,  '|','Z','X',
    /*0x30*/ 'C','V','B','N','M','<','>','?', 0,    0,   0,    ' ', 0,   0,   0,   0,
    /*0x40*/ 0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-',
    /*0x50*/ '4','5','6','+','1','2','3','0','.', 0,0,0,0,0,0,0
};

// ===== ES (QWERTY España), versión ASCII “pragmática” =====
// Notas de posiciones en set1:
//  - 0x27 (US ';:')  → 'ñ'/'Ñ' (en ES físico a la derecha de 'L')
//  - 0x28 (US '\''") → tecla de acento agudo (aquí emitimos '\'' y '"')
//  - 0x29 (US '`~')  → 'º'/'ª' (aquí usamos '`' y '~' como aproximación ASCII)
//  - 0x2B (US '\|')  → (ES suele ser 'ç' en algunos layouts; mantenemos '\|')
//  - Resto: igual que US salvo pequeñas permutaciones de símbolos.
//
// Si usas VGA texto con **CP437**, puedes sustituir:
#define CP437_N_TILDE  (char)0xA4  // 'ñ'
#define CP437_N_TILDEU (char)0xA5  // 'Ñ'
#define CP437_ACUTE    (char)0xB4  // '´'
#define CP437_DIAER    (char)0xA8  // '¨'
#define CP437_MASCORD  (char)0xA7  // 'º'  (según fuente puede ser 0xA7/0xF8/…)
#define CP437_FEMORD   (char)0xA6  // 'ª'  (ajusta si tu fuente difiere)
//
// Y entonces cambia las líneas marcadas.

static const char sc_es_unshift[128] = {
    /*0x00*/ 0,   27, '1','2','3','4','5','6','7','8','9','0', '\'', '=',  0,   // 0x0C: '\'' ; 0x0D: '=' aprox
    /*0x10*/ 0,'q','w','e','r','t','y','u','i','o','p',      '[', '+',  0,  0,
    /*0x20*/ 'a','s','d','f','g','h','j','k','l',               CP437_N_TILDE, '\'', '`',   0, '\\','z','x',
    /*0x30*/ 'c','v','b','n','m',                               ',', '.',  '-',   0,  0,  0,   ' ', 0,  0,  0,  0,
    /*0x40*/ 0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-',
    /*0x50*/ '4','5','6','+','1','2','3','0','.', 0,0,0,0,0,0,0
};
// Sustituciones recomendadas si usas CP437:
//   en 0x27 (posición de ';') pon 'ñ' → CP437 0xA4
//   en shift, 'Ñ' → CP437 0xA5
//   en 0x28 (posición de '\'') pon agudo '´' y con Shift '¨' si lo prefieres.

static const char sc_es_shift[128] = {
    /*0x00*/ 0,   27, '!','"','#','$','%','&','/','(',')',     '=',  '?',  0, 0,  // 0x0C:'?'  0x0D:'=' (~aprox)
    /*0x10*/ 0,'Q','W','E','R','T','Y','U','I','O','P',     ']',  '*',  0,  0,
    /*0x20*/ 'A','S','D','F','G','H','J','K','L',              CP437_N_TILDEU,  '"',  '~',   0, '|','Z','X',
    /*0x30*/ 'C','V','B','N','M',                              ';',  ':',  '_',   0,  0,  0,   ' ', 0,  0,  0,  0,
    /*0x40*/ 0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-',
    /*0x50*/ '4','5','6','+','1','2','3','0','.', 0,0,0,0,0,0,0
};

// Nota: las filas de símbolos en ES real varían según ISO/ANSI y mapeo BIOS.
// Si quieres el ES exacto con 'ñ','¡','¿','º','ª', usa CP437 y ajusta bytes.

// ===== Selector =====
static const char *g_unshift = sc_us_unshift;
static const char *g_shift   = sc_us_shift;
static kbd_layout_t g_layout = KBD_LAYOUT_US;

void kbd_set_layout(kbd_layout_t lay){
    g_layout = lay;
    switch (lay){
    case KBD_LAYOUT_ES:
        g_unshift = sc_es_unshift;
        g_shift   = sc_es_shift;
        break;
    case KBD_LAYOUT_US:
    default:
        g_unshift = sc_us_unshift;
        g_shift   = sc_us_shift;
        break;
    }
}
kbd_layout_t kbd_get_layout(void){ return g_layout; }

const char* kbd_table_unshift(void){ return g_unshift; }
const char* kbd_table_shift(void){   return g_shift;   }

// --------- API pública ---------

void kbd_init(void) {
    uint8_t imr = inb(PIC1_DATA);
    imr &= ~(1u << 1);       // bit 1 = IRQ1
    outb(PIC1_DATA, imr);
}

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
            if (shift) ch = (sc < 128) ? g_shift[sc] : 0;
            else       ch = (sc < 128) ? g_unshift[sc] : 0;

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

int kbd_getchar(void) {
    return kbd_get();
}

size_t kbd_read(char *dst, size_t max) {
    size_t n = 0;
    while (n < max) {
        int c = kbd_get();
        if (c < 0) break;
        dst[n++] = (char)c;
    }
    return n;
}