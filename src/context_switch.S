/*═══════════════════════════════════════════════════════════════════
 * context_switch.S - Minimal context switch for classic AVR cores
 *═══════════════════════════════════════════════════════════════════*/

#include <avr/io.h>

.section .text
.global _nk_switch_context

/* void _nk_switch_context(uint8_t **from_sp, uint8_t *to_sp) */
_nk_switch_context:
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

    in   r26, __SP_L__
    in   r27, __SP_H__
    st   X+, r26
    st   X,  r27

    movw r26, r22
    out  __SP_L__, r26
    out  __SP_H__, r27

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

    ret

