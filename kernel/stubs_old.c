// stubs.c — syscalls mínimos para newlib en kernel bare-metal (x86/x86_64)
// - Consola VGA en modo texto (0xB8000)
// - _write/_read/_close/_lseek/_fstat/_isatty/_sbrk/_exit/_kill/_getpid
// - Aliases: write/read/... -> _write/_read/...

#include <stdint.h>
#include <stddef.h>     // ptrdiff_t
#include <sys/types.h>  // ssize_t, off_t
#include <sys/stat.h>   // struct stat, S_IFCHR, S_IFREG
#include <errno.h>

// ------------------ VGA texto ------------------
#define VGA_TEXT   ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static inline uint16_t vga_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}

static int cursor_row = 0;
static int cursor_col = 0;

static void vga_putchar(char c) {
    const uint8_t attr = 0x07; // gris sobre negro
    if (c == '\n') {
        cursor_col = 0;
        if (++cursor_row >= VGA_HEIGHT) cursor_row = 0; // wrap simple (sin scroll)
        return;
    }
    VGA_TEXT[(size_t)cursor_row * VGA_WIDTH + (size_t)cursor_col] = vga_entry(c, attr);
    if (++cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        if (++cursor_row >= VGA_HEIGHT) cursor_row = 0;
    }
}

// ------------------ errno ------------------
int errno;

// ------------------ write/read/close/lseek ------------------
ssize_t _write(int fd, const void *buf, size_t count) {
vga_putchar('*');
    // stdout(1)/stderr(2) -> VGA
    if (1) {
        const char *p = (const char *)buf;
        for (size_t i = 0; i < count; i++) {
            vga_putchar(p[i]);
        }
        return (ssize_t)count;
    }
    // Cualquier otro fd: "fake" (descarta y finge éxito)
    return (ssize_t)count;
}

ssize_t _read(int fd, void *buf, size_t count) {
    (void)fd; (void)buf; (void)count;
    // Sin entrada disponible: EOF inmediato
    return 0;
}

int _close(int fd) {
    (void)fd;
    return 0;
}

off_t _lseek(int fd, off_t offset, int whence) {
    (void)fd; (void)offset; (void)whence;
    // Sin FS real; devolver 0 como posición "válida"
    return 0;
}

// ------------------ fstat/isatty ------------------
int _fstat(int fd, struct stat *st) {
    if (!st) { errno = EFAULT; return -1; }
    if (fd == 1 || fd == 2) {
        st->st_mode = S_IFCHR;
        return 0;
    }
    st->st_mode = S_IFREG;
    st->st_nlink = 1;
    st->st_size = 0;
    return 0;
}

int _isatty(int fd) {
    return (fd == 1 || fd == 2) ? 1 : 0;
}

// ------------------ sbrk (heap) ------------------
// Define este símbolo en tu script de link (fin de .bss)
extern char _end; // proviene del linker (fin de .bss = inicio del heap)
static char *heap_end;

void *_sbrk(ptrdiff_t incr) {
    if (heap_end == 0) heap_end = &_end;
    char *prev = heap_end;
    // TODO: si quieres, comprueba límite del heap con un símbolo __heap_limit
    heap_end += incr;
    return (void *)prev;
}

// ------------------ exit/kill/getpid ------------------
void _exit(int status) {
    (void)status;
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

int _kill(int pid, int sig) {
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void) {
    return 1;
}

// ------------------ Aliases sin guion bajo ------------------
// Exporta write/read/close/lseek/isatty/fstat/sbrk/kill/getpid como alias
// a sus versiones _write/_read/...  (útil para _*_r de newlib).

ssize_t _write(int fd, const void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count)
    __attribute__((alias("_write")));

ssize_t _read(int fd, void *buf, size_t count);
ssize_t read(int fd, void *buf, size_t count)
    __attribute__((alias("_read")));

int _close(int fd);
int close(int fd) __attribute__((alias("_close")));

off_t _lseek(int fd, off_t offset, int whence);
off_t lseek(int fd, off_t offset, int whence)
    __attribute__((alias("_lseek")));

int _isatty(int fd);
int isatty(int fd) __attribute__((alias("_isatty")));

int _fstat(int fd, struct stat *st);
int fstat(int fd, struct stat *st)
    __attribute__((alias("_fstat")));

void *_sbrk(ptrdiff_t incr);
void *sbrk(ptrdiff_t incr) __attribute__((alias("_sbrk")));

int _kill(int pid, int sig);
int kill(int pid, int sig) __attribute__((alias("_kill")));

int _getpid(void);
int getpid(void) __attribute__((alias("_getpid")));
