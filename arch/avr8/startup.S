/* SPDX-License-Identifier: MIT */

/**
 * @file startup.S
 * @brief AVR8 Startup Code for ATmega328P
 *
 * This file contains the reset handler and initialization code that runs
 * before main() is called. It performs the following tasks:
 *
 * 1. Reset vector (jumps to __init)
 * 2. Initialize stack pointer
 * 3. Clear .bss section (uninitialized global variables)
 * 4. Copy .data section from flash to RAM (initialized globals)
 * 5. Call global constructors (C++)
 * 6. Call main()
 * 7. Call global destructors (if main() returns)
 * 8. Infinite loop (should never reach)
 *
 * Memory Layout (ATmega328P):
 * - Flash: 0x0000-0x7FFF (32 KB)
 * - SRAM:  0x0100-0x08FF (2 KB, after registers/I/O)
 * - EEPROM: 0x0000-0x03FF (1 KB)
 */

#include <avr/io.h>

/*═══════════════════════════════════════════════════════════════════════
 * ASSEMBLY MACROS
 *═══════════════════════════════════════════════════════════════════════*/

/* Stack pointer is initialized to RAMEND (top of SRAM) */
#define STACKEND  RAMEND

/* Linker-provided symbols (defined in linker script or by avr-gcc) */
.extern __data_start      /* Start of .data in RAM */
.extern __data_end        /* End of .data in RAM */
.extern __data_load_start /* Start of .data in Flash (.rodata) */
.extern __bss_start       /* Start of .bss in RAM */
.extern __bss_end         /* End of .bss in RAM */
.extern main              /* User's main() function */

/*═══════════════════════════════════════════════════════════════════════
 * RESET HANDLER (.init0 section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Entry point after MCU reset
 * This is the first code that runs after power-on or reset
 */
.section .init0, "ax", @progbits
.global __vectors
__vectors:
    rjmp    __init        ; Reset vector → jump to initialization

/*═══════════════════════════════════════════════════════════════════════
 * STACK POINTER INITIALIZATION (.init2 section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Initialize stack pointer to top of RAM
 * Must happen before any function calls or local variables
 */
.section .init2, "ax", @progbits
.global __init_stack
__init_stack:
    clr     r1                      ; R1 must always be zero (compiler convention)

    /* Set stack pointer to RAMEND */
    ldi     r28, lo8(STACKEND)      ; Load low byte of RAMEND into r28 (YL)
    ldi     r29, hi8(STACKEND)      ; Load high byte of RAMEND into r29 (YH)
    out     _SFR_IO_ADDR(SPL), r28  ; Set Stack Pointer Low
    out     _SFR_IO_ADDR(SPH), r29  ; Set Stack Pointer High

/*═══════════════════════════════════════════════════════════════════════
 * CLEAR .bss SECTION (.init4 section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Zero out .bss section (uninitialized global/static variables)
 * C standard requires uninitialized globals to be zero
 */
.section .init4, "ax", @progbits
.global __clear_bss
__clear_bss:
    ldi     r26, lo8(__bss_start)   ; Load start of .bss into X (r26:r27)
    ldi     r27, hi8(__bss_start)
    ldi     r30, lo8(__bss_end)     ; Load end of .bss into Z (r30:r31)
    ldi     r31, hi8(__bss_end)
    rjmp    2f                      ; Jump to loop condition check

1:  /* Loop: zero one byte at a time */
    st      X+, r1                  ; Store zero (r1) and increment X
2:  /* Loop condition: while (X < bss_end) */
    cp      r26, r30                ; Compare X with bss_end (low byte)
    cpc     r27, r31                ; Compare with carry (high byte)
    brne    1b                      ; Branch if not equal (continue loop)

/*═══════════════════════════════════════════════════════════════════════
 * COPY .data SECTION FROM FLASH TO RAM (.init4 section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Copy .data section from Flash to RAM
 * Initialized global/static variables are stored in Flash and must be
 * copied to RAM before use
 */
.section .init4, "ax", @progbits
.global __copy_data
__copy_data:
    ldi     r26, lo8(__data_start)  ; Load destination (RAM) into X
    ldi     r27, hi8(__data_start)
    ldi     r30, lo8(__data_load_start)  ; Load source (Flash) into Z
    ldi     r31, hi8(__data_load_start)
    ldi     r24, lo8(__data_end)    ; Load end address
    ldi     r25, hi8(__data_end)
    rjmp    4f                      ; Jump to loop condition

