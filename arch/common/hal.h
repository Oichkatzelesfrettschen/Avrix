/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file hal.h
 * @brief Hardware Abstraction Layer (HAL) - Common Interface
 *
 * This header defines the platform-independent HAL interface that must be
 * implemented by each architecture (AVR8, ARM Cortex-M, MSP430, etc.).
 *
 * The HAL provides a uniform API for:
 * - System initialization and control
 * - Interrupt management
 * - Timer/clock services
 * - Context switching (for scheduler)
 * - Memory barriers
 * - Atomic operations
 *
 * Each architecture implements this interface in arch/<arch>/hal_impl.c
 */

#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*═══════════════════════════════════════════════════════════════════
 * 1. ARCHITECTURE DETECTION & INCLUDES
 *═══════════════════════════════════════════════════════════════════*/

/* Detect architecture and include specific HAL implementation */
#if defined(__AVR__)
    #define HAL_ARCH_AVR8 1
    #define HAL_ARCH_NAME "AVR8"
    #define HAL_WORD_SIZE 8
    #include "arch/avr8/include/hal_avr8.h"

#elif defined(__ARM_ARCH) || defined(__arm__) || defined(__aarch64__)
    #define HAL_ARCH_ARM 1
    #define HAL_ARCH_NAME "ARM"
    #if defined(__ARM_ARCH_6M__)
        #define HAL_ARCH_ARMV6M 1
        #define HAL_WORD_SIZE 32
        #include "arch/armcm/cortex-m0/hal_armv6m.h"
    #elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        #define HAL_ARCH_ARMV7M 1
        #define HAL_WORD_SIZE 32
        #include "arch/armcm/cortex-m3/hal_armv7m.h"
    #else
        #error "Unsupported ARM architecture variant"
    #endif

#elif defined(__MSP430__)
    #define HAL_ARCH_MSP430 1
    #define HAL_ARCH_NAME "MSP430"
    #define HAL_WORD_SIZE 16
    #include "arch/msp430/hal_msp430.h"

#elif defined(__x86_64__) || defined(__i386__) || defined(__aarch64__)
    /* Host build (native x86/ARM for testing) */
    #define HAL_ARCH_HOST 1
    #define HAL_ARCH_NAME "Host"
    #define HAL_WORD_SIZE (__SIZEOF_POINTER__ * 8)
    #include "arch/common/hal_host.h"

#else
    #error "Unsupported architecture - please add HAL implementation"
#endif

/*═══════════════════════════════════════════════════════════════════
 * 2. COMMON TYPE DEFINITIONS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief HAL capabilities flags
 *
 * Each architecture reports which optional features it supports.
 */
typedef struct hal_caps {
    bool has_mpu;           /**< Memory Protection Unit */
    bool has_fpu;           /**< Floating Point Unit */
    bool has_hardware_div;  /**< Hardware division */
    bool has_atomic_ops;    /**< Atomic compare-exchange */
    bool has_dma;           /**< Direct Memory Access */
    bool has_cache;         /**< Data/instruction cache */
    uint8_t num_cores;      /**< Number of CPU cores (1=single, 2+=SMP) */
    uint32_t cpu_freq_hz;   /**< CPU frequency in Hz */
} hal_caps_t;

/**
 * @brief System reset reasons
 */
typedef enum {
    HAL_RESET_UNKNOWN = 0,
    HAL_RESET_POWER_ON,
    HAL_RESET_EXTERNAL,
    HAL_RESET_WATCHDOG,
    HAL_RESET_SOFTWARE,
    HAL_RESET_BROWNOUT
} hal_reset_reason_t;

/*═══════════════════════════════════════════════════════════════════
 * 3. CORE SYSTEM CONTROL
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize hardware abstraction layer
 *
 * Must be called once at system startup before any other HAL functions.
 * Performs architecture-specific initialization:
 * - Set up stack pointer (if needed)
 * - Initialize system clocks
 * - Configure memory (if applicable)
 * - Detect CPU features
 */
void hal_init(void);

/**
 * @brief Reset the system
 *
 * Triggers a hardware reset. Does not return.
 */
void hal_reset(void) __attribute__((noreturn));

/**
 * @brief Enter idle/low-power mode
 *
 * CPU enters sleep mode until next interrupt. Returns after interrupt.
 * On AVR: sleep mode. On ARM: WFI instruction.
 */
void hal_idle(void);

/**
 * @brief Get reset reason
 *
 * @return Reason for last reset
 */
hal_reset_reason_t hal_reset_reason(void);

