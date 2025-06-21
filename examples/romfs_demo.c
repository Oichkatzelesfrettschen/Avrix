#include "romfs.h"
#include <stdio.h>

/* Simple host demonstration of the flash-resident ROMFS. */
int main(void)
{
    const romfs_file_t *f = romfs_open("/usr/bin/hello");
    if (!f) {
        puts("not found");
        return 1;
    }
    char buf[16];
    int n = romfs_read(f, 0, buf, sizeof buf - 1);
    buf[n] = '\0';
    printf("%s", buf);
    return 0;
}
