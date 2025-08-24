// stubs.c — newlib syscalls + FatFs over RAM image
// - VGA console on 0xB8000 for fd 1/2
// - POSIX-like file syscalls via FatFs (libfatfs.a)
// - Minimal fd table (fd >= 3 are regular files)

#include <stdint.h>
#include <stddef.h>     // size_t, ptrdiff_t
#include <sys/types.h>  // ssize_t, off_t
#include <sys/stat.h>   // struct stat, S_IFCHR, S_IFREG
#include <errno.h>
#include <fcntl.h>      // O_* flags
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

// ---------- FatFs ----------
#include "ff.h"         // FatFs API

#include "stubs.h"
#include "input.h"
// Lazy mount (single volume "")
static FATFS g_fs;
static int g_mounted = 0;

static int ensure_mounted(void) {
    if (g_mounted) return 0;
    FRESULT fr = f_mount(&g_fs, "", 0);
    if (fr == FR_OK) { g_mounted = 1; return 0; }
    errno = EIO;
    return -1;
}

// Map FRESULT -> errno (very small table)
static int ff_to_errno(FRESULT fr) {
    switch (fr) {
    case FR_OK: return 0;
    case FR_NO_FILE: case FR_NO_PATH: return ENOENT;
    case FR_INVALID_NAME: return EINVAL;
    case FR_EXIST: return EEXIST;
    case FR_DENIED: return EACCES;
    case FR_NOT_READY: return EBUSY;
    case FR_INVALID_OBJECT: return EBADF;
    case FR_DISK_ERR: case FR_INT_ERR: return EIO;
    case FR_WRITE_PROTECTED: return EROFS;
    case FR_NOT_ENABLED: case FR_NO_FILESYSTEM: return ENODEV;
    case FR_TOO_MANY_OPEN_FILES: return EMFILE;
    default: return EIO;
    }
}

// ---------- VGA text console ----------
#define VGA_TEXT   ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static inline uint8_t inb(uint16_t p){ uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }

void vga_enable_cursor(uint8_t start, uint8_t end){
    outb(0x3D4, 0x0A);                 // Cursor Start
    uint8_t val = (inb(0x3D5) & 0xC0) | (start & 0x1F);
    outb(0x3D5, val);
    outb(0x3D4, 0x0B);                 // Cursor End
    outb(0x3D5, end & 0x1F);
}

void vga_update_cursor(int row, int col){        // 80x25
    uint16_t pos = (uint16_t)(row * 80 + col);
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));   // low
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(pos >> 8));     // high
}

static inline uint16_t vga_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}
void clear_scrn(void) {
    uint16_t blank = vga_entry(' ', 0x07);  // espacio gris sobre negro
    for (int row = 0; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            VGA_TEXT[row * VGA_WIDTH + col] = blank;
        }
    }
		vga_update_cursor(0,0);
}

static int cursor_row = 0, cursor_col = 0;

static void vga_putchar(char c) {
    const uint8_t attr = 0x07;
    if (c == '\r') { cursor_col = 0; vga_update_cursor(cursor_row, cursor_col); return;  }
    if (c == '\n') { cursor_col = 0; if (++cursor_row >= VGA_HEIGHT) cursor_row = 0; vga_update_cursor(cursor_row, cursor_col); return; }
    VGA_TEXT[(size_t)cursor_row * VGA_WIDTH + (size_t)cursor_col] = vga_entry(c, attr);
    if (++cursor_col >= VGA_WIDTH) { cursor_col = 0; if (++cursor_row >= VGA_HEIGHT) cursor_row = 0; }
		vga_update_cursor(cursor_row, cursor_col);
}
static void vga_write(const char *p, size_t n) { for (size_t i=0;i<n;i++) vga_putchar(p[i]); }

// ---------- errno ----------
int errno;

// ---------- fd table for files (fd >= 3) ----------
typedef struct {
    uint8_t used;
    FIL f;           // FatFs file object
    int append;      // whether opened with O_APPEND
} fd_entry;

