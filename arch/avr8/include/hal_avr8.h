/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file hal_avr8.h
 * @brief AVR8 Architecture-Specific HAL Definitions
 *
 * This header provides AVR8-specific implementations and types for the
 * common HAL interface defined in arch/common/hal.h
 *
 * Supported MCUs:
 * - ATmega128, ATmega128A
 * - ATmega1280, ATmega1281
 * - ATmega1284, ATmega1284P
 * - ATmega328P, ATmega328PB
 * - ATmega32, ATmega32A
 */

#ifndef HAL_AVR8_H
#define HAL_AVR8_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* AVR-specific headers */
#if defined(__AVR__)
#  include <avr/io.h>
#  include <avr/interrupt.h>
#  include <avr/sleep.h>
#  include <avr/wdt.h>
#  include <avr/boot.h>
#else
#  /* Host build - use compatibility headers */
#  include "compat/avr/io.h"
#  include "compat/avr/interrupt.h"
#endif

/*═══════════════════════════════════════════════════════════════════
 * AVR8-SPECIFIC CONFIGURATION
 *═══════════════════════════════════════════════════════════════════*/

/* AVR8 does not have these features */
#define HAL_HAS_MPU         0
#define HAL_HAS_FPU         0
#define HAL_HAS_CACHE       0
#define HAL_HAS_DMA         0
#define HAL_HAS_HARDWARE_DIV 0

/* AVR8 has limited atomic support (8-bit only) */
#define HAL_HAS_ATOMIC_U8   1
#define HAL_HAS_ATOMIC_U16  0  /* Requires interrupt disable */
#define HAL_HAS_ATOMIC_U32  0  /* Requires interrupt disable */

/*═══════════════════════════════════════════════════════════════════
 * CONTEXT STRUCTURE
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief AVR8 task context
 *
 * AVR8 context switch requires saving/restoring:
 * - Stack pointer (SPH:SPL) - 16 bits
 * - SREG (status register)
 * - 32 general-purpose registers (r0-r31)
 *
 * For space efficiency, we only store the stack pointer here.
 * Registers are saved/restored on the stack by the context switch routine.
 */
typedef struct {
    uint16_t sp;  /**< Stack pointer (little-endian: SPL:SPH) */
} hal_context_t;

/* Ensure context size is known at compile time */
_Static_assert(sizeof(hal_context_t) == 2, "AVR8 context must be 2 bytes");

/*═══════════════════════════════════════════════════════════════════
 * MCU DETECTION & CONFIGURATION
 *═══════════════════════════════════════════════════════════════════*/

/* Detect specific MCU */
#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega128A__)
#  define HAL_MCU_NAME "ATmega128"
#  define HAL_FLASH_SIZE 131072UL  /* 128 KB */
#  define HAL_SRAM_SIZE  4096UL    /* 4 KB */
#  define HAL_EEPROM_SIZE 4096UL   /* 4 KB */
#  define HAL_NUM_TIMERS 4
#  define HAL_NUM_UARTS  2

#elif defined(__AVR_ATmega1280__)
#  define HAL_MCU_NAME "ATmega1280"
#  define HAL_FLASH_SIZE 131072UL  /* 128 KB */
#  define HAL_SRAM_SIZE  8192UL    /* 8 KB */
#  define HAL_EEPROM_SIZE 4096UL   /* 4 KB */
#  define HAL_NUM_TIMERS 6
#  define HAL_NUM_UARTS  4

#elif defined(__AVR_ATmega1281__)
#  define HAL_MCU_NAME "ATmega1281"
#  define HAL_FLASH_SIZE 131072UL  /* 128 KB */
#  define HAL_SRAM_SIZE  8192UL    /* 8 KB */
#  define HAL_EEPROM_SIZE 4096UL   /* 4 KB */
#  define HAL_NUM_TIMERS 6
#  define HAL_NUM_UARTS  2

#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
#  define HAL_MCU_NAME "ATmega1284P"
#  define HAL_FLASH_SIZE 131072UL  /* 128 KB */
#  define HAL_SRAM_SIZE  16384UL   /* 16 KB */
#  define HAL_EEPROM_SIZE 4096UL   /* 4 KB */
#  define HAL_NUM_TIMERS 4
#  define HAL_NUM_UARTS  2

#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__)
#  define HAL_MCU_NAME "ATmega328P"
#  define HAL_FLASH_SIZE 32768UL   /* 32 KB */
#  define HAL_SRAM_SIZE  2048UL    /* 2 KB */
#  define HAL_EEPROM_SIZE 1024UL   /* 1 KB */
#  define HAL_NUM_TIMERS 3
#  define HAL_NUM_UARTS  1

