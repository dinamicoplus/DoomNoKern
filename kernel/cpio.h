#include <stdint.h>
#include <string.h>
#include <stdio.h>

static uint32_t hx8(const char *s);
static uint32_t align4(uint32_t x);
const void* cpio_find(const void *cpio, uint32_t size, const char *path, uint32_t *out_len) ;
