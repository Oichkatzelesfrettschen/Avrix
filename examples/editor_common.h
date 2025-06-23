#ifndef EDITOR_COMMON_H
#define EDITOR_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Helper routines shared between the minimal editors (ned/vini).
 * The host build uses stubbed AVR headers, while the cross build
 * relies on the real toolchain.  All functions are portable C.
 */

static inline void pgm_print(const char *p)
{
#ifdef __AVR__
    for (char c = pgm_read_byte(p); c; c = pgm_read_byte(++p))
        putchar(c);
#else
    fputs(p, stdout);
#endif
}

static inline void highlight(const char *line)
{
    if (strncmp(line, "//", 2) == 0 || line[0] == '#') {
        printf("\x1b[33m%s\x1b[0m", line);
        return;
    }

    for (const char *p = line; *p; ++p) {
        if (isdigit((unsigned char)*p))
            printf("\x1b[36m%c\x1b[0m", *p);
        else
            putchar(*p);
    }
}

static inline void insert_line(char lines[][MAX_LINE_LEN], uint8_t *count,
                               uint8_t idx, const char *text)
{
    if (*count >= MAX_LINES)
        return;
    if (idx > *count)
        idx = *count;
    for (uint8_t i = *count; i > idx; --i)
        memcpy(lines[i], lines[i - 1], MAX_LINE_LEN);
    strncpy(lines[idx], text, MAX_LINE_LEN - 1);
    lines[idx][MAX_LINE_LEN - 1] = '\0';
    ++*count;
}

static inline void delete_line(char lines[][MAX_LINE_LEN], uint8_t *count,
                               uint8_t idx)
{
    if (idx >= *count)
        return;
    for (uint8_t i = idx; i + 1 < *count; ++i)
        memcpy(lines[i], lines[i + 1], MAX_LINE_LEN);
    --*count;
}

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_COMMON_H */
