/* SPDX-License-Identifier: MIT */

/**
 * @file vectors.S
 * @brief ATmega328P Interrupt Vector Table
 *
 * This file defines the complete interrupt vector table for the ATmega328P.
 * Each entry is 4 bytes (2-word JMP instruction) for a total of 26 vectors.
 *
 * Memory Layout:
 * - Vector 0 (RESET): 0x0000 → __vectors (in startup.S)
 * - Vector 1-25:      0x0002-0x0032 → interrupt handlers
 * - Total size:       52 bytes (0x34)
 *
 * Reference: ATmega328P Datasheet Table 12-6 (Reset and Interrupt Vectors)
 */

#include <avr/io.h>

/*═══════════════════════════════════════════════════════════════════════
 * EXTERNAL SYMBOLS
 *═══════════════════════════════════════════════════════════════════════*/

.extern __vectors           /* Reset handler (startup.S) */
.extern __bad_interrupt     /* Default handler for unused vectors */

/* User interrupt handlers (weak symbols, can be overridden) */
.extern INT0_vect           /* External Interrupt Request 0 */
.extern INT1_vect           /* External Interrupt Request 1 */
.extern PCINT0_vect         /* Pin Change Interrupt Request 0 */
.extern PCINT1_vect         /* Pin Change Interrupt Request 1 */
.extern PCINT2_vect         /* Pin Change Interrupt Request 2 */
.extern WDT_vect            /* Watchdog Time-out Interrupt */
.extern TIMER2_COMPA_vect   /* Timer/Counter2 Compare Match A */
.extern TIMER2_COMPB_vect   /* Timer/Counter2 Compare Match B */
.extern TIMER2_OVF_vect     /* Timer/Counter2 Overflow */
.extern TIMER1_CAPT_vect    /* Timer/Counter1 Capture Event */
.extern TIMER1_COMPA_vect   /* Timer/Counter1 Compare Match A */
.extern TIMER1_COMPB_vect   /* Timer/Counter1 Compare Match B */
.extern TIMER1_OVF_vect     /* Timer/Counter1 Overflow */
.extern TIMER0_COMPA_vect   /* Timer/Counter0 Compare Match A */
.extern TIMER0_COMPB_vect   /* Timer/Counter0 Compare Match B */
.extern TIMER0_OVF_vect     /* Timer/Counter0 Overflow */
.extern SPI_STC_vect        /* SPI Serial Transfer Complete */
.extern USART_RX_vect       /* USART Rx Complete */
.extern USART_UDRE_vect     /* USART Data Register Empty */
.extern USART_TX_vect       /* USART Tx Complete */
.extern ADC_vect            /* ADC Conversion Complete */
.extern EE_READY_vect       /* EEPROM Ready */
.extern ANALOG_COMP_vect    /* Analog Comparator */
.extern TWI_vect            /* Two-wire Serial Interface (I2C) */
.extern SPM_READY_vect      /* Store Program Memory Ready */

/*═══════════════════════════════════════════════════════════════════════
 * INTERRUPT VECTOR TABLE (.vectors section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * The .vectors section is placed at address 0x0000 in flash
 * Each vector is a JMP instruction (4 bytes = 2 words)
 */
.section .vectors, "ax", @progbits
.global __vector_table
__vector_table:

/*─────────────────────────────────────────────────────────────────────
 * Vector 0: RESET
 * Address: 0x0000
 * Handler: __vectors (startup.S) → jumps to __init
 *─────────────────────────────────────────────────────────────────────*/
    jmp     __vectors         ; 0x0000: RESET → startup code

/*─────────────────────────────────────────────────────────────────────
 * Vector 1: INT0 - External Interrupt Request 0
 * Address: 0x0002
 *─────────────────────────────────────────────────────────────────────*/
.weak INT0_vect
.set  INT0_vect, __bad_interrupt
    jmp     INT0_vect         ; 0x0002: INT0

/*─────────────────────────────────────────────────────────────────────
 * Vector 2: INT1 - External Interrupt Request 1
 * Address: 0x0004
 *─────────────────────────────────────────────────────────────────────*/
.weak INT1_vect
.set  INT1_vect, __bad_interrupt
    jmp     INT1_vect         ; 0x0004: INT1

/*─────────────────────────────────────────────────────────────────────
 * Vector 3: PCINT0 - Pin Change Interrupt Request 0
 * Address: 0x0006
 *─────────────────────────────────────────────────────────────────────*/
.weak PCINT0_vect
.set  PCINT0_vect, __bad_interrupt
    jmp     PCINT0_vect       ; 0x0006: PCINT0

