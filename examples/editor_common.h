#ifndef EDITOR_COMMON_H
#define EDITOR_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Helper routines shared between the minimal editors (ned/vini).
 * The host build uses stubbed AVR headers, while the cross build
 * relies on the real toolchain.  All functions are portable C.
 */

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

void pgm_print(const char *p);
void highlight(const char *line);
void insert_line(char lines[][MAX_LINE_LEN], uint8_t *count,
                 uint8_t idx, const char *text);
void delete_line(char lines[][MAX_LINE_LEN], uint8_t *count,
                 uint8_t idx);
int  display_width(const char *s, size_t byte_offset);

void set_status_message(const char *fmt, ...);
extern char status_msg[64];

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_COMMON_H */
