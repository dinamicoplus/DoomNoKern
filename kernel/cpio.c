#include <stdint.h>
#include <string.h>
#include <stdio.h>

static uint32_t hx8(const char *s) { // 8 hex chars -> uint32
    uint32_t v=0;
    for (int i=0;i<8;i++) {
        char c=s[i];
        uint32_t d = (c>='0'&&c<='9')?c-'0':(c>='A'&&c<='F')?c-'A'+10:c-'a'+10;
        v = (v<<4) | d;
    }
    return v;
}
static uint32_t align4(uint32_t x){ return (x + 3) & ~3u; }

const void* cpio_find(const void *cpio, uint32_t size, const char *path, uint32_t *out_len) {
    const uint8_t *p = (const uint8_t*)cpio;
    const uint8_t *end = p + size;
    while (p + 110 <= end) {                    // header “newc” = 110 bytes ASCII
        if (memcmp(p, "070701", 6)!=0) return NULL;
        uint32_t namesize = hx8((const char*)p+94);   // c_namesize
        uint32_t filesize = hx8((const char*)p+54);   // c_filesize
        const uint8_t *name = p + 110;
        if (name + namesize > end) return NULL;
        const char *nstr = (const char*)name;
        // Nombre "TRAILER!!!" => fin
        if (strcmp(nstr, "TRAILER!!!")==0) return NULL;

        const uint8_t *data = (const uint8_t*)align4((uint32_t)(uintptr_t)(name + namesize));
        if (data + filesize > end) return NULL;

        if (strcmp(nstr, path)==0) {
            if (out_len) *out_len = filesize;
            return data;
        }
        p = (const uint8_t*)align4((uint32_t)(uintptr_t)(data + filesize));
    }
    return NULL;
}
