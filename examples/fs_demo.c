#include "fs.h"
#include <stdio.h>
#include <string.h>

/*
 * Minimal demonstration of the in-memory filesystem. The program creates
 * a file, writes a string to it and then reads the contents back. When run
 * under simavr the greeting will be printed to the simulator console.
 */

int main(void)
{
    fs_init();

    /* Create a regular file named greeting.txt. */
    if (fs_create("greeting.txt", 1) < 0) {
        puts("create failed");
        return 1;
    }

    file_t f;
    if (fs_open("greeting.txt", &f) != 0) {
        puts("open failed");
        return 1;
    }

    const char msg[] = "Hello from Avrix";
    if (fs_write(&f, msg, strlen(msg)) != (int)strlen(msg)) {
        puts("write failed");
        return 1;
    }

    /* Rewind and read the contents back. */
    f.off = 0;
    char buf[32];
    int n = fs_read(&f, buf, sizeof buf - 1);
    buf[n] = '\0';

    printf("%s\n", buf);
    return 0;
}
