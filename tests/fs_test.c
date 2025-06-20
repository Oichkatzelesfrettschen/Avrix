#include "fs.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    fs_init();
    /* allocate first block to avoid zero as a valid block number */
    int dummy = fs_create("dummy", 1);
    file_t d;
    fs_open("dummy", &d);
    char c = 'x';
    fs_write(&d, &c, 1);

    int inum = fs_create("test", 1);
    assert(inum >= 0);

    file_t f;
    assert(fs_open("test", &f) == 0);

    const char msg[] = "hello world";
    assert(fs_write(&f, msg, sizeof(msg)) == sizeof(msg));

    f.off = 0;
    char buf[32];
    int r = fs_read(&f, buf, sizeof(msg));
    assert(r == sizeof(msg));
    assert(memcmp(buf, msg, sizeof(msg)) == 0);

    puts("fs basic test passed");
    return 0;
}