#define FD_BASE   3
#define FD_MAX    16
static fd_entry g_fd[FD_MAX];

static inline int is_console_fd(int fd){ return (fd < 0) || (fd == 1) || (fd == 2); }

static int fd_alloc(void) {
    for (int i=0;i<FD_MAX;i++) if (!g_fd[i].used) { g_fd[i].used=1; g_fd[i].append=0; return FD_BASE+i; }
    errno = EMFILE; return -1;
}
static fd_entry* fd_get(int fd) {
    int idx = fd - FD_BASE;
    if (idx < 0 || idx >= FD_MAX) return NULL;
    return g_fd[idx].used ? &g_fd[idx] : NULL;
}
static void fd_free(int fd) {
    int idx = fd - FD_BASE;
    if (idx >= 0 && idx < FD_MAX) memset(&g_fd[idx], 0, sizeof(g_fd[idx]));
}

// ---------- syscalls ----------
ssize_t _write(int fd, const void *buf, size_t count) {
    const char *p = (const char*)buf;
    if (is_console_fd(fd)) { vga_write(p, count); return (ssize_t)count; }

    if (ensure_mounted() < 0) return -1;
    fd_entry *e = fd_get(fd);
    if (!e) { errno = EBADF; return -1; }

#if FF_FS_READONLY
    errno = EROFS; return -1;
#else
    if (e->append) f_lseek(&e->f, f_size(&e->f));
    UINT bw = 0;
    FRESULT fr = f_write(&e->f, buf, (UINT)count, &bw);
    if (fr != FR_OK) { errno = ff_to_errno(fr); return -1; }
    return (ssize_t)bw;
#endif
}

ssize_t _read(int fd, void *buf, size_t count) {
    //if (is_console_fd(fd)) return 0; // no stdin
		if (fd == 0) {
		    size_t got = kbd_read((char*)buf, count);
		    return (ssize_t)got;  // puede ser 0 si no hay teclas
		}

    if (ensure_mounted() < 0) return -1;
    fd_entry *e = fd_get(fd);
    if (!e) { errno = EBADF; return -1; }

    UINT br = 0;
    FRESULT fr = f_read(&e->f, buf, (UINT)count, &br);
    if (fr != FR_OK) { errno = ff_to_errno(fr); return -1; }
    return (ssize_t)br;
}

int _close(int fd) {
    if (is_console_fd(fd)) return 0;
    fd_entry *e = fd_get(fd);
    if (!e) { errno = EBADF; return -1; }
    FRESULT fr = f_close(&e->f);
    fd_free(fd);
    if (fr != FR_OK) { errno = ff_to_errno(fr); return -1; }
    return 0;
}

off_t _lseek(int fd, off_t offset, int whence) {
    if (is_console_fd(fd)) return 0;
    if (ensure_mounted() < 0) return -1;
    fd_entry *e = fd_get(fd);
    if (!e) { errno = EBADF; return -1; }

    FSIZE_t base = 0;
    switch (whence) {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = e->f.fptr; break;
        case SEEK_END: base = f_size(&e->f); break;
        default: errno = EINVAL; return -1;
    }
    if (offset < 0 && (FSIZE_t)(-offset) > base) { errno = EINVAL; return -1; }
    FSIZE_t pos = base + (FSIZE_t)offset;

    FRESULT fr = f_lseek(&e->f, pos);
    if (fr != FR_OK) { errno = ff_to_errno(fr); return -1; }
    return (off_t)e->f.fptr;
}

int _fstat(int fd, struct stat *st) {
    if (!st) { errno = EFAULT; return -1; }
    if (is_console_fd(fd)) { st->st_mode = S_IFCHR; st->st_nlink=1; st->st_size=0; return 0; }

    fd_entry *e = fd_get(fd);
    if (!e) { errno = EBADF; return -1; }
    st->st_mode = S_IFREG;
    st->st_nlink = 1;
    st->st_size  = (off_t)f_size(&e->f);
    return 0;
}

int _isatty(int fd) { return is_console_fd(fd) ? 1 : 0; }

