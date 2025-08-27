// stubs.c — newlib syscalls + FatFs over RAM image, usando capas console/stdin
// - stdout/stderr → console_write() (backend texto o vga13)
// - stdin         → stdin_read()   (backend teclado PS/2 u otro)
// - fd >= 3       → ficheros FatFs

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include <kernel/console.h>   // console_write()
#include <kernel/stdin.h>     // stdin_read()

typedef enum { TTY_RAW=0, TTY_COOKED=1 } tty_mode_t;
static tty_mode_t g_tty_mode = TTY_COOKED;
static int g_tty_echo = 1;

void stdin_set_mode(int cooked){ g_tty_mode = cooked ? TTY_COOKED : TTY_RAW; }
void stdin_set_echo(int on){ g_tty_echo = on ? 1 : 0; }

// ---- helpers internos para eco “seguro” ----
static inline void echo_char(char c){
    if (!g_tty_echo) return;
    if (c == '\b') console_write("\b \b", 3);
    else console_putc(c);
}
static inline void echo_str(const char* s){ if (g_tty_echo) console_write(s, strlen(s)); }

// ---------- FatFs ----------
#include <fatfs/ff.h>         // FatFs API
static FATFS g_fs;
static int g_mounted = 0;

static int ensure_mounted(void) {
    if (g_mounted) return 0;
    FRESULT fr = f_mount(&g_fs, "", 0);
    if (fr == FR_OK) { g_mounted = 1; return 0; }
    errno = EIO;
    return -1;
}

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

// ---------- errno ----------
int errno;

// ---------- fd table para FatFs (fd >= 3) ----------
typedef struct {
    uint8_t used;
    FIL f;
    int append;
} fd_entry;

#define FD_BASE 3
#define FD_MAX  16
static fd_entry g_fd[FD_MAX];

static inline int is_stdin_fd(int fd){  return fd == 0; }
static inline int is_stdout_fd(int fd){ return fd == 1 || fd == 2; }

static int fd_alloc(void){
    for (int i=0;i<FD_MAX;i++) if (!g_fd[i].used) { g_fd[i].used=1; g_fd[i].append=0; return FD_BASE+i; }
    errno = EMFILE; return -1;
}
static fd_entry* fd_get(int fd){
    int idx = fd - FD_BASE;
    if (idx < 0 || idx >= FD_MAX) return NULL;
    return g_fd[idx].used ? &g_fd[idx] : NULL;
}
static void fd_free(int fd){
    int idx = fd - FD_BASE;
    if (idx >= 0 && idx < FD_MAX) memset(&g_fd[idx], 0, sizeof(g_fd[idx]));
}

// ---------- syscalls ----------
ssize_t _write(int fd, const void *buf, size_t count) {
    if (is_stdout_fd(fd)) {
        console_write((const char*)buf, count);
        return (ssize_t)count;
    }
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
    if (fd == 0) {
        if (g_tty_mode == TTY_RAW) {
            // RAW: devolver lo disponible (no bloqueante + bloqueamos con hlt)
            size_t n = 0;
            while (n == 0) {
                n = stdin_read((char*)buf, count);
                if (n == 0) __asm__ __volatile__("hlt");
            }
            if (g_tty_echo) echo_str((const char*)buf);  // eco directo si quieres
            return (ssize_t)n;
        }

        // COOKED: editar línea, tope de backspace = 0, devuelve al encontrar '\n'
        static char line[256];
        static size_t have = 0;   // bytes preparados y aún no entregados
        static size_t pos  = 0;   // índice de lectura de 'line'

        // Entrega remanente de una línea previa
        if (have > 0) {
            size_t n = have < count ? have : count;
            memcpy(buf, line + pos, n);
            pos  += n;
            have -= n;
            return (ssize_t)n;
        }

        // Construye nueva línea (bloquea hasta '\n')
        size_t len = 0; // tope de edición (no se permite len<0)
        for (;;) {
            int k = stdin_getchar();
            if (k < 0) { __asm__ __volatile__("hlt"); continue; }
            char c = (char)k;

            // Normaliza CR → LF
            if (c == '\r') c = '\n';

            // Backspace/Delete: no borra antes del inicio (len==0)
            if (c == '\b' || (unsigned char)c == 0x7F) {
                if (len > 0) { len--; echo_char('\b'); }
                continue;
            }

            // Fin de línea
            if (c == '\n') {
                echo_char('\n');
                // Prepara buffer a entregar (incluye '\n' como hace un TTY)
                if (len < sizeof(line)-1) { line[len++] = '\n'; }
                pos = 0; have = len;
                // Entrega ahora hasta 'count' (resto queda para la próxima _read)
                size_t n = have < count ? have : count;
                memcpy(buf, line + pos, n);
                pos  += n;
                have -= n;
                return (ssize_t)n;
            }

            // Carácter imprimible normal
            if (len + 1 < sizeof(line)) {  // reserva sitio para el '\n' posterior
                line[len++] = c;
                echo_char(c);
            } else {
                // opcional: campana al llegar al límite
                // console_putc('\a');
            }
        }
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
    if (is_stdin_fd(fd) || is_stdout_fd(fd)) return 0;
    fd_entry *e = fd_get(fd);
    if (!e) { errno = EBADF; return -1; }
    FRESULT fr = f_close(&e->f);
    fd_free(fd);
    if (fr != FR_OK) { errno = ff_to_errno(fr); return -1; }
    return 0;
}

off_t _lseek(int fd, off_t offset, int whence) {
    if (is_stdin_fd(fd) || is_stdout_fd(fd)) return 0;
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
    if (is_stdin_fd(fd) || is_stdout_fd(fd)) { st->st_mode = S_IFCHR; st->st_nlink=1; st->st_size=0; return 0; }
    fd_entry *e = fd_get(fd);
    if (!e) { errno = EBADF; return -1; }
    st->st_mode = S_IFREG;
    st->st_nlink = 1;
    st->st_size  = (off_t)f_size(&e->f);
    return 0;
}

int _isatty(int fd) { return (is_stdin_fd(fd) || is_stdout_fd(fd)) ? 1 : 0; }

// ---------- POSIX extra ----------
int _open(const char *path, int oflag, int mode) {
    (void)mode;
    if (ensure_mounted() < 0) return -1;

    BYTE acc = 0;
    int append = 0;

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

// ---------- sbrk / process ----------
extern char _end; static char *heap_end;
void *_sbrk(ptrdiff_t incr){ if (!heap_end) heap_end=&_end; char*prev=heap_end; heap_end+=incr; return prev; }

void _exit(int status){ (void)status; for(;;){ __asm__ __volatile__("hlt"); } }
int  _kill(int pid,int sig){ (void)pid;(void)sig; errno=EINVAL; return -1; }
int  _getpid(void){ return 1; }

// ---------- non-underscore wrappers ----------
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
        va_list ap; va_start(ap, flags);
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);
    }
    return _open(path, flags, mode);
}
int unlink(const char *p)                    { return _unlink(p); }
int rename(const char *o, const char *n)     { return _rename(o, n); }
int mkdir (const char *p, mode_t m)          { return _mkdir(p, m); }