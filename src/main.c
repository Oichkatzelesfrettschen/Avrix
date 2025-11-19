/* SPDX-License-Identifier: MIT */

/**
 * @file main.c
 * @brief Avrix µ-UNIX Kernel Entry Point
 *
 * Minimal bootable kernel for ATmega328P (PSE51 profile)
 * Target: 2KB RAM, 32KB flash, single-threaded cooperative scheduling
 *
 * Memory Budget (ATmega328P):
 * - Kernel data: 400 bytes
 * - Heap: 1136 bytes
 * - Stack: 512 bytes
 * - Total: 2048 bytes (100% of available SRAM)
 */

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/* Kernel subsystems */
#include "../kernel/sched/scheduler.h"
#include "../kernel/mm/kalloc.h"
#include "../drivers/tty/tty.h"
#include "../drivers/fs/vfs.h"
#include "../drivers/fs/romfs.h"
#include "../drivers/fs/eepfs.h"

/* HAL */
#include "../arch/common/hal.h"

/*═══════════════════════════════════════════════════════════════════════
 * CONFIGURATION - PSE51 MINIMAL PROFILE
 *═══════════════════════════════════════════════════════════════════════*/

#define PSE51_PROFILE           1    /* Minimal embedded POSIX */
#define AVRIX_MAX_TASKS         1    /* Single-threaded (cooperative) */
#define AVRIX_ENABLE_THREADS    0    /* No pthread */
#define AVRIX_ENABLE_IPC        0    /* No Door RPC (save RAM) */
#define AVRIX_ENABLE_SIGNALS    0    /* No signals */
#define AVRIX_HEAP_SIZE         1136 /* Bytes for kalloc */
#define AVRIX_STACK_SIZE        512  /* Bytes for main task */

/* UART configuration (115200 baud @ 16 MHz) */
#define BAUD                    115200
#define UBRR_VALUE              ((F_CPU / (16UL * BAUD)) - 1)

/*═══════════════════════════════════════════════════════════════════════
 * GLOBAL STATE (Kernel Data Segment)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Heap memory pool for kalloc()
 * Static allocation to ensure it's in .bss (not on stack)
 */
static uint8_t heap_pool[AVRIX_HEAP_SIZE];

/**
 * Main task control block
 */
static nk_tcb_t main_task_tcb;

/**
 * Main task stack
 */
static uint8_t main_task_stack[AVRIX_STACK_SIZE];

/**
 * TTY (UART) buffers
 */
#define TTY_BUF_SIZE 64
static uint8_t tty_rx_buffer[TTY_BUF_SIZE];
static uint8_t tty_tx_buffer[TTY_BUF_SIZE];
static tty_t console_tty;

/**
 * Boot status flag
 */
static volatile bool kernel_booted = false;

/*═══════════════════════════════════════════════════════════════════════
 * HAL UART CALLBACKS (for TTY driver)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * @brief UART transmit byte (blocking)
 */
static void uart_putc(uint8_t c)
{
    /* Wait for transmit buffer to be empty */
    while (!(UCSR0A & (1 << UDRE0)));

    /* Send byte */
    UDR0 = c;
}

/**
 * @brief UART receive byte (non-blocking)
 * @return Byte value, or -1 if no data available
 */
static int uart_getc(void)
{
    /* Check if data available */
    if (UCSR0A & (1 << RXC0)) {
        return UDR0;
    }
    return -1;  /* No data */
}

/*═══════════════════════════════════════════════════════════════════════
 * HARDWARE INITIALIZATION
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize UART (115200 baud, 8N1)
 */
static void uart_init(void)
{
    /* Set baud rate */
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR_VALUE;

    /* Enable transmitter and receiver */
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);

    /* Set frame format: 8 data bits, 1 stop bit, no parity */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/**
 * @brief Initialize hardware abstraction layer
 */
static void hal_init(void)
{
    /* Disable interrupts during initialization */
    cli();

    /* Initialize UART for console I/O */
    uart_init();

    /* Initialize TTY driver */
    tty_init(&console_tty,
             tty_rx_buffer, tty_tx_buffer, TTY_BUF_SIZE,
             uart_putc, uart_getc);

    /* Interrupts will be enabled by scheduler_run() */
}

/*═══════════════════════════════════════════════════════════════════════
 * KERNEL INITIALIZATION
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize kernel subsystems
 */
static void kernel_init(void)
{
    /* Initialize memory allocator */
    kalloc_init(heap_pool, AVRIX_HEAP_SIZE);

    /* Initialize virtual filesystem */
    vfs_init();

    /* TODO: Mount ROMFS and EEPFS (when filesystem data available) */
    /* vfs_mount(VFS_TYPE_ROMFS, "/rom"); */
    /* vfs_mount(VFS_TYPE_EEPFS, "/eeprom"); */

    /* Initialize scheduler */
    scheduler_init();

    /* Kernel is now ready */
    kernel_booted = true;
}

/*═══════════════════════════════════════════════════════════════════════
 * BOOT BANNER
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * @brief Print boot banner to console
 */
