#include "fs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void)
{
    fs_init();

    /* create a file and write to it */
    assert(fs_create("a.txt", 1) >= 0);
    file_t f;
    assert(fs_open("a.txt", &f) == 0);
    const char msg[] = "simavr";
    assert(fs_write(&f, msg, strlen(msg)) == (int)strlen(msg));

    /* read it back */
    f.off = 0;
    char buf[8] = {0};
    assert(fs_read(&f, buf, sizeof buf - 1) == (int)strlen(msg));
    printf("read:%s\n", buf);

    /* list files */
    char list[32] = {0};
    int n = fs_list(list, sizeof list);
    printf("list:%d:%s\n", n, list);

    /* remove and confirm */
    assert(fs_unlink("a.txt") == 0);
    assert(fs_open("a.txt", &f) != 0);

    puts("fs simavr ok");
    return 0;
}
