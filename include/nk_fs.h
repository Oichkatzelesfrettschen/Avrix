#ifndef NK_FS_H
#define NK_FS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file nk_fs.h
 * @brief TinyLog-4 EEPROM filesystem API.
 */

/** Initialise TinyLog-4 by scanning EEPROM for the last valid record. */
void nk_fs_init(void);

/** Append a key/value pair. Returns false on error or full store. */
bool nk_fs_put(uint16_t key, uint16_t val);

/** Retrieve the most recent value for a key. */
bool nk_fs_get(uint16_t key, uint16_t *val);

/** Mark a key as deleted. */
bool nk_fs_del(uint16_t key);

/** Optional garbage collection routine. */
void nk_fs_gc(void);

#ifdef __cplusplus
}
#endif

#endif /* NK_FS_H */
