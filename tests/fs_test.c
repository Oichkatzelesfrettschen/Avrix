#include "fs.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    fs_init();

    /* invalid name checks */
    assert(fs_create("", 1) == -EINVAL);
    assert(fs_create("aaaaaaaaaaaaaa", 1) == -EINVAL);

    /*
     * Reserve block zero so that a real file never maps to block number
     * zero. This simplifies the checks below.
     */
    int dummy = fs_create("dummy", 1);
    assert(dummy >= 0);
    assert(fs_create("dummy", 1) == -EEXIST);
    file_t d;
    assert(fs_open("dummy", &d) == 0);
    char c = 'x';
    assert(fs_write(&d, &c, 1) == 1);

    int inum = fs_create("test", 1);
    assert(inum >= 0);

    file_t f;
    assert(fs_open("test", &f) == 0);

    /* -- basic write/read check ------------------------------------------ */
    const char msg[] = "hello world";
    assert(fs_write(&f, msg, sizeof msg) == (int)sizeof msg);

    f.off = 0;
    char buf[sizeof msg];
    int r = fs_read(&f, buf, sizeof msg);
    assert(r == (int)sizeof msg);
    assert(memcmp(buf, msg, sizeof msg) == 0);
    puts("fs basic I/O passed");

    /* -- fill remaining blocks ------------------------------------------ */
    char block[FS_BLOCK_SIZE] = {0};
    char name[8];
    int idx = 0;
    int blocks = 0;

    while (1) {
        snprintf(name, sizeof name, "b%02d", idx++);
        int fi = fs_create(name, 1);
        assert(fi >= 0);
        file_t bf;
        assert(fs_open(name, &bf) == 0);
        for (int j = 0; j < 4; ++j) {
            int w = fs_write(&bf, block, sizeof block);
            if (w < (int)sizeof block) {
                /* disk space exhausted */
                assert(w == 0);
                goto blocks_exhausted;
            }
            ++blocks;
        }
    }

blocks_exhausted:
    assert(blocks == FS_NUM_BLOCKS - 2);
    puts("fs block exhaustion handled");

    /* -- consume remaining inodes -------------------------------------- */
    int used = 1 + 2 + idx; /* root + dummy/test + b-files */
    while (used < FS_NUM_INODES) {
        snprintf(name, sizeof name, "i%02d", used);
        assert(fs_create(name, 1) >= 0);
        ++used;
    }
    assert(fs_create("extra", 1) == -1);
    puts("fs inode exhaustion handled");

    return 0;
}
