/*
 * Door RPC entry point - AVR assembly implementation
 *
 * External symbols referenced:
 * - door_slab: Shared memory buffer for door messages
 * - nk_switch_to: Task switching function
 */

#include <avr/io.h>

.section .text
.global _nk_door
.type _nk_door, @function

/* External symbols */
.extern door_slab
.extern nk_switch_to

; void _nk_door(const uint8_t *src, uint8_t len, uint8_t tid)
; r24:r25 -> src pointer
; r22     -> len (bytes)
; r20     -> target tid
_nk_door:
    push r28
    push r29

    movw r30, r24        ; Z = src
    ldi  r28, lo8(door_slab)
    ldi  r29, hi8(door_slab)
    mov  r18, r22        ; byte counter
1:
    cpi  r18, 0
    breq 2f
    ld   r19, Z+
    st   Y+, r19
    dec  r18
    rjmp 1b
2:
    mov  r24, r20        ; nk_switch_to parameter
    rcall nk_switch_to

    pop  r29
    pop  r28
    ret

.size _nk_door, .-_nk_door