// ---------- extra POSIX-y syscalls for newlib ----------
int _open(const char *path, int oflag, int mode) {
    (void)mode;
    if (ensure_mounted() < 0) return -1;

    BYTE acc = 0;
    int append = 0;

    // Translate O_* to FatFs flags
    switch (oflag & O_ACCMODE) {
        case O_RDONLY: acc |= FA_READ; break;
        case O_WRONLY: acc |= FA_WRITE; break;
        case O_RDWR:   acc |= (FA_READ | FA_WRITE); break;
        default: errno = EINVAL; return -1;
    }
    if (oflag & O_CREAT) {
#if FF_FS_READONLY
        errno = EROFS; return -1;
#else
        if (oflag & O_EXCL)       acc |= FA_CREATE_NEW;
        else if (oflag & O_TRUNC) acc |= FA_CREATE_ALWAYS;
        else                      acc |= FA_OPEN_ALWAYS;
#endif
    } else {
        acc |= FA_OPEN_EXISTING;
    }
    if (oflag & O_APPEND) append = 1;

    int fd = fd_alloc();
    if (fd < 0) return -1;
    fd_entry *e = fd_get(fd);

    FRESULT fr = f_open(&e->f, path, acc);
    if (fr != FR_OK) { fd_free(fd); errno = ff_to_errno(fr); return -1; }

    e->append = append;
    if (append) f_lseek(&e->f, f_size(&e->f));
    return fd;
}

int _unlink(const char *path) {
#if FF_FS_READONLY
    errno = EROFS; return -1;
#else
    if (ensure_mounted() < 0) return -1;
    FRESULT fr = f_unlink(path);
    if (fr != FR_OK) { errno = ff_to_errno(fr); return -1; }
    return 0;
#endif
}

int _rename(const char *oldp, const char *newp) {
#if FF_FS_READONLY
    errno = EROFS; return -1;
#else
    if (ensure_mounted() < 0) return -1;
    FRESULT fr = f_rename(oldp, newp);
    if (fr != FR_OK) { errno = ff_to_errno(fr); return -1; }
    return 0;
#endif
}

int _mkdir(const char *path, mode_t mode) {
    (void)mode;
#if FF_FS_READONLY
    errno = EROFS; return -1;
#else
    if (ensure_mounted() < 0) return -1;
    FRESULT fr = f_mkdir(path);
    if (fr != FR_OK) { errno = ff_to_errno(fr); return -1; }
    return 0;
#endif
}

// ---------- sbrk / process stubs ----------
extern char _end; static char *heap_end;
void *_sbrk(ptrdiff_t incr){ if (!heap_end) heap_end=&_end; char*prev=heap_end; heap_end+=incr; return prev; }

void _exit(int status){ (void)status; for(;;){ __asm__ __volatile__("hlt"); } }
int  _kill(int pid,int sig){ (void)pid;(void)sig; errno=EINVAL; return -1; }
int  _getpid(void){ return 1; }

// ---------- non-underscore wrappers (portable) ----------
ssize_t write(int fd, const void *buf, size_t n) { return _write(fd, buf, n); }
ssize_t read (int fd, void *buf, size_t n)       { return _read(fd, buf, n); }
int     close(int fd)                             { return _close(fd); }
off_t   lseek(int fd, off_t o, int w)            { return _lseek(fd, o, w); }
int     isatty(int fd)                            { return _isatty(fd); }
int     fstat(int fd, struct stat *st)           { return _fstat(fd, st); }
void*   sbrk(ptrdiff_t incr)                     { return _sbrk(incr); }
int     kill(int pid, int sig)                   { return _kill(pid, sig); }
int     getpid(void)                             { return _getpid(); }
int open(const char *path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        /* El tercer argumento se pasa como int según default promotions */
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);
    }
    return _open(path, flags, mode);
}
int     unlink(const char *p)                    { return _unlink(p); }
int     rename(const char *o, const char *n)     { return _rename(o, n); }
int     mkdir(const char *p, mode_t m)           { return _mkdir(p, m); }
