/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */
/* Minimal on-device GDB remote stub over a TTY. */

#ifndef AVR_GDBSTUB_H
#define AVR_GDBSTUB_H

#include "tty.h"

#ifdef __cplusplus
extern "C" {
#endif

void gdbstub_init(tty_t *tty);
void gdbstub_poll(void);
void gdbstub_break(void);

#ifdef __cplusplus
}
#endif

#endif /* AVR_GDBSTUB_H */
