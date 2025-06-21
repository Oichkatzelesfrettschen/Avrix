/*───────────────────────────────────────────────────────────────────────
 * switch_task.S — Context switch + scheduler update for classic AVR
 *
 *   void _nk_switch_task(uint8_t **from_sp,
 *                        uint8_t *to_sp,
 *                        uint8_t *current_ptr,
 *                        uint8_t  next_tid);
 *
 * Saves caller-saved registers, stores the current stack pointer in
 * *from_sp, loads the new stack pointer from to_sp, updates *current_ptr
 * with next_tid and restores registers.
 *
 * Cycle count: ~36 push + 13 core + 36 pop + 4 ret = ~89 cycles
 *───────────────────────────────────────────────────────────────────────*/

#include <avr/io.h>

.section .text
.global _nk_switch_task
.type _nk_switch_task, @function

/* Caller-saved register push/pop (r2-r17, r28-r29) --------------------*/
.macro PUSH_CALLER
    push r2
    push r3
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push r16
    push r17
    push r28
    push r29
.endm

.macro POP_CALLER
    pop  r29
    pop  r28
    pop  r17
    pop  r16
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  r11
    pop  r10
    pop  r9
    pop  r8
    pop  r7
    pop  r6
    pop  r5
    pop  r4
    pop  r3
    pop  r2
.endm

/* void _nk_switch_task(uint8_t **from_sp,
 *                      uint8_t *to_sp,
 *                      uint8_t *current_ptr,
 *                      uint8_t  next_tid) */
_nk_switch_task:
    PUSH_CALLER

    movw r30, r24        ; Z = from_sp
    in   r26, __SP_L__
    in   r27, __SP_H__
    st   Z+, r26         ; *from_sp = SP
    st   Z,  r27

    movw r26, r22        ; r22:r23 = to_sp
    out  __SP_L__, r26
    out  __SP_H__, r27

    movw r30, r20        ; Z = current_ptr
    st   Z,  r19         ; *current_ptr = next_tid

    POP_CALLER
    ret

.size _nk_switch_task, .-_nk_switch_task
