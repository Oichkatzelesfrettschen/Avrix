/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file hal_avr8.c
 * @brief AVR8 Hardware Abstraction Layer Implementation
 *
 * This file implements the HAL interface defined in arch/common/hal.h
 * for the AVR8 architecture (ATmega series microcontrollers).
 *
 * Features:
 * - System initialization and reset
 * - Timer-based system tick (Timer0 @ 1 kHz)
 * - Delay functions (microsecond/millisecond precision)
 * - Capability detection
 * - Reset reason tracking
 */

#include "arch/common/hal.h"
#include "arch/avr8/include/hal_avr8.h"

#include <string.h>

/*═══════════════════════════════════════════════════════════════════
 * GLOBAL STATE
 *═══════════════════════════════════════════════════════════════════*/

static volatile uint32_t hal_tick_count = 0;
static hal_reset_reason_t hal_last_reset_reason = HAL_RESET_UNKNOWN;

/*═══════════════════════════════════════════════════════════════════
 * SYSTEM INITIALIZATION
 *═══════════════════════════════════════════════════════════════════*/

void hal_init(void) {
    /* Detect reset reason before MCUSR is cleared */
    hal_last_reset_reason = hal_reset_reason();

    /* Clear reset flags (required for watchdog handling) */
#if defined(MCUSR)
    MCUSR = 0;
#elif defined(MCUCSR)
    MCUCSR = 0;
#endif

    /* Disable watchdog timer (in case it was enabled) */
#if defined(WDTCSR)
    WDTCSR = 0;
#elif defined(WDTCR)
    WDTCR = 0;
#endif

    /* Initialize tick counter */
    hal_tick_count = 0;

    /* Additional platform-specific init can be added here */
}

void hal_reset(void) {
#if defined(__AVR__)
    /* Enable watchdog with shortest timeout (15 ms) */
    wdt_enable(WDTO_15MS);
    /* Wait for watchdog to trigger reset */
    for (;;) {
        /* Infinite loop - watchdog will reset us */
    }
#else
    /* Host build - use exit */
    exit(1);
#endif
}

hal_reset_reason_t hal_reset_reason(void) {
#if defined(MCUSR)
    uint8_t mcusr = MCUSR;
#elif defined(MCUCSR)
    uint8_t mcusr = MCUCSR;
#else
    return HAL_RESET_UNKNOWN;
#endif

    /* Check reset flags (priority order) */
    if (mcusr & _BV(PORF))  return HAL_RESET_POWER_ON;
    if (mcusr & _BV(EXTRF)) return HAL_RESET_EXTERNAL;
    if (mcusr & _BV(BORF))  return HAL_RESET_BROWNOUT;
    if (mcusr & _BV(WDRF))  return HAL_RESET_WATCHDOG;

    return HAL_RESET_UNKNOWN;
}

void hal_get_caps(hal_caps_t *caps) {
    if (!caps) return;

    memset(caps, 0, sizeof(hal_caps_t));

    caps->has_mpu          = HAL_HAS_MPU;
    caps->has_fpu          = HAL_HAS_FPU;
    caps->has_hardware_div = HAL_HAS_HARDWARE_DIV;
    caps->has_atomic_ops   = HAL_HAS_ATOMIC_U8;
    caps->has_dma          = HAL_HAS_DMA;
    caps->has_cache        = HAL_HAS_CACHE;
    caps->num_cores        = 1;  /* AVR8 is always single-core */
    caps->cpu_freq_hz      = HAL_CPU_FREQ_HZ;
}

const char *hal_arch_name(void) {
    return HAL_ARCH_NAME;
}

const char *hal_cpu_model(void) {
    return HAL_MCU_NAME;
}

/*═══════════════════════════════════════════════════════════════════
 * TIMER & TICK MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief System tick handler - called from ISR
 *
 * This is a weak symbol that can be overridden by the kernel scheduler.
 * Default implementation just increments tick counter.
 */
__attribute__((weak))
void hal_timer_tick_handler(void) {
    hal_tick_count++;
}

/**
 * @brief Timer0 Compare A ISR - 1 kHz system tick
 */
#if defined(HAL_TIMER0_COMPA_ISR)
ISR(HAL_TIMER0_COMPA_ISR) {
    hal_timer_tick_handler();
}
#endif