/**
 * @brief Get HAL capabilities
 *
 * @param[out] caps Pointer to capabilities structure to fill
 */
void hal_get_caps(hal_caps_t *caps);

/*═══════════════════════════════════════════════════════════════════
 * 4. INTERRUPT MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Enable global interrupts
 *
 * AVR: sei(), ARM: CPSIE i, MSP430: _enable_interrupts()
 */
void hal_irq_enable(void);

/**
 * @brief Disable global interrupts
 *
 * AVR: cli(), ARM: CPSID i, MSP430: _disable_interrupts()
 */
void hal_irq_disable(void);

/**
 * @brief Check if interrupts are enabled
 *
 * @return true if global interrupts are enabled
 */
bool hal_irq_enabled(void);

/**
 * @brief Save interrupt state and disable
 *
 * @return Previous interrupt enable state (for hal_irq_restore)
 */
uint32_t hal_irq_save(void);

/**
 * @brief Restore interrupt state
 *
 * @param state Value returned from hal_irq_save()
 */
void hal_irq_restore(uint32_t state);

/*═══════════════════════════════════════════════════════════════════
 * 5. TIMER & CLOCK SERVICES
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize system timer for periodic tick
 *
 * Configures a hardware timer to generate periodic interrupts at the
 * specified frequency. Used by the scheduler for preemption.
 *
 * @param freq_hz Desired tick frequency (typically 100-1000 Hz)
 *
 * @note Implementation must set up timer interrupt and call
 *       scheduler tick handler from ISR.
 */
void hal_timer_init(uint32_t freq_hz);

/**
 * @brief Get current timer tick count
 *
 * @return Number of ticks since hal_timer_init() or overflow
 */
uint32_t hal_timer_ticks(void);

/**
 * @brief Busy-wait delay in microseconds
 *
 * @param us Microseconds to delay (blocking)
 *
 * @note Accuracy depends on CPU frequency. May use timer or cycle counting.
 */
void hal_timer_delay_us(uint32_t us);

/**
 * @brief Busy-wait delay in milliseconds
 *
 * @param ms Milliseconds to delay (blocking)
 */
void hal_timer_delay_ms(uint32_t ms);

/**
 * @brief Get CPU frequency in Hz
 *
 * @return CPU clock frequency (e.g., 16000000 for 16 MHz)
 */
uint32_t hal_cpu_freq_hz(void);

/*═══════════════════════════════════════════════════════════════════
 * 6. CONTEXT SWITCHING (for scheduler)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Architecture-specific context structure
 *
 * This is defined per-architecture in the architecture-specific header.
 * It contains saved registers, stack pointer, etc.
 *
 * Example (AVR8):
 *   typedef struct {
 *       uint16_t sp;  // Stack pointer
 *   } hal_context_t;
 *
 * Example (ARM Cortex-M):
 *   typedef struct {
 *       uint32_t *sp;     // Stack pointer
 *       uint32_t control; // CONTROL register
 *   } hal_context_t;
 */
/* hal_context_t is defined in arch-specific header */

/**
 * @brief Initialize a new task context
 *
 * Prepares stack and registers for a new task. When the scheduler
 * switches to this context, execution will start at `entry` function.
 *
 * @param[out] ctx        Context structure to initialize
 * @param entry           Task entry point function
 * @param stack           Pointer to stack buffer
 * @param stack_size      Size of stack buffer in bytes
 *
 * @note Stack must be pre-allocated by caller
 * @note On AVR, stack grows downward from stack+stack_size
 */
void hal_context_init(hal_context_t *ctx, void (*entry)(void), void *stack, size_t stack_size);

/**
 * @brief Switch from one task context to another
 *
 * Saves current CPU state to `from` context and restores CPU state
 * from `to` context. Execution continues in the `to` task.
 *
 * @param from  Context to save current state into (can be NULL for first switch)
 * @param to    Context to restore and switch to
 *
 * @note This function is typically called from scheduler with interrupts disabled
 * @note Implementation must save/restore all caller-saved registers
 */
void hal_context_switch(hal_context_t *from, hal_context_t *to);

/*═══════════════════════════════════════════════════════════════════
 * 7. MEMORY BARRIERS & SYNCHRONIZATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Full memory barrier
 *
 * Ensures all memory accesses before this point complete before any
 * memory accesses after this point.
 *
 * AVR: nop (no reordering on single-core 8-bit)
 * ARM: DMB (Data Memory Barrier)
 */
void hal_memory_barrier(void);

/**
 * @brief Data Memory Barrier (ARM-specific, nop on others)
 *
 * Ensures all memory accesses complete before proceeding.
 */
