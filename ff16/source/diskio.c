#include "ff.h"
#include "diskio.h"
#include <string.h>   // memcpy
#include <stdint.h>

#define DEV_RAM  0    /* único dispositivo */

// Declarado en otro .c si usas xxd -i o symbols de objcopy:
extern unsigned char fat12_img[];
extern unsigned int  fat12_img_len;

// Parámetros FAT12
#define SECTOR_SIZE 512
#define NUM_SECTORS (fat12_img_len / SECTOR_SIZE)

// ---------------------------------------------

static DSTATUS ram_status = STA_NOINIT;
DWORD get_fattime (void){
	return 0;
}

DSTATUS disk_status (BYTE pdrv) {
    if (pdrv != DEV_RAM) return STA_NOINIT;
    return ram_status;
}

DSTATUS disk_initialize (BYTE pdrv) {
    if (pdrv != DEV_RAM) return STA_NOINIT;
    // nada que inicializar en RAM
    ram_status = 0;
    return ram_status;
}

DRESULT disk_read (BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != DEV_RAM) return RES_PARERR;
    if (sector + count > NUM_SECTORS) return RES_PARERR;

    memcpy(buff, fat12_img + sector*SECTOR_SIZE, count*SECTOR_SIZE);
    return RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write (BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != DEV_RAM) return RES_PARERR;
    if (sector + count > NUM_SECTORS) return RES_PARERR;

    memcpy(fat12_img + sector*SECTOR_SIZE, buff, count*SECTOR_SIZE);
    return RES_OK;
}
#endif

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv != DEV_RAM) return RES_PARERR;

    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD*)buff = SECTOR_SIZE;
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(LBA_t*)buff = NUM_SECTORS;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD*)buff = 1;   // erase block size (dummy = 1 sector)
        return RES_OK;
    }
    return RES_PARERR;
}
