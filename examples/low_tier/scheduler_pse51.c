/* SPDX-License-Identifier: MIT */

/**
 * @file scheduler_pse51.c
 * @brief PSE51 Minimal Profile - Single-Task Scheduler Demo
 *
 * Demonstrates PSE51 task scheduling WITHOUT threading:
 * - Single task execution model
 * - Cooperative (non-preemptive) scheduling
 * - Deterministic timing
 * - No context switching overhead
 *
 * Target: Low-end MCUs (ATmega128, limited RAM)
 * Profile: PSE51 (single-threaded, deterministic)
 *
 * Memory Footprint:
 * - Flash: ~250 bytes (scheduler + demo)
 * - RAM: ~24 bytes (task state + stack frame)
 * - EEPROM: 0 bytes
 *
 * Note: PSE51 explicitly EXCLUDES pthread. This demo shows
 * the single-threaded alternative suitable for resource-constrained systems.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Task state for cooperative scheduling
 */
typedef struct {
    uint32_t next_run_ms;  /**< Next execution time (milliseconds) */
    uint32_t interval_ms;  /**< Execution interval */
    uint32_t exec_count;   /**< Execution counter */
    bool     enabled;      /**< Task enabled flag */
} task_t;

/**
 * @brief Simulated millisecond timer (would be hardware timer in production)
 */
static uint32_t g_system_time_ms = 0;

/**
 * @brief Get current system time in milliseconds
 *
 * In production:
 * - AVR: Timer0/Timer1 overflow interrupt
 * - ARM: SysTick timer
 * - x86: clock_gettime(CLOCK_MONOTONIC)
 */
static uint32_t get_system_time_ms(void) {
    return g_system_time_ms;
}

/**
 * @brief Task 1: LED blinker (simulated)
 */
static void task_led_blink(task_t *task) {
    static bool led_state = false;
    led_state = !led_state;

    printf("[%5lu ms] LED: %s (exec #%lu)\n",
           get_system_time_ms(),
           led_state ? "ON " : "OFF",
           task->exec_count);
}

/**
 * @brief Task 2: Sensor reader (simulated)
 */
static void task_read_sensor(task_t *task) {
    /* Simulate sensor reading */
    uint16_t sensor_value = (uint16_t)(task->exec_count * 17 % 256);

    printf("[%5lu ms] Sensor: %u°C (exec #%lu)\n",
           get_system_time_ms(),
           sensor_value,
           task->exec_count);
}

/**
 * @brief Task 3: Watchdog heartbeat
 */
static void task_watchdog(task_t *task) {
    printf("[%5lu ms] Watchdog: OK (exec #%lu)\n",
           get_system_time_ms(),
           task->exec_count);
}

/**
 * @brief PSE51 cooperative scheduler
 *
 * Features:
 * - Non-preemptive (tasks run to completion)
 * - Deterministic (fixed execution order)
 * - Zero context-switch overhead
 * - Suitable for hard real-time systems
 */
int main(void) {
    printf("=== PSE51 Single-Task Scheduler Demo ===\n");
    printf("Profile: Cooperative, non-preemptive, deterministic\n\n");

    /* Initialize tasks */
    task_t task_led = {
        .next_run_ms = 0,
        .interval_ms = 500,  /* 500ms = 2 Hz blink */
        .exec_count  = 0,
        .enabled     = true
    };

    task_t task_sensor = {
        .next_run_ms = 100,
        .interval_ms = 1000,  /* 1000ms = 1 Hz sample */
        .exec_count  = 0,
        .enabled     = true
    };

    task_t task_wdt = {
        .next_run_ms = 250,
        .interval_ms = 2000,  /* 2000ms = 0.5 Hz heartbeat */
        .exec_count  = 0,
        .enabled     = true
    };

    printf("Tasks initialized:\n");
    printf("  1. LED blink:    %lu ms interval\n", task_led.interval_ms);
    printf("  2. Sensor read:  %lu ms interval\n", task_sensor.interval_ms);
    printf("  3. Watchdog:     %lu ms interval\n\n", task_wdt.interval_ms);

    /* Main scheduling loop (PSE51: single-threaded, cooperative) */
    printf("Starting scheduler...\n\n");

    for (uint8_t sim_ticks = 0; sim_ticks < 50; sim_ticks++) {
        /* Simulate time passage (100ms per tick) */
        g_system_time_ms += 100;

        uint32_t now = get_system_time_ms();

        /* Task 1: LED blink */
        if (task_led.enabled && now >= task_led.next_run_ms) {
            task_led_blink(&task_led);
            task_led.exec_count++;
            task_led.next_run_ms = now + task_led.interval_ms;
        }

        /* Task 2: Sensor read */
        if (task_sensor.enabled && now >= task_sensor.next_run_ms) {
            task_read_sensor(&task_sensor);
            task_sensor.exec_count++;
            task_sensor.next_run_ms = now + task_sensor.interval_ms;
        }

        /* Task 3: Watchdog heartbeat */
        if (task_wdt.enabled && now >= task_wdt.next_run_ms) {
            task_watchdog(&task_wdt);
            task_wdt.exec_count++;
            task_wdt.next_run_ms = now + task_wdt.interval_ms;
        }

        /* PSE51: Cooperative - yield control to idle loop */
        /* In production, this would be:
         *   - __WFI() on ARM (wait for interrupt)
         *   - sleep_cpu() on AVR
         *   - Polling for hardware events
         */
    }

    /* Summary statistics */
    printf("\n=== Scheduler Statistics ===\n");
    printf("Runtime: %lu ms\n", get_system_time_ms());
    printf("Task executions:\n");
    printf("  LED blink:   %lu\n", task_led.exec_count);
    printf("  Sensor read: %lu\n", task_sensor.exec_count);
    printf("  Watchdog:    %lu\n", task_wdt.exec_count);

    printf("\nPSE51 scheduler demo complete.\n");
    return 0;
}

/**
 * PSE51 vs PSE52/PSE54 Scheduling:
 * ═════════════════════════════════
 *
 * **PSE51 (This Demo):**
 * - Single-threaded, cooperative
 * - Tasks run to completion (no preemption)
 * - Deterministic execution order
 * - Zero context-switch overhead
 * - Suitable for: Hard real-time, simple state machines
 * - Memory: Minimal (no separate stacks per task)
 *
 * **PSE52 (Mid-Tier):**
 * - Multi-threaded with pthread
 * - Preemptive scheduling (time-sliced)
 * - Non-deterministic order (unless RT priorities)
 * - Context-switch overhead (~10-50 cycles)
 * - Suitable for: Concurrent I/O, complex workflows
 * - Memory: Stack per thread (128-512 bytes each)
 *
 * **PSE54 (High-Tier):**
 * - Full POSIX threading + signals
 * - Preemptive + real-time scheduling (SCHED_FIFO, SCHED_RR)
 * - Priority inheritance, robust mutexes
 * - Higher overhead (MMU, process isolation)
 * - Suitable for: Linux-like systems, complex applications
 * - Memory: Virtual memory, page tables
 *
 * **Migration Path:**
 * PSE51 → PSE52: Convert cooperative tasks to pthread
 * PSE52 → PSE54: Add signals, process isolation, MMU
 */
