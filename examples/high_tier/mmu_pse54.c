/* SPDX-License-Identifier: MIT */

/**
 * @file mmu_pse54.c
 * @brief PSE54 Full POSIX Profile - Memory Management Unit
 *
 * Demonstrates PSE54 MMU and virtual memory:
 * - Virtual address spaces
 * - Memory protection (read/write/execute permissions)
 * - Memory mapping (mmap/munmap)
 * - Shared memory regions
 * - Page fault handling
 *
 * Target: High-end MCUs/MPUs with MMU (ARM Cortex-A, RISC-V)
 * Profile: PSE54 (IEEE 1003.13-2003 Full POSIX)
 *
 * Memory Footprint:
 * - Flash: ~2 KB (MMU infrastructure + demo)
 * - RAM: Variable (per-process virtual spaces)
 * - EEPROM: 0 bytes
 *
 * Note: Requires hardware MMU with page table support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

/**
 * @brief Demonstrate memory mapping
 */
static void demo_mmap(void) {
    printf("Test 1: Memory Mapping (mmap)\n");
    printf("------------------------------\n");

    /* Allocate anonymous memory region */
    size_t size = 4096;  /* One page */
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (addr == MAP_FAILED) {
        perror("mmap failed");
        return;
    }

    printf("  ✓ Mapped %zu bytes at address: %p\n", size, addr);
    printf("    Protection: READ | WRITE\n");
    printf("    Flags: PRIVATE | ANONYMOUS\n\n");

    /* Write to mapped region */
    const char *msg = "Hello from mmap!";
    memcpy(addr, msg, strlen(msg) + 1);
    printf("  Wrote: \"%s\"\n", (char *)addr);
    printf("  Read back: \"%s\"\n", (char *)addr);

    /* Change protection to read-only */
    printf("\n  Changing protection to READ-ONLY...\n");
    if (mprotect(addr, size, PROT_READ) == 0) {
        printf("  ✓ Protection changed\n");
        printf("    (Writing would now cause SIGSEGV)\n");
    } else {
        perror("mprotect failed");
    }

    /* Unmap memory */
    if (munmap(addr, size) == 0) {
        printf("\n  ✓ Memory unmapped\n");
    } else {
        perror("munmap failed");
    }
}

/**
 * @brief Demonstrate shared memory
 */