3:  /* Loop: copy one byte at a time */
    lpm     r0, Z+                  ; Load byte from Flash (Z) into r0, inc Z
    st      X+, r0                  ; Store to RAM (X) and increment X
4:  /* Loop condition: while (X < data_end) */
    cp      r26, r24                ; Compare X with data_end (low byte)
    cpc     r27, r25                ; Compare with carry (high byte)
    brne    3b                      ; Branch if not equal (continue loop)

/*═══════════════════════════════════════════════════════════════════════
 * CALL GLOBAL CONSTRUCTORS (.init6 section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Call C++ global constructors (if any)
 * For C-only projects, this section is typically empty
 */
.section .init6, "ax", @progbits
.global __do_global_ctors
__do_global_ctors:
    /* Nothing to do for pure C projects */
    /* C++ projects would call __do_global_ctors here */

/*═══════════════════════════════════════════════════════════════════════
 * JUMP TO main() (.init9 section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Call user's main() function
 * main() should never return, but if it does, we handle it gracefully
 */
.section .init9, "ax", @progbits
.global __init
__init:
    call    main                    ; Call main() function

    /* If main() returns (should never happen), jump to exit handler */
    rjmp    __stop_program

/*═══════════════════════════════════════════════════════════════════════
 * CALL GLOBAL DESTRUCTORS (.fini6 section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Call C++ global destructors (if main() returns)
 */
.section .fini6, "ax", @progbits
.global __do_global_dtors
__do_global_dtors:
    /* Nothing to do for pure C projects */

/*═══════════════════════════════════════════════════════════════════════
 * INFINITE LOOP (.fini9 section)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Infinite loop (should never reach here)
 * If main() returns, we end up here and halt
 */
.section .fini9, "ax", @progbits
.global __stop_program
__stop_program:
    cli                             ; Disable interrupts
1:  rjmp    1b                      ; Infinite loop (halt)

/*═══════════════════════════════════════════════════════════════════════
 * DEFAULT INTERRUPT HANDLER (weak symbol)
 *═══════════════════════════════════════════════════════════════════════*/

/**
 * Default handler for unused interrupts
 * Can be overridden by defining __bad_interrupt() in user code
 */
.section .text.__bad_interrupt, "ax", @progbits
.global __bad_interrupt
.weak   __bad_interrupt
__bad_interrupt:
    cli                             ; Disable interrupts
1:  rjmp    1b                      ; Halt (infinite loop)

/*═══════════════════════════════════════════════════════════════════════
 * NOTES
 *═══════════════════════════════════════════════════════════════════════
 *
 * Initialization Sections (.initN):
 * - .init0: Reset vector (earliest, jumps to __init)
 * - .init1: (reserved)
 * - .init2: Stack pointer initialization
 * - .init3: (user-defined)
 * - .init4: Clear .bss, copy .data
 * - .init5: (user-defined)
 * - .init6: Call C++ constructors
 * - .init7: (user-defined)
 * - .init8: (user-defined)
 * - .init9: Jump to main()
 *
 * Finalization Sections (.finiN):
 * - .fini0-5: (user-defined)
 * - .fini6: Call C++ destructors
 * - .fini7-8: (user-defined)
 * - .fini9: Infinite loop (halt)
 *
 * Register Usage:
 * - r0: Temporary (used by LPM instruction)
 * - r1: Always zero (compiler convention)
 * - r24-r25: Used for addresses
 * - r26-r27 (X): Pointer to RAM (.bss, .data)
 * - r28-r29 (Y): Stack pointer initialization
 * - r30-r31 (Z): Pointer to Flash (.data source)
 *
 * Stack Pointer:
 * - Initialized to RAMEND (0x08FF for ATmega328P)
 * - Grows downward (decrements on push)
 * - Must be initialized before any function calls
 *
 * Memory Sections:
 * - .text: Code (Flash)
 * - .data: Initialized globals (copied from Flash to RAM)
 * - .bss: Uninitialized globals (zeroed in RAM)
 * - .rodata: Read-only data (Flash, accessed via PROGMEM)
 */