/*─────────────────────────────────────────────────────────────────────
 * Vector 4: PCINT1 - Pin Change Interrupt Request 1
 * Address: 0x0008
 *─────────────────────────────────────────────────────────────────────*/
.weak PCINT1_vect
.set  PCINT1_vect, __bad_interrupt
    jmp     PCINT1_vect       ; 0x0008: PCINT1

/*─────────────────────────────────────────────────────────────────────
 * Vector 5: PCINT2 - Pin Change Interrupt Request 2
 * Address: 0x000A
 *─────────────────────────────────────────────────────────────────────*/
.weak PCINT2_vect
.set  PCINT2_vect, __bad_interrupt
    jmp     PCINT2_vect       ; 0x000A: PCINT2

/*─────────────────────────────────────────────────────────────────────
 * Vector 6: WDT - Watchdog Time-out Interrupt
 * Address: 0x000C
 *─────────────────────────────────────────────────────────────────────*/
.weak WDT_vect
.set  WDT_vect, __bad_interrupt
    jmp     WDT_vect          ; 0x000C: WDT

/*─────────────────────────────────────────────────────────────────────
 * Vector 7: TIMER2_COMPA - Timer/Counter2 Compare Match A
 * Address: 0x000E
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER2_COMPA_vect
.set  TIMER2_COMPA_vect, __bad_interrupt
    jmp     TIMER2_COMPA_vect ; 0x000E: TIMER2_COMPA

/*─────────────────────────────────────────────────────────────────────
 * Vector 8: TIMER2_COMPB - Timer/Counter2 Compare Match B
 * Address: 0x0010
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER2_COMPB_vect
.set  TIMER2_COMPB_vect, __bad_interrupt
    jmp     TIMER2_COMPB_vect ; 0x0010: TIMER2_COMPB

/*─────────────────────────────────────────────────────────────────────
 * Vector 9: TIMER2_OVF - Timer/Counter2 Overflow
 * Address: 0x0012
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER2_OVF_vect
.set  TIMER2_OVF_vect, __bad_interrupt
    jmp     TIMER2_OVF_vect   ; 0x0012: TIMER2_OVF

/*─────────────────────────────────────────────────────────────────────
 * Vector 10: TIMER1_CAPT - Timer/Counter1 Capture Event
 * Address: 0x0014
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER1_CAPT_vect
.set  TIMER1_CAPT_vect, __bad_interrupt
    jmp     TIMER1_CAPT_vect  ; 0x0014: TIMER1_CAPT

/*─────────────────────────────────────────────────────────────────────
 * Vector 11: TIMER1_COMPA - Timer/Counter1 Compare Match A
 * Address: 0x0016
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER1_COMPA_vect
.set  TIMER1_COMPA_vect, __bad_interrupt
    jmp     TIMER1_COMPA_vect ; 0x0016: TIMER1_COMPA

/*─────────────────────────────────────────────────────────────────────
 * Vector 12: TIMER1_COMPB - Timer/Counter1 Compare Match B
 * Address: 0x0018
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER1_COMPB_vect
.set  TIMER1_COMPB_vect, __bad_interrupt
    jmp     TIMER1_COMPB_vect ; 0x0018: TIMER1_COMPB

/*─────────────────────────────────────────────────────────────────────
 * Vector 13: TIMER1_OVF - Timer/Counter1 Overflow
 * Address: 0x001A
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER1_OVF_vect
.set  TIMER1_OVF_vect, __bad_interrupt
    jmp     TIMER1_OVF_vect   ; 0x001A: TIMER1_OVF

/*─────────────────────────────────────────────────────────────────────
 * Vector 14: TIMER0_COMPA - Timer/Counter0 Compare Match A
 * Address: 0x001C
 * NOTE: Used by Avrix scheduler for 1 kHz tick (if enabled)
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER0_COMPA_vect
.set  TIMER0_COMPA_vect, __bad_interrupt
    jmp     TIMER0_COMPA_vect ; 0x001C: TIMER0_COMPA (scheduler tick)

/*─────────────────────────────────────────────────────────────────────
 * Vector 15: TIMER0_COMPB - Timer/Counter0 Compare Match B
 * Address: 0x001E
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER0_COMPB_vect
.set  TIMER0_COMPB_vect, __bad_interrupt
    jmp     TIMER0_COMPB_vect ; 0x001E: TIMER0_COMPB

/*─────────────────────────────────────────────────────────────────────
 * Vector 16: TIMER0_OVF - Timer/Counter0 Overflow
 * Address: 0x0020
 *─────────────────────────────────────────────────────────────────────*/
.weak TIMER0_OVF_vect
.set  TIMER0_OVF_vect, __bad_interrupt
    jmp     TIMER0_OVF_vect   ; 0x0020: TIMER0_OVF

