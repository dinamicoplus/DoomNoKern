/**
 * @file cpio.h
 * @brief Interfaz para acceder a archivos en formato CPIO
 */
#ifndef FS_CPIO_H
#define FS_CPIO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convierte 8 caracteres hexadecimales a un uint32_t
 * @param s Cadena con 8 caracteres hexadecimales
 * @return Valor convertido
 */
static uint32_t hx8(const char *s);

/**
 * @brief Alinea un valor a 4 bytes
 * @param x Valor a alinear
 * @return Valor alineado
 */
static uint32_t align4(uint32_t x);

/**
 * @brief Busca un archivo en un archivo CPIO en memoria
 * @param cpio Puntero a la imagen CPIO en memoria
 * @param size Tamaño de la imagen CPIO
 * @param path Ruta del archivo a buscar
 * @param out_len Puntero donde almacenar el tamaño del archivo encontrado
 * @return Puntero al contenido del archivo o NULL si no se encuentra
 */
const void* cpio_find(const void *cpio, uint32_t size, const char *path, uint32_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* FS_CPIO_H */