/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file slip.h (LEGACY COMPATIBILITY HEADER)
 * @brief Forwarding header for backward compatibility
 *
 * DEPRECATED: This header is provided for backward compatibility only.
 * New code should directly include:
 *   #include "drivers/net/slip.h"
 *
 * This header will be removed in a future release.
 */

#ifndef SLIP_H
#define SLIP_H

/* Forward to new modular location (Phase 5 refactoring) */
#include "../drivers/net/slip.h"

/* Deprecated warning for GCC/Clang */
#if defined(__GNUC__) || defined(__clang__)
#  warning "include/slip.h is deprecated. Use 'drivers/net/slip.h' instead."
#endif

#endif /* SLIP_H */
