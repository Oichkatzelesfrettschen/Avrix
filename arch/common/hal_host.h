/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file hal_host.h
 * @brief HAL Implementation for Host (Linux/x86)
 *
 * Provides dummy implementations and emulation for testing the OS on a PC.
 */

#ifndef HAL_HOST_H
#define HAL_HOST_H

#define _DEFAULT_SOURCE /* For usleep, etc. */
#define _XOPEN_SOURCE 700

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/time.h>

/* Architecture definitions */
typedef struct {
    ucontext_t uc;
    void *stack;
    size_t stack_size;
} hal_context_t;

/* Inline functions for HAL */

static inline void hal_init(void) {
    /* Initialize host emulation */
}

static inline void hal_reset(void) {
    exit(0);
}

static inline void hal_idle(void) {
    usleep(1000); /* Sleep 1ms */
}

static inline void hal_irq_enable(void) {
    /* Host simulation of IRQ enable */
}

static inline void hal_irq_disable(void) {
    /* Host simulation of IRQ disable */
}

static inline uint32_t hal_irq_save(void) {
    return 0;
}

static inline void hal_irq_restore(uint32_t state) {
    (void)state;
}

static inline void hal_context_switch(hal_context_t *from, hal_context_t *to) {
    if (from) {
        swapcontext(&from->uc, &to->uc);
    } else {
        setcontext(&to->uc);
    }
}

static inline void hal_context_init(hal_context_t *ctx, void (*entry)(void), void *stack, size_t stack_size) {
    getcontext(&ctx->uc);
    ctx->uc.uc_stack.ss_sp = stack;
    ctx->uc.uc_stack.ss_size = stack_size;
    ctx->uc.uc_link = NULL;
    ctx->stack = stack;
    ctx->stack_size = stack_size;
    makecontext(&ctx->uc, entry, 0);
}

/* Timer */
extern void hal_timer_tick_handler(void); /* From scheduler */

static inline void hal_timer_init(uint32_t freq_hz) {
    /* In a real host port, we'd set up a SIGALRM or similar. */
    (void)freq_hz;
}

/* Atomics (Host uses GCC builtins) */
static inline uint8_t hal_atomic_test_and_set_u8(volatile uint8_t *ptr) {
    return __sync_lock_test_and_set(ptr, 1);
}

static inline uint8_t hal_atomic_exchange_u8(volatile uint8_t *ptr, uint8_t val) {
    return __sync_lock_test_and_set(ptr, val);
}

static inline void hal_memory_barrier(void) {
    __sync_synchronize();
}

/* EEPROM Stubs */
static inline bool hal_eeprom_available(void) { return false; }
static inline uint16_t hal_eeprom_size(void) { return 0; }
static inline uint8_t hal_eeprom_read_byte(uint16_t addr) { (void)addr; return 0xFF; }
static inline void hal_eeprom_write_byte(uint16_t addr, uint8_t val) { (void)addr; (void)val; }

static inline void hal_eeprom_read_block(void *dest, uint16_t addr, size_t len) {
    (void)addr;
    memset(dest, 0xFF, len);
}

static inline void hal_eeprom_update_block(uint16_t addr, const void *src, size_t len) {
    (void)addr; (void)src; (void)len;
}

#ifdef __cplusplus
}
#endif

#endif /* HAL_HOST_H */
