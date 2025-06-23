/*─────────────────────────────────────────────────────────────────────────────
 *  editor_common.c — unified runtime/editor helpers  (resolved 2025-06-22)
 *───────────────────────────────────────────────────────────────────────────*/
#include "editor_common.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#ifdef __AVR__
#  include <avr/pgmspace.h>
#else
#  include "../compat/avr/pgmspace.h"
#  include <wchar.h>
#  include <locale.h>
#endif

/*────────────────────────────  Status-line helper  ─────────────────────────*/

char status_msg[64];

void set_status_message(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(status_msg, sizeof status_msg, fmt, ap);
    va_end(ap);
}

/*──────────────────────────  PROGMEM string printer  ───────────────────────*/

void pgm_print(const char *p)
{
#ifdef __AVR__
    for (char c = pgm_read_byte(p); c; c = pgm_read_byte(++p))
        putchar(c);
#else
    fputs(p, stdout);
#endif
}

/*────────────────────────────  Syntax highlighter  ─────────────────────────*/

void highlight(const char *line)
{
    if (strncmp(line, "//", 2) == 0 || line[0] == '#') {
        printf("\x1b[33m%s\x1b[0m", line);           /* comments/directives */
        return;
    }
    for (const char *p = line; *p; ++p) {
        if (isdigit((unsigned char)*p))
            printf("\x1b[36m%c\x1b[0m", *p);         /* numbers             */
        else
            putchar(*p);
    }
}

/*───────────────────────────  Line-buffer helpers  ─────────────────────────*/

void insert_line(char lines[][MAX_LINE_LEN], uint8_t *count,
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

void delete_line(char lines[][MAX_LINE_LEN], uint8_t *count,
                 uint8_t idx)
{
    if (idx >= *count)
        return;

    for (uint8_t i = idx; i + 1 < *count; ++i)
        memcpy(lines[i], lines[i + 1], MAX_LINE_LEN);

    --*count;
}

/*─────────────────────  Display-column width calculator  ───────────────────*/

int display_width(const char *s, size_t byte_offset)
{
#ifdef __AVR__
    int width = 0;
    for (size_t i = 0; i < byte_offset && s[i]; ++i) {
        if (s[i] == '\t')
            width += 8 - (width % 8);
        else
            ++width;
    }
    return width;
#else
    int width  = 0;
    size_t i   = 0;
    while (i < byte_offset && s[i]) {
        if (s[i] == '\t') {
            width += 8 - (width % 8);
            ++i;
        } else {
            wchar_t wc;
            int len = mbtowc(&wc, s + i, MB_CUR_MAX);
            if (len <= 0) {
                ++width;
                ++i;
            } else {
                int w = wcwidth(wc);
                width += (w > 0) ? w : 1;
                i += len;
            }
        }
    }
    return width;
#endif
}
