#include <stdio.h>
#include "cpio.h"
#include <ff.h>
#include "fat12_img.h"

FATFS fs;
FIL f;
char buf[64];
extern unsigned char ramdisk_cpio[];
extern unsigned int ramdisk_cpio_len;

const void* ramdisk_data(void) { return ramdisk_cpio; }
unsigned long ramdisk_size(void) { return ramdisk_cpio_len; }

void kernel_main(void) {
    // opcional: desbufferiza
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    puts("Hola desde kernel_main!");
    puts("Si ves este texto, el kernel arranco bien.");
		printf("Funciona esto? %lu", ramdisk_size());

		uint32_t flen;
		const char *file = cpio_find(ramdisk_cpio, ramdisk_cpio_len, "./README.txt", &flen);
		if (file) {
    		printf("Encontrado README.txt (%u bytes)\n", flen);
    		// ya puedes leer `file[0..flen-1]`
				int i=0;
				for (i=0; i< flen; i++) {
					printf("%c",file[i]);
				}
		} else {
    		puts("README.txt no encontrado");
		}

    f_mount(&fs, "", 0);
    if (f_open(&f, "README.TXT", FA_READ) == FR_OK) {
        UINT br;
        f_read(&f, buf, sizeof(buf)-1, &br);
        buf[br] = 0;
        puts(buf);
        f_close(&f);
    } else {
        puts("no pude abrir README.TXT");
    }
    for(;;) __asm__ __volatile__("hlt");
}
