#include <kernel/console.h>
#include <kernel/stdin.h>
#include <stdio.h>

static const console_ops_t* g_ops = NULL;

static void null_clear(void){ (void)0; }
static void null_putc(char c){ (void)c; }
static void null_write(const char* s, size_t n){ (void)s; (void)n; }
static console_ops_t g_null = { null_clear, null_putc, null_write };

void console_init(const console_ops_t* ops){ g_ops = ops ? ops : &g_null; }
void console_set_backend(const console_ops_t* ops){ g_ops = ops ? ops : &g_null; }

void console_clear(void){ (g_ops && g_ops->clear ? g_ops->clear : g_null.clear)(); }
void console_putc(char c){ (g_ops && g_ops->putc ? g_ops->putc : g_null.putc)(c); }
void console_write(const char* s, size_t n){
    if (g_ops && g_ops->write) g_ops->write(s, n);
    else for (size_t i=0;i<n;i++) console_putc(s[i]);
}

static void console_setup_stdio(console_stdio_mode_t m){
    switch (m){
    case CONSOLE_STDIO_UNBUFFERED:
        setvbuf(stdin,  NULL, _IOLBF, 0);   // <-- añade esto
        setvbuf(stdout, NULL, _IONBF, 0); 
        setvbuf(stderr, NULL, _IONBF, 0);
        break;
    case CONSOLE_STDIO_LINE_BUFFERED:
        setvbuf(stdin,  NULL, _IOLBF, 0);   // stdin suele ser no bufferizado en TTY
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
        break;
    default: break;
    }
}

void console_init_all(const console_ops_t* out_ops,
                      const stdin_ops_t*    in_ops,
                      console_stdio_mode_t  stdio_mode)
{
    console_init(out_ops);
    stdin_init(in_ops);
    console_setup_stdio(stdio_mode);
}