void hal_timer_init(uint32_t freq_hz) {
#if defined(__AVR__)
    /* We use Timer0 in CTC mode for system tick */
    /* CTC mode: WGM01=1, WGM00=0 */
    TCCR0A = _BV(WGM01);

    /* Prescaler: CS01=1, CS00=1 for /64 */
    TCCR0B = _BV(CS01) | _BV(CS00);

    /* Set compare value for desired frequency */
    /* OCR0A = (F_CPU / prescaler / freq) - 1 */
    OCR0A = (uint8_t)HAL_TIMER_RELOAD;

    /* Enable Timer0 Compare A interrupt */
    TIMSK0 = _BV(OCIE0A);

    /* Reset tick counter */
    hal_tick_count = 0;
#else
    (void)freq_hz;  /* Unused in host build */
#endif
}

uint32_t hal_timer_ticks(void) {
    uint32_t ticks;

    /* Read tick counter atomically */
    hal_irq_disable();
    ticks = hal_tick_count;
    hal_irq_enable();

    return ticks;
}

void hal_timer_delay_us(uint32_t us) {
#if defined(__AVR__)
    /* Busy-wait delay using cycle counting */
    /* Each iteration is roughly 4 cycles (load, decrement, branch) */
    /* delay_loops = (us * F_CPU) / 1000000 / 4 */

    /* For F_CPU = 16 MHz: 1 µs = 16 cycles = 4 loop iterations */
    uint32_t loops = (us * (F_CPU / 1000000UL)) / 4UL;

    /* Simple busy-wait loop */
    while (loops--) {
        __asm__ volatile ("nop");
    }
#else
    /* Host build - use usleep */
    #include <unistd.h>
    usleep(us);
#endif
}

void hal_timer_delay_ms(uint32_t ms) {
    /* Use microsecond delay in a loop */
    while (ms--) {
        hal_timer_delay_us(1000);
    }
}

/*═══════════════════════════════════════════════════════════════════
 * CONTEXT MANAGEMENT
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Initialize a new task context
 *
 * Sets up the initial stack frame for a new task. When the scheduler
 * switches to this context, execution will begin at the entry function.
 *
 * AVR stack frame (grows downward):
 *   [stack_base + stack_size]  <- SP starts here
 *   - Entry point address (PC) [2 bytes, little-endian]
 *   - SREG (status register)   [1 byte, I-flag set]
 *   - 32 general registers     [32 bytes, all zero]
 *   [lower addresses]
 *
 * @note This is compatible with the existing µ-UNIX context switch code
 */
void hal_context_init(hal_context_t *ctx, void (*entry)(void), void *stack, size_t stack_size) {
    if (!ctx || !entry || !stack || stack_size < 64) {
        return;  /* Invalid parameters */
    }

    /* Start at top of stack (AVR stack grows downward) */
    uint8_t *sp = (uint8_t *)stack + stack_size;

    /* Push entry point address (16-bit, little-endian) */
    *--sp = (uint16_t)entry & 0xFF;         /* PCL */
    *--sp = ((uint16_t)entry >> 8) & 0xFF;  /* PCH */

    /* Push SREG with interrupts enabled (I-flag = bit 7) */
    *--sp = 0x80;  /* SREG: I=1, all other flags cleared */

    /* Push 32 general-purpose registers (r0-r31), all zero */
    memset(sp - 32, 0, 32);
    sp -= 32;

    /* Save stack pointer to context */
    ctx->sp = (uint16_t)sp;
}

/**
 * @brief Context switch implementation
 *
 * This function is implemented in assembly (hal_context_switch.S)
 * because it requires direct manipulation of the stack pointer and
 * saving/restoring all CPU registers.
 *
 * See arch/avr8/common/hal_context_switch.S for implementation.
 */
/* Defined in hal_context_switch.S - declaration only */
extern void hal_context_switch(hal_context_t *from, hal_context_t *to);

/*═══════════════════════════════════════════════════════════════════
 * OPTIONAL: EARLY INIT (can be overridden)
 *═══════════════════════════════════════════════════════════════════*/

/**
 * @brief Platform-specific early initialization (weak)
 *
 * Called before hal_init() for critical early setup. Can be overridden
 * by platform-specific code if needed (e.g., external RAM init).
 */
__attribute__((weak))
void hal_early_init(void) {
    /* Default: do nothing */
}

/*═══════════════════════════════════════════════════════════════════
 * END OF FILE
 *═══════════════════════════════════════════════════════════════════*/
