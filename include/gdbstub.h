#ifndef AVRX_GDBSTUB_H
#define AVRX_GDBSTUB_H
/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the on-device GDB stub. */
void gdbstub_init(void);

/** Trigger a breakpoint handled by GDB. */
void gdbstub_break(void);

#ifdef __cplusplus
}
#endif

#endif /* AVRX_GDBSTUB_H */
