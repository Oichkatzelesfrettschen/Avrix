/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file tty.h (LEGACY COMPATIBILITY HEADER)
 * @brief Forwarding header for backward compatibility
 *
 * DEPRECATED: This header is provided for backward compatibility only.
 * New code should directly include:
 *   #include "drivers/tty/tty.h"
 *
 * This header will be removed in a future release.
 */

#ifndef TTY_H
#define TTY_H

/* Forward to new modular location (Phase 5 refactoring) */
#include "../drivers/tty/tty.h"

/* Deprecated warning for GCC/Clang */
#if defined(__GNUC__) || defined(__clang__)
#  warning "include/tty.h is deprecated. Use 'drivers/tty/tty.h' instead."
#endif

#endif /* TTY_H */