#elif defined(__AVR_ATmega32__) || defined(__AVR_ATmega32A__)
#  define HAL_MCU_NAME "ATmega32"
#  define HAL_FLASH_SIZE 32768UL   /* 32 KB */
#  define HAL_SRAM_SIZE  2048UL    /* 2 KB */
#  define HAL_EEPROM_SIZE 1024UL   /* 1 KB */
#  define HAL_NUM_TIMERS 3
#  define HAL_NUM_UARTS  1

#elif defined(__AVR_ATmega16U2__)
#  define HAL_MCU_NAME "ATmega16U2"
#  define HAL_FLASH_SIZE 16384UL   /* 16 KB */
#  define HAL_SRAM_SIZE  512UL     /* 512 B */
#  define HAL_EEPROM_SIZE 512UL    /* 512 B */
#  define HAL_NUM_TIMERS 2
#  define HAL_NUM_UARTS  1

#else
#  warning "Unknown AVR MCU - using generic defaults"
#  define HAL_MCU_NAME "AVR (generic)"
#  define HAL_FLASH_SIZE 32768UL
#  define HAL_SRAM_SIZE  2048UL
#  define HAL_EEPROM_SIZE 1024UL
#  define HAL_NUM_TIMERS 3
#  define HAL_NUM_UARTS  1
#endif

/* CPU frequency - must be defined externally via -DF_CPU=... */
#ifndef F_CPU
#  warning "F_CPU not defined, assuming 16 MHz"
#  define F_CPU 16000000UL
#endif

#define HAL_CPU_FREQ_HZ F_CPU

/*═══════════════════════════════════════════════════════════════════
 * TIMER CONFIGURATION
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Select which timer to use for system tick
 *
 * Timer0: 8-bit, typically used for system tick
 * Timer1: 16-bit, can be used for higher precision
 * Timer2: 8-bit, available on some MCUs
 */
#ifndef HAL_SYSTICK_TIMER
#  define HAL_SYSTICK_TIMER 0  /* Use Timer0 by default */
#endif

/* Timer0 configuration for 1 kHz tick (as in original µ-UNIX) */
#if HAL_SYSTICK_TIMER == 0
#  define HAL_TIMER_PRESCALE  64
#  define HAL_TIMER_HZ        1000
#  define HAL_TIMER_RELOAD    ((F_CPU / HAL_TIMER_PRESCALE / HAL_TIMER_HZ) - 1)
   _Static_assert(HAL_TIMER_RELOAD <= 255, "Timer0 reload exceeds 8-bit range");
#endif

/*═══════════════════════════════════════════════════════════════════
 * INTERRUPT VECTOR TABLE
 *═══════════════════════════════════════════════════════════════════*/

/* Define common interrupt vectors (names vary by MCU) */
#if defined(TIMER0_COMPA_vect)
#  define HAL_TIMER0_COMPA_ISR TIMER0_COMPA_vect
#elif defined(TIM0_COMPA_vect)
#  define HAL_TIMER0_COMPA_ISR TIM0_COMPA_vect
#else
#  warning "Unknown Timer0 compare A vector"
#endif

/*═══════════════════════════════════════════════════════════════════
 * INLINE HAL FUNCTIONS (PERFORMANCE-CRITICAL)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Enable global interrupts (inline for speed)
 */
static inline void hal_irq_enable(void) {
#if defined(__AVR__)
    sei();
#endif
}

/**
 * @brief Disable global interrupts (inline for speed)
 */
static inline void hal_irq_disable(void) {
#if defined(__AVR__)
    cli();
#endif
}

/**
 * @brief Check if interrupts are enabled
 */
static inline bool hal_irq_enabled(void) {
#if defined(__AVR__)
    return (SREG & _BV(SREG_I)) != 0;
#else
    return false;  /* Host build */
#endif
}

/**
 * @brief Save interrupt state and disable
 */
static inline uint32_t hal_irq_save(void) {
#if defined(__AVR__)
    uint8_t sreg = SREG;
    cli();
    return sreg;
#else
    return 0;
#endif
}

/**
 * @brief Restore interrupt state
 */
static inline void hal_irq_restore(uint32_t state) {
#if defined(__AVR__)
    SREG = (uint8_t)state;
#else
    (void)state;
#endif
}

/**
 * @brief Memory barrier (nop on AVR8 - no reordering)
 */
static inline void hal_memory_barrier(void) {
    /* AVR8 is in-order, single-core - no barrier needed */
    __asm__ volatile ("" ::: "memory");
}

static inline void hal_dmb(void) {
    hal_memory_barrier();
}

static inline void hal_dsb(void) {
    hal_memory_barrier();
}

static inline void hal_isb(void) {
    hal_memory_barrier();
}