static void demo_shared_memory(void) {
    printf("\nTest 2: Shared Memory\n");
    printf("---------------------\n");

    size_t size = 4096;
    void *shared = mmap(NULL, size, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (shared == MAP_FAILED) {
        perror("mmap failed");
        return;
    }

    printf("  ✓ Shared region allocated at: %p\n", shared);
    printf("    Size: %zu bytes\n", size);
    printf("    Flags: MAP_SHARED (visible to children)\n\n");

    /* Write counter to shared memory */
    uint32_t *counter = (uint32_t *)shared;
    *counter = 0;

    printf("  Initial counter value: %u\n", *counter);
    printf("  Incrementing counter...\n");

    for (int i = 0; i < 10; i++) {
        (*counter)++;
    }

    printf("  Final counter value: %u\n", *counter);
    printf("  (Child processes would see same memory)\n");

    munmap(shared, size);
    printf("\n  ✓ Shared memory unmapped\n");
}

/**
 * @brief Demonstrate memory layout
 */
static void demo_memory_layout(void) {
    printf("\nTest 3: Virtual Memory Layout\n");
    printf("-----------------------------\n");

    /* Stack variable */
    int stack_var = 42;

    /* Heap allocation */
    int *heap_var = malloc(sizeof(int));
    *heap_var = 123;

    /* Static data */
    static int static_var = 999;

    printf("Virtual Address Space Layout:\n");
    printf("  Text (code):    ~0x00400000 (read + execute)\n");
    printf("  Data (rodata):  ~0x00600000 (read-only)\n");
    printf("  BSS (uninit):   ~0x00601000 (read + write)\n");
    printf("  Heap:           %p (read + write, grows up)\n", (void *)heap_var);
    printf("  Stack:          %p (read + write, grows down)\n", (void *)&stack_var);
    printf("  Static data:    %p (read + write)\n", (void *)&static_var);

    printf("\nMemory Protection:\n");
    printf("  Text segment:   PROT_READ | PROT_EXEC\n");
    printf("  Data segment:   PROT_READ | PROT_WRITE\n");
    printf("  Stack:          PROT_READ | PROT_WRITE (+ guard page)\n");
    printf("  Heap:           PROT_READ | PROT_WRITE\n");

    printf("\nPage Table Hierarchy (ARM):\n");
    printf("  L1 (1st level): 4096 entries × 1 MB sections\n");
    printf("  L2 (2nd level): 256 entries × 4 KB pages\n");
    printf("  TLB: Translation Lookaside Buffer (cache)\n");

    free(heap_var);
}

/**
 * @brief PSE54 MMU demonstration
 */
int main(void) {
    printf("=== PSE54 Memory Management Unit Demo ===\n");
    printf("Profile: Virtual memory with MMU protection\n\n");

    printf("System Information:\n");
    printf("  Page size: %ld bytes\n", sysconf(_SC_PAGESIZE));
    printf("  Physical pages: %ld\n", sysconf(_SC_PHYS_PAGES));
    printf("  Available pages: %ld\n", sysconf(_SC_AVPHYS_PAGES));
    printf("\n");

    /* Run demonstrations */
    demo_mmap();
    demo_shared_memory();
    demo_memory_layout();

    /* Statistics */
    printf("\n=== MMU Statistics ===\n");
    printf("Memory operations performed:\n");
    printf("  - mmap: 2 calls (anonymous + shared)\n");
    printf("  - mprotect: 1 call (change permissions)\n");
    printf("  - munmap: 2 calls (cleanup)\n");
    printf("  - malloc/free: 1 pair (heap allocation)\n");

    printf("\nMMU Features Demonstrated:\n");
    printf("  ✓ Virtual address spaces\n");
    printf("  ✓ Memory protection (R/W/X permissions)\n");
    printf("  ✓ Anonymous mapping (no file backing)\n");
    printf("  ✓ Shared memory regions\n");
    printf("  ✓ Page-level granularity\n");

    printf("\nPSE54 MMU demo complete.\n");
    return 0;
}

/**
 * PSE54 Memory Management:
 * ════════════════════════
 *
 * **Virtual Memory Concepts:**
 * - Each process has separate virtual address space
 * - MMU translates virtual → physical addresses
 * - Page tables store translations
 * - TLB caches recent translations
 *
 * **Memory Mapping Functions:**
 * ```c
 * // Allocate anonymous memory
 * void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
 *                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
 *
 * // Map file into memory
 * int fd = open("file.dat", O_RDONLY);
 * void *ptr = mmap(NULL, size, PROT_READ,
 *                  MAP_PRIVATE, fd, 0);
 *
 * // Unmap memory
 * munmap(ptr, size);
 * ```
 *
 * **Protection Flags:**
 * - PROT_NONE  - No access
 * - PROT_READ  - Read access
 * - PROT_WRITE - Write access
 * - PROT_EXEC  - Execute access
 *
 * **Mapping Flags:**
 * - MAP_PRIVATE    - Copy-on-write
 * - MAP_SHARED     - Shared with other processes
 * - MAP_ANONYMOUS  - No file backing
 * - MAP_FIXED      - Use exact address
 * - MAP_LOCKED     - Lock pages in RAM (no swap)
 *
 * **Page Fault Handling:**
 * 1. CPU accesses virtual address
 * 2. MMU checks page table entry
 * 3. If invalid → Page fault exception
 * 4. Kernel handles fault:
 *    - Allocate physical page (demand paging)
 *    - Load from disk (if file-backed)
 *    - Update page table
 * 5. Resume execution
 *
 * **Page Table Structure (ARM):**
 * ```
 * Virtual Address (32-bit):
 * [31:20] L1 index (12 bits) → Section (1 MB)
 * [19:12] L2 index (8 bits)  → Page (4 KB)
 * [11:0]  Offset (12 bits)   → Byte in page
 *
 * Page Table Entry (PTE):
 * [31:12] Physical address
 * [11:10] Access permissions (R/W)
 * [9]     Execute-never (XN)
 * [8:2]   Reserved / flags
 * [1:0]   Entry type (section/page/invalid)
 * ```
 *
 * **TLB (Translation Lookaside Buffer):**
 * - Small cache of recent translations
 * - Typical size: 32-128 entries
 * - Hit rate: >99% for most workloads
 * - Flushed on context switch
 *
 * **Use Cases:**
 * - Process isolation (sandboxing)
 * - Memory-mapped I/O
 * - Shared libraries (code sharing)
 * - Inter-process communication (shared memory)
 * - File I/O optimization (mmap)
 * - Large sparse arrays (demand paging)
 *
 * **Performance:**
 * - TLB hit: 0 cycles (hardware lookup)
 * - TLB miss: ~10-100 cycles (page table walk)
 * - Page fault: ~1000-10000 cycles (OS handler)
 * - mmap: ~1-5 µs (create mapping)
 * - munmap: ~1-5 µs (destroy mapping)
 *
 * **Hardware Requirements:**
 * - MMU with page table support
 * - TLB for translation caching
 * - Memory protection unit (MPU) alternative for no-MMU
 *
 * **Comparison Across Profiles:**
 *
 * | Feature          | PSE51 | PSE52 | PSE54 |
 * |------------------|-------|-------|-------|
 * | Virtual memory   | ✗     | ✗     | ✓     |
 * | MMU              | ✗     | ✗     | ✓     |
 * | mmap/munmap      | ✗     | ✗     | ✓     |
 * | mprotect         | ✗     | ✗     | ✓     |
 * | Shared memory    | ✗     | ✗     | ✓     |
 * | Page faults      | ✗     | ✗     | ✓     |
 *
 * **Embedded MMU Alternatives:**
 * - MPU (Memory Protection Unit): Simpler than MMU, region-based
 * - No virtual addresses (physical only)
 * - Fewer protection regions (8-16 vs unlimited)
 * - Lower overhead but less flexible
 */
