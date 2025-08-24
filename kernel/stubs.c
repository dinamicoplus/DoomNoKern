// stubs.c — syscalls mínimos para newlib en kernel bare-metal (i386/x86_64)
// - Consola VGA en modo texto (0xB8000)
// - _write/_read/_close/_lseek/_fstat/_isatty/_sbrk/_exit/_kill/_getpid
// - Wrappers: write/read/... llaman a _write/_read/...

#include <stdint.h>
#include <stddef.h>     // size_t, ptrdiff_t
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

    if (c == '\r') {           // retorno de carro: ir al inicio de la línea
        cursor_col = 0;
        return;
    }
    if (c == '\n') {           // salto de línea
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

static void vga_write(const char *p, size_t n) {
    for (size_t i = 0; i < n; i++) vga_putchar(p[i]);
}

// ------------------ errno ------------------
// newlib reentrante usa __errno_r, pero disponer de 'errno' global es práctico para stubs.
int errno;

// ------------------ helpers ------------------
static inline int is_console_fd(int fd) {
    // stdout/stderr (1,2) y también descriptores inválidos negativos
    // (newlib puede usar -1 antes de inicializar stdout).
    return (fd < 0) || (fd == 1) || (fd == 2);
}

// ------------------ write/read/close/lseek ------------------
ssize_t _write(int fd, const void *buf, size_t count) {
    const char *p = (const char *)buf;
    if (is_console_fd(fd)) {
        vga_write(p, count);
        return (ssize_t)count;
    }
    errno = EBADF;
    return -1;
}

ssize_t _read(int fd, void *buf, size_t count) {
    (void)buf; (void)count;
    if (is_console_fd(fd)) {
        // sin entrada disponible -> EOF inmediato
        return 0;
    }
    errno = EBADF;
    return -1;
}

int _close(int fd) {
    if (is_console_fd(fd)) return 0;
    errno = EBADF;
    return -1;
}

off_t _lseek(int fd, off_t offset, int whence) {
    (void)offset; (void)whence;
    if (is_console_fd(fd)) return 0;
    errno = EBADF;
    return -1;
}

// ------------------ fstat/isatty ------------------
int _fstat(int fd, struct stat *st) {
    if (!st) { errno = EFAULT; return -1; }
    if (is_console_fd(fd)) {
        st->st_mode  = S_IFCHR;
        st->st_nlink = 1;
        st->st_size  = 0;
        return 0;
    }
    errno = EBADF;
    return -1;
}

int _isatty(int fd) {
    return is_console_fd(fd) ? 1 : 0;
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

// ------------------ Wrappers sin guion bajo ------------------
// (más portables que __attribute__((alias)))

ssize_t write(int fd, const void *buf, size_t count)    { return _write(fd, buf, count); }
ssize_t read(int fd, void *buf, size_t count)           { return _read(fd, buf, count); }
int     close(int fd)                                    { return _close(fd); }
off_t   lseek(int fd, off_t offset, int whence)          { return _lseek(fd, offset, whence); }
int     isatty(int fd)                                   { return _isatty(fd); }
int     fstat(int fd, struct stat *st)                   { return _fstat(fd, st); }
void*   sbrk(ptrdiff_t incr)                             { return _sbrk(incr); }
int     kill(int pid, int sig)                           { return _kill(pid, sig); }
int     getpid(void)                                     { return _getpid(); }