/*─────────────────────────────────────────────────────────────────────
 * Vector 17: SPI_STC - SPI Serial Transfer Complete
 * Address: 0x0022
 *─────────────────────────────────────────────────────────────────────*/
.weak SPI_STC_vect
.set  SPI_STC_vect, __bad_interrupt
    jmp     SPI_STC_vect      ; 0x0022: SPI_STC

/*─────────────────────────────────────────────────────────────────────
 * Vector 18: USART_RX - USART Rx Complete
 * Address: 0x0024
 * NOTE: Used by TTY driver for interrupt-driven RX (optional)
 *─────────────────────────────────────────────────────────────────────*/
.weak USART_RX_vect
.set  USART_RX_vect, __bad_interrupt
    jmp     USART_RX_vect     ; 0x0024: USART_RX

/*─────────────────────────────────────────────────────────────────────
 * Vector 19: USART_UDRE - USART Data Register Empty
 * Address: 0x0026
 * NOTE: Used by TTY driver for interrupt-driven TX (optional)
 *─────────────────────────────────────────────────────────────────────*/
.weak USART_UDRE_vect
.set  USART_UDRE_vect, __bad_interrupt
    jmp     USART_UDRE_vect   ; 0x0026: USART_UDRE

/*─────────────────────────────────────────────────────────────────────
 * Vector 20: USART_TX - USART Tx Complete
 * Address: 0x0028
 *─────────────────────────────────────────────────────────────────────*/
.weak USART_TX_vect
.set  USART_TX_vect, __bad_interrupt
    jmp     USART_TX_vect     ; 0x0028: USART_TX

/*─────────────────────────────────────────────────────────────────────
 * Vector 21: ADC - ADC Conversion Complete
 * Address: 0x002A
 *─────────────────────────────────────────────────────────────────────*/
.weak ADC_vect
.set  ADC_vect, __bad_interrupt
    jmp     ADC_vect          ; 0x002A: ADC

/*─────────────────────────────────────────────────────────────────────
 * Vector 22: EE_READY - EEPROM Ready
 * Address: 0x002C
 *─────────────────────────────────────────────────────────────────────*/
.weak EE_READY_vect
.set  EE_READY_vect, __bad_interrupt
    jmp     EE_READY_vect     ; 0x002C: EE_READY

/*─────────────────────────────────────────────────────────────────────
 * Vector 23: ANALOG_COMP - Analog Comparator
 * Address: 0x002E
 *─────────────────────────────────────────────────────────────────────*/
.weak ANALOG_COMP_vect
.set  ANALOG_COMP_vect, __bad_interrupt
    jmp     ANALOG_COMP_vect  ; 0x002E: ANALOG_COMP

/*─────────────────────────────────────────────────────────────────────
 * Vector 24: TWI - Two-wire Serial Interface (I2C)
 * Address: 0x0030
 *─────────────────────────────────────────────────────────────────────*/
.weak TWI_vect
.set  TWI_vect, __bad_interrupt
    jmp     TWI_vect          ; 0x0030: TWI

/*─────────────────────────────────────────────────────────────────────
 * Vector 25: SPM_READY - Store Program Memory Ready
 * Address: 0x0032
 *─────────────────────────────────────────────────────────────────────*/
.weak SPM_READY_vect
.set  SPM_READY_vect, __bad_interrupt
    jmp     SPM_READY_vect    ; 0x0032: SPM_READY

/*═══════════════════════════════════════════════════════════════════════
 * NOTES
 *═══════════════════════════════════════════════════════════════════════
 *
 * Weak Symbols:
 * - All interrupt handlers are weak symbols
 * - Default to __bad_interrupt (infinite loop)
 * - Can be overridden by defining handler in user code (e.g., ISR macro)
 *
 * Vector Priorities:
 * - Lower address = higher priority
 * - RESET (0) has highest priority
 * - SPM_READY (25) has lowest priority
 *
 * Interrupt Handling:
 * - AVR automatically disables interrupts when entering ISR
 * - ISR must use RETI (not RET) to re-enable interrupts
 * - Nested interrupts require explicit SEI in handler
 *
 * Avrix Interrupt Usage:
 * - Vector 14 (TIMER0_COMPA): Scheduler tick (1 kHz)
 * - Vector 18 (USART_RX): TTY RX (optional)
 * - Vector 19 (USART_UDRE): TTY TX (optional)
 * - All others: Available for application use
 *
 * Memory Size:
 * - Total vectors: 26
 * - Bytes per vector: 4 (JMP instruction)
 * - Total size: 104 bytes (0x68)
 *   BUT: Vector 0 is in startup.S, so this file = 100 bytes
 */
