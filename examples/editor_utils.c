#include <ctype.h>
#include <stdio.h>
#include <string.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#include "../compat/avr/pgmspace.h"
#endif

void pgm_print(const char *p) {
#ifdef __AVR__
    for (char c = pgm_read_byte(p); c; c = pgm_read_byte(++p))
        putchar(c);
#else
    fputs(p, stdout);
#endif
}

void highlight(const char *line) {
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