static void print_boot_banner(void)
{
    const char banner[] PROGMEM =
        "\r\n"
        "╔═══════════════════════════════════════════════════════════════╗\r\n"
        "║ Avrix µ-UNIX for AVR                                          ║\r\n"
        "║ Version 0.1.0 (PSE51 Minimal Profile)                         ║\r\n"
        "╠═══════════════════════════════════════════════════════════════╣\r\n"
        "║ Target  : ATmega328P @ 16 MHz                                 ║\r\n"
        "║ Profile : PSE51 (Minimal Embedded POSIX)                      ║\r\n"
        "║ RAM     : 2 KB (400 B kernel, 1136 B heap, 512 B stack)       ║\r\n"
        "║ Flash   : 32 KB                                               ║\r\n"
        "╚═══════════════════════════════════════════════════════════════╝\r\n"
        "\r\n";

    /* Print banner from flash (PROGMEM) */
    for (uint16_t i = 0; i < sizeof(banner) - 1; i++) {
        char c = (char)hal_pgm_read_byte(&banner[i]);
        uart_putc((uint8_t)c);
    }
}

/*═══════════════════════════════════════════════════════════════════════
 * MAIN TASK (Idle Loop)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * @brief Main task entry point (PSE51 single-threaded)
 *
 * In PSE51 profile, this is the only task. It runs cooperatively
 * and yields periodically to allow scheduler ticks.
 */
static void main_task(void)
{
    uint32_t counter = 0;

    /* Main idle loop */
    while (1) {
        /* Heartbeat every 1000 iterations */
        if ((counter % 1000) == 0) {
            uart_putc('.');  /* Heartbeat indicator */
        }

        counter++;

        /* Cooperative yield (PSE51: no preemption without this) */
        /* In a real application, this would be replaced with actual work */
        /* For now, just burn cycles to show the kernel is running */

        /* Small delay to prevent flooding UART */
        for (volatile uint16_t i = 0; i < 10000; i++);
    }
}

/*═══════════════════════════════════════════════════════════════════════
 * KERNEL ENTRY POINT
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * @brief main() - Avrix kernel entry point
 *
 * Called after AVR startup code (startup.S) initializes hardware:
 * - Stack pointer set to RAMEND
 * - .bss section zeroed
 * - .data section copied from flash to RAM
 * - Interrupts disabled
 *
 * This function:
 * 1. Initializes HAL (hardware abstraction layer)
 * 2. Initializes kernel subsystems (memory, fs, scheduler)
 * 3. Prints boot banner
 * 4. Creates main task
 * 5. Starts scheduler (never returns)
 *
 * @return Never returns (scheduler_run() is noreturn)
 */
int main(void)
{
    /*───────────────────────────────────────────────────────────────
     * 1. HARDWARE INITIALIZATION
     *───────────────────────────────────────────────────────────────*/
    hal_init();

    /*───────────────────────────────────────────────────────────────
     * 2. KERNEL INITIALIZATION
     *───────────────────────────────────────────────────────────────*/
    kernel_init();

    /*───────────────────────────────────────────────────────────────
     * 3. BOOT BANNER
     *───────────────────────────────────────────────────────────────*/
    print_boot_banner();

    /*───────────────────────────────────────────────────────────────
     * 4. CREATE MAIN TASK (PSE51: single-threaded)
     *───────────────────────────────────────────────────────────────*/
    bool created = nk_task_create(
        &main_task_tcb,         /* TCB */
        main_task,              /* Entry point */
        0,                      /* Priority 0 (highest) */
        main_task_stack,        /* Stack buffer */
        AVRIX_STACK_SIZE        /* Stack size */
    );

    if (!created) {
        /* Task creation failed - print error and halt */
        const char error[] PROGMEM = "\r\n[PANIC] Failed to create main task!\r\n";
        for (uint16_t i = 0; i < sizeof(error) - 1; i++) {
            uart_putc((uint8_t)hal_pgm_read_byte(&error[i]));
        }

        /* Halt with interrupts disabled */
        cli();
        while (1);
    }

    /*───────────────────────────────────────────────────────────────
     * 5. START SCHEDULER (never returns)
     *───────────────────────────────────────────────────────────────*/
    const char starting[] PROGMEM = "[BOOT] Starting scheduler...\r\n";
    for (uint16_t i = 0; i < sizeof(starting) - 1; i++) {
        uart_putc((uint8_t)hal_pgm_read_byte(&starting[i]));
    }

    scheduler_run();  /* Enable interrupts and start scheduling */

    /* Should never reach here */
    while (1);
}

/*═══════════════════════════════════════════════════════════════════════
 * INTERRUPT HANDLERS (Weak symbols, can be overridden)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * @brief Default interrupt handler (unused interrupts)
 */
void __attribute__((weak)) __bad_interrupt(void)
{
    /* Unexpected interrupt - halt */
    cli();
    while (1);
}

/**
 * @brief UART RX Complete interrupt (optional, for interrupt-driven RX)
 */
ISR(USART_RX_vect, ISR_BLOCK)
{
    /* Read byte from UART */
    uint8_t c = UDR0;

    /* Push to TTY RX buffer */
    /* TODO: tty_rx_push(&console_tty, c); */
    (void)c;  /* Suppress unused warning for now */
}

/**
 * @brief UART TX Complete interrupt (optional, for interrupt-driven TX)
 */
ISR(USART_UDRE_vect, ISR_BLOCK)
{
    /* TODO: Pop from TTY TX buffer and send */
    /* For now, disable interrupt (not using interrupt-driven TX yet) */
    UCSR0B &= ~(1 << UDRIE0);
}