/**
 * @brief Enter idle/low-power mode
 */
static inline void hal_idle(void) {
#if defined(__AVR__)
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
#endif
}

/**
 * @brief Get CPU frequency
 */
static inline uint32_t hal_cpu_freq_hz(void) {
    return HAL_CPU_FREQ_HZ;
}

/*═══════════════════════════════════════════════════════════════════
 * ATOMIC OPERATIONS
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Atomic test-and-set (8-bit)
 *
 * AVR has no native TAS instruction, so we use interrupt disable.
 */
static inline uint8_t hal_atomic_test_and_set_u8(volatile uint8_t *ptr) {
    uint8_t old;
#if defined(__AVR__)
    uint8_t sreg = SREG;
    cli();
    old = *ptr;
    *ptr = 1;
    SREG = sreg;
#else
    old = __atomic_exchange_n(ptr, 1, __ATOMIC_SEQ_CST);
#endif
    return old;
}

/**
 * @brief Atomic exchange (8-bit)
 */
static inline uint8_t hal_atomic_exchange_u8(volatile uint8_t *ptr, uint8_t val) {
    uint8_t old;
#if defined(__AVR__)
    uint8_t sreg = SREG;
    cli();
    old = *ptr;
    *ptr = val;
    SREG = sreg;
#else
    old = __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
#endif
    return old;
}

/**
 * @brief Atomic exchange (16-bit) - requires interrupt disable on AVR
 */
static inline uint16_t hal_atomic_exchange_u16(volatile uint16_t *ptr, uint16_t val) {
    uint16_t old;
#if defined(__AVR__)
    uint8_t sreg = SREG;
    cli();
    old = *ptr;
    *ptr = val;
    SREG = sreg;
#else
    old = __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
#endif
    return old;
}

/**
 * @brief Atomic exchange (32-bit) - requires interrupt disable on AVR
 */
static inline uint32_t hal_atomic_exchange_u32(volatile uint32_t *ptr, uint32_t val) {
    uint32_t old;
#if defined(__AVR__)
    uint8_t sreg = SREG;
    cli();
    old = *ptr;
    *ptr = val;
    SREG = sreg;
#else
    old = __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
#endif
    return old;
}

/**
 * @brief Atomic compare-exchange (8-bit)
 */
static inline bool hal_atomic_compare_exchange_u8(volatile uint8_t *ptr,
                                                   uint8_t *expected,
                                                   uint8_t val) {
#if defined(__AVR__)
    bool success;
    uint8_t sreg = SREG;
    cli();
    if (*ptr == *expected) {
        *ptr = val;
        success = true;
    } else {
        *expected = *ptr;
        success = false;
    }
    SREG = sreg;
    return success;
#else
    return __atomic_compare_exchange_n(ptr, expected, val, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}

/**
 * @brief Atomic compare-exchange (16-bit)
 */
static inline bool hal_atomic_compare_exchange_u16(volatile uint16_t *ptr,
                                                    uint16_t *expected,
                                                    uint16_t val) {
#if defined(__AVR__)
    bool success;
    uint8_t sreg = SREG;
    cli();
    if (*ptr == *expected) {
        *ptr = val;
        success = true;
    } else {
        *expected = *ptr;
        success = false;
    }
    SREG = sreg;
    return success;
#else
    return __atomic_compare_exchange_n(ptr, expected, val, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}

/**
 * @brief Atomic compare-exchange (32-bit)
 */
static inline bool hal_atomic_compare_exchange_u32(volatile uint32_t *ptr,
                                                    uint32_t *expected,
                                                    uint32_t val) {
#if defined(__AVR__)
    bool success;
    uint8_t sreg = SREG;
    cli();
    if (*ptr == *expected) {
        *ptr = val;
        success = true;
    } else {
        *expected = *ptr;
        success = false;
    }
    SREG = sreg;
    return success;
#else
    return __atomic_compare_exchange_n(ptr, expected, val, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}

/*═══════════════════════════════════════════════════════════════════
 * AVR8-SPECIFIC FUNCTION PROTOTYPES
 *═══════════════════════════════════════════════════════════════════*/

/*
 * NOTE: Common HAL interface functions (hal_init, hal_reset, hal_timer_init,
 * etc.) are declared in arch/common/hal.h and implemented in hal_avr8.c.
 * They are NOT redeclared here to avoid type conflicts during include.
 */

/* Context switching - implemented in ASM (hal_context_switch.S) */
void hal_context_init(hal_context_t *ctx, void (*entry)(void), void *stack, size_t stack_size);
void hal_context_switch(hal_context_t *from, hal_context_t *to);

/* Internal helper for ISR */
extern void hal_timer_tick_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_AVR8_H */
