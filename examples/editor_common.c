#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "editor_common.h"

char status_msg[64];

void set_status_message(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(status_msg, sizeof status_msg, fmt, ap);
    va_end(ap);
}
