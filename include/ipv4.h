/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file ipv4.h (LEGACY COMPATIBILITY HEADER)
 * @brief Forwarding header for backward compatibility
 *
 * DEPRECATED: This header is provided for backward compatibility only.
 * New code should directly include:
 *   #include "drivers/net/ipv4.h"
 *
 * This header will be removed in a future release.
 */

#ifndef IPV4_H
#define IPV4_H

/* Forward to new modular location (Phase 5 refactoring) */
#include "../drivers/net/ipv4.h"

/* Deprecated warning for GCC/Clang */
#if defined(__GNUC__) || defined(__clang__)
#  warning "include/ipv4.h is deprecated. Use 'drivers/net/ipv4.h' instead."
#endif

#endif /* IPV4_H */
