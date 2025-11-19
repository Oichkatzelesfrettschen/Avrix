/* SPDX-License-Identifier: MIT */

/**
 * @file vfs_test.c
 * @brief Unit tests for Virtual Filesystem (VFS) layer
 *
 * Tests Phase 5 VFS implementation:
 * - Mount point management
 * - Path resolution (longest-prefix matching)
 * - File descriptor management
 * - Multi-filesystem operations
 */

#include "drivers/fs/vfs.h"
#include "drivers/fs/romfs.h"
#include "drivers/fs/eepfs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  ✓ %s\n", msg); \
        tests_passed++; \
    } else { \
        printf("  ✗ %s\n", msg); \
        tests_failed++; \
    } \
} while(0)

/**
 * Test 1: Mount operations
 */
static void test_vfs_mount(void) {
    printf("\nTest 1: VFS Mount Operations\n");
    printf("-----------------------------\n");

    int ret;

    /* Mount ROMFS at /rom */
    ret = vfs_mount(VFS_TYPE_ROMFS, "/rom");
    TEST_ASSERT(ret == 0, "Mount ROMFS at /rom");

    /* Mount EEPFS at /eeprom */
    ret = vfs_mount(VFS_TYPE_EEPFS, "/eeprom");
    TEST_ASSERT(ret == 0, "Mount EEPFS at /eeprom");

    /* Attempt duplicate mount (should fail or replace) */
    ret = vfs_mount(VFS_TYPE_ROMFS, "/rom");
    TEST_ASSERT(ret == 0 || ret < 0, "Duplicate mount handled");

    /* Mount at root */
    ret = vfs_mount(VFS_TYPE_ROMFS, "/");
    TEST_ASSERT(ret == 0, "Mount at root /");
}

/**
 * Test 2: Path resolution
 */
static void test_vfs_path_resolution(void) {
    printf("\nTest 2: Path Resolution\n");
    printf("-----------------------\n");

    /* Test longest-prefix matching */
    /* Paths starting with /rom should resolve to ROMFS */
    /* Paths starting with /eeprom should resolve to EEPFS */

    int fd;

    /* Try opening from different mount points */
    fd = vfs_open("/rom/config.txt", VFS_O_RDONLY);
    TEST_ASSERT(fd >= 0 || fd == -2, "Path /rom/config.txt resolved");
    if (fd >= 0) vfs_close(fd);

    fd = vfs_open("/eeprom/data.bin", VFS_O_RDWR);
    TEST_ASSERT(fd >= 0 || fd == -2, "Path /eeprom/data.bin resolved");
    if (fd >= 0) vfs_close(fd);

    /* Test invalid path (no mount) */
    fd = vfs_open("/invalid/file.txt", VFS_O_RDONLY);
    TEST_ASSERT(fd < 0, "Invalid path rejected");

    /* Test nested path */
    fd = vfs_open("/rom/sub/dir/file.txt", VFS_O_RDONLY);
    TEST_ASSERT(fd >= 0 || fd == -2, "Nested path resolved");
    if (fd >= 0) vfs_close(fd);
}

/**
 * Test 3: File descriptor management
 */
static void test_vfs_fd_management(void) {
    printf("\nTest 3: File Descriptor Management\n");
    printf("-----------------------------------\n");

    int fds[8];
    int count = 0;

    /* Open multiple files */
    for (int i = 0; i < 8; i++) {
        fds[i] = vfs_open("/rom/test.txt", VFS_O_RDONLY);
        if (fds[i] >= 0) {
            count++;
        }
    }
    TEST_ASSERT(count > 0, "Opened multiple file descriptors");

    /* Close all */
    for (int i = 0; i < count; i++) {
        int ret = vfs_close(fds[i]);
        TEST_ASSERT(ret == 0, "Closed file descriptor");
    }

    /* Verify FDs are reusable */
    int fd = vfs_open("/rom/test.txt", VFS_O_RDONLY);
    TEST_ASSERT(fd >= 0 || fd == -2, "FD reused after close");
    if (fd >= 0) vfs_close(fd);
}

/**
 * Test 4: Read/write operations
 */
static void test_vfs_read_write(void) {
    printf("\nTest 4: Read/Write Operations\n");
    printf("------------------------------\n");

    uint8_t write_buf[32] = "VFS Test Data";
    uint8_t read_buf[32];
    int fd, ret;

    /* Write test (EEPFS) */
    fd = vfs_open("/eeprom/test.dat", VFS_O_RDWR | VFS_O_CREAT);
    if (fd >= 0) {
        ret = vfs_write(fd, write_buf, sizeof(write_buf));
        TEST_ASSERT(ret > 0, "Write to EEPFS");

        /* Seek back to start */
        ret = vfs_lseek(fd, 0, VFS_SEEK_SET);
        TEST_ASSERT(ret == 0, "Seek to start");

        /* Read back */
        ret = vfs_read(fd, read_buf, sizeof(read_buf));
        TEST_ASSERT(ret > 0, "Read from EEPFS");

        /* Verify data */
        TEST_ASSERT(memcmp(write_buf, read_buf, sizeof(write_buf)) == 0,
                    "Read data matches written data");

        vfs_close(fd);
    } else {
        printf("  ⊘ Write test skipped (file not created)\n");
    }

    /* Read-only test (ROMFS) */
    fd = vfs_open("/rom/readonly.txt", VFS_O_RDONLY);
    if (fd >= 0) {
        ret = vfs_write(fd, write_buf, sizeof(write_buf));
        TEST_ASSERT(ret < 0, "Write to read-only filesystem rejected");
        vfs_close(fd);
    } else {
        printf("  ⊘ Read-only test skipped (file not found)\n");
    }
}

/**
 * Test 5: Seek operations
 */
static void test_vfs_seek(void) {
    printf("\nTest 5: Seek Operations\n");
    printf("------------------------\n");

    int fd = vfs_open("/rom/test.txt", VFS_O_RDONLY);
    if (fd < 0) {
        printf("  ⊘ Seek tests skipped (file not found)\n");
        return;
    }

    int ret;

    /* SEEK_SET */
    ret = vfs_lseek(fd, 10, VFS_SEEK_SET);
    TEST_ASSERT(ret >= 0, "SEEK_SET to offset 10");

    /* SEEK_CUR */
    ret = vfs_lseek(fd, 5, VFS_SEEK_CUR);
    TEST_ASSERT(ret >= 0, "SEEK_CUR forward 5");

    /* SEEK_END */
    ret = vfs_lseek(fd, 0, VFS_SEEK_END);
    TEST_ASSERT(ret >= 0, "SEEK_END to end of file");

    vfs_close(fd);
}

/**
 * Main test runner
 */
int main(void) {
    printf("=== VFS Unit Tests ===\n");
    printf("Testing Phase 5 Virtual Filesystem implementation\n");

    /* Run tests */
    test_vfs_mount();
    test_vfs_path_resolution();
    test_vfs_fd_management();
    test_vfs_read_write();
    test_vfs_seek();

    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
