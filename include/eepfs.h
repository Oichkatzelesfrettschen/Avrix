/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#ifndef EEPFS_H
#define EEPFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Simple read-only filesystem stored in the AVR EEPROM.
 *  Directory entries mirror romfs and consume four bytes each. All access
 *  uses the standard eeprom_read_* API so the runtime uses virtually no RAM.
 */

typedef struct {
    uint16_t addr; /**< EEPROM offset of the file data */
    uint16_t size; /**< Size in bytes                  */
} eepfs_file_t;

const eepfs_file_t *eepfs_open(const char *path);
int eepfs_read(const eepfs_file_t *f, uint16_t off, void *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* EEPFS_H */