void hal_dmb(void);

/**
 * @brief Data Synchronization Barrier (ARM-specific, nop on others)
 *
 * Ensures all instructions complete before proceeding.
 */
void hal_dsb(void);

/**
 * @brief Instruction Synchronization Barrier (ARM-specific, nop on others)
 *
 * Flushes pipeline, ensures all previous instructions complete.
 */
void hal_isb(void);

/*═══════════════════════════════════════════════════════════════════
 * 8. ATOMIC OPERATIONS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Atomic exchange (8-bit)
 *
 * Atomically swaps value at `ptr` with `val`.
 *
 * @param ptr  Pointer to value
 * @param val  New value
 * @return Previous value at `ptr`
 */
uint8_t hal_atomic_exchange_u8(volatile uint8_t *ptr, uint8_t val);

/**
 * @brief Atomic exchange (16-bit)
 *
 * @param ptr  Pointer to value
 * @param val  New value
 * @return Previous value at `ptr`
 */
uint16_t hal_atomic_exchange_u16(volatile uint16_t *ptr, uint16_t val);

/**
 * @brief Atomic exchange (32-bit)
 *
 * @param ptr  Pointer to value
 * @param val  New value
 * @return Previous value at `ptr`
 */
uint32_t hal_atomic_exchange_u32(volatile uint32_t *ptr, uint32_t val);

/**
 * @brief Atomic compare-and-exchange (8-bit)
 *
 * Atomically: if (*ptr == *expected) { *ptr = val; return true; }
 *             else { *expected = *ptr; return false; }
 *
 * @param ptr      Pointer to value
 * @param expected Pointer to expected value (updated on failure)
 * @param val      New value to store
 * @return true if exchange succeeded
 */
bool hal_atomic_compare_exchange_u8(volatile uint8_t *ptr, uint8_t *expected, uint8_t val);

/**
 * @brief Atomic compare-and-exchange (16-bit)
 */
bool hal_atomic_compare_exchange_u16(volatile uint16_t *ptr, uint16_t *expected, uint16_t val);

/**
 * @brief Atomic compare-and-exchange (32-bit)
 */
bool hal_atomic_compare_exchange_u32(volatile uint32_t *ptr, uint32_t *expected, uint32_t val);

/**
 * @brief Atomic test-and-set (8-bit)
 *
 * Sets bit 0 and returns previous value (0=was clear, 1=was set).
 * Equivalent to: old = *ptr; *ptr = 1; return old;
 *
 * @param ptr Pointer to value
 * @return Previous value (0 or non-zero)
 */
uint8_t hal_atomic_test_and_set_u8(volatile uint8_t *ptr);

/*═══════════════════════════════════════════════════════════════════
 * 9. PLATFORM-SPECIFIC FUNCTIONS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Get architecture name string
 *
 * @return String like "AVR8", "ARMv7-M", "MSP430"
 */
const char *hal_arch_name(void);

/**
 * @brief Get CPU model string
 *
 * @return String like "ATmega128", "STM32F407", "MSP430F5529"
 */
const char *hal_cpu_model(void);

/**
 * @brief Platform-specific early init (optional)
 *
 * Called before hal_init() for critical early setup (e.g., clock config).
 * Can be stubbed if not needed.
 */
void hal_early_init(void) __attribute__((weak));

/*═══════════════════════════════════════════════════════════════════
 * 10. OPTIONAL: MPU/MMU SUPPORT (high-end only)
 *═══════════════════════════════════════════════════════════════════*/

#if defined(HAL_HAS_MPU) && HAL_HAS_MPU

/**
 * @brief MPU region configuration
 */
typedef struct {
    uint32_t base_addr;     /**< Base address (must be aligned) */
    uint32_t size;          /**< Region size in bytes */
    uint8_t  permissions;   /**< Read/write/execute flags */
    bool     enable;        /**< Enable this region */
} hal_mpu_region_t;

/**
 * @brief Initialize MPU
 */
void hal_mpu_init(void);

/**
 * @brief Configure MPU region
 *
 * @param region_num Region number (0-7 on Cortex-M)
 * @param config     Region configuration
 */
void hal_mpu_configure_region(uint8_t region_num, const hal_mpu_region_t *config);

/**
 * @brief Enable MPU
 */
void hal_mpu_enable(void);

/**
 * @brief Disable MPU
 */
void hal_mpu_disable(void);

#endif /* HAL_HAS_MPU */

#ifdef __cplusplus
}
#endif

#endif /* HAL_COMMON_H */
