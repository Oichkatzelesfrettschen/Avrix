#include "romfs.h"
#include "eepfs.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void)
{
    const romfs_file_t *f = romfs_open("/etc/config/version.txt");
    assert(f);
    char buf[8] = {0};
    int n = romfs_read(f, 0, buf, sizeof buf - 1);
    assert(n > 0);
    printf("romfs:%s\n", buf);

    const eepfs_file_t *ef = eepfs_open("/sys/message.txt");
    assert(ef);
    char b2[12] = {0};
    n = eepfs_read(ef, 0, b2, sizeof b2 - 1);
    assert(n > 0);
    printf("eepfs:%s\n", b2);
    puts("romfs/eepfs ok");
    return 0;
}
