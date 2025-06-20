#ifndef AVR_DOOR_H
#define AVR_DOOR_H
/*  Tiny descriptor–based RPC for μ-UNIX on ATmega328P.
    Implements a flattened Cap’n-Proto-style zero-copy slab plus
    Solaris-like synchronous “doors”. */

#include <stdint.h>
#ifdef __cplusplus
extern "C" {         /* pure C23 but usable from C++ */
#endif

/* ---------- configuration ------------------------------------------------ */
#ifndef DOOR_SLOTS
#  define DOOR_SLOTS       4          /* per-task descriptors */
#endif
#ifndef DOOR_SLAB_SIZE
#  define DOOR_SLAB_SIZE 128          /* bytes, multiple of 8 */
#endif
/* ------------------------------------------------------------------------- */

/* Descriptor lives in the per-task vector (in .noinit). */
typedef struct {
    uint8_t tgt_tid;              /* callee task id                         */
    uint8_t words : 4;            /* message length in 8-byte words (1-15)  */
    uint8_t flags : 4;            /* option flags (bit-mapped)              */
} door_t;

/* One slab shared by all tasks (defined in door.c, .noinit) */
extern uint8_t door_slab[DOOR_SLAB_SIZE];

/* --------- user API ------------------------------------------------------ */
void     door_register(uint8_t idx, uint8_t target, uint8_t words,
                       uint8_t flags);
void     door_call    (uint8_t idx, void *buf);
void     door_return  (void);                 /* callee -> caller          */

const void *door_message(void);               /* callee side accessors     */
uint8_t     door_words  (void);
uint8_t     door_flags  (void);

#ifdef __cplusplus
}
#endif
#endif /* AVR_DOOR_H */
