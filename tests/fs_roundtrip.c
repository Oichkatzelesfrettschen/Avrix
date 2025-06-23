#include "fs.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void)
{
    fs_init();

    /* reserve block zero */
    /* Reserve block zero for metadata sanity */
    (void)fs_create("dummy", 1);
    file_t d;
    assert(fs_open("dummy", &d) == 0);
    char c = 'x';
    assert(fs_write(&d, &c, 1) == 1);

    /* create and write to a file */
    int inum = fs_create("demo", 1);
    assert(inum >= 0);

    file_t fh;
    assert(fs_open("demo", &fh) == 0);

    const char msg[] = "sample";
    assert(fs_write(&fh, msg, sizeof msg) == (int)sizeof msg);

    fh.off = 0;
    char buf[sizeof msg];
    int n = fs_read(&fh, buf, sizeof buf);
    printf("read %d\n", n);
    assert(n == (int)sizeof buf);
    assert(memcmp(buf, msg, sizeof msg) == 0);

    char list[64];
    int cnt = fs_list(list, sizeof list);
    printf("list before unlink (%d): %s\n", cnt, list);
    assert(strstr(list, "demo") != NULL);

    assert(fs_unlink("demo") == 0);
    assert(fs_open("demo", &fh) == -1);

    char list2[64];
    int cnt2 = fs_list(list2, sizeof list2);
    printf("list after unlink (%d): %s\n", cnt2, list2);
    assert(strstr(list2, "demo") == NULL);

    puts("fs roundtrip passed");
    return 0;
}
