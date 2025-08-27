#include <kernel/stdin.h>
#include <drivers/keyboard.h>   // input_init(), kbd_getchar(), kbd_read()

static const stdin_ops_t* g_in = 0;

static int null_getchar(void){ return -1; }
static size_t null_read(char* d, size_t m){ (void)d; (void)m; return 0; }
static void null_shutdown(void){}

static const stdin_ops_t NULL_STDIN = {
    .init = 0,
    .getchar = null_getchar,
    .read = null_read,
    .shutdown = null_shutdown,
};

void stdin_init(const stdin_ops_t* ops){
    g_in = ops ? ops : &NULL_STDIN;
    if (g_in->init) (void)g_in->init();
}

void stdin_set_backend(const stdin_ops_t* ops){
    if (g_in && g_in->shutdown) g_in->shutdown();
    g_in = ops ? ops : &NULL_STDIN;
    if (g_in->init) (void)g_in->init();
}

int stdin_getchar(void){
    return (g_in && g_in->getchar) ? g_in->getchar() : -1;
}

size_t stdin_read(char* dst, size_t max){
    return (g_in && g_in->read) ? g_in->read(dst, max) : 0;
}


static int ps2_init(void){
    kbd_init();   // ← ahora habilita HW y desenmascara
    return 0;
}

static int ps2_getchar(void){
    int c = kbd_getchar();
    return c;
}

static size_t ps2_read(char* dst, size_t max){
    size_t n = kbd_read(dst, max);
    return n;
}
static void ps2_shutdown(void){ /* opcional: re-mascarar IRQ1 si quieres */ }

const stdin_ops_t STDIN_PS2 = {
    .init     = ps2_init,
    .getchar  = ps2_getchar,
    .read     = ps2_read,
    .shutdown = ps2_shutdown,
};