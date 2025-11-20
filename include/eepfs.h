/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file eepfs.h (LEGACY COMPATIBILITY HEADER)
 * @brief Forwarding header for backward compatibility
 *
 * DEPRECATED: This header is provided for backward compatibility only.
 * New code should directly include:
 *   #include "drivers/fs/eepfs.h"
 *
 * This header will be removed in a future release.
 */

#ifndef EEPFS_H
#define EEPFS_H

/* Forward to new modular location (Phase 5 refactoring) */
#include "../drivers/fs/eepfs.h"

/* Deprecated warning for GCC/Clang */
#if defined(__GNUC__) || defined(__clang__)
#  warning "include/eepfs.h is deprecated. Use 'drivers/fs/eepfs.h' instead."
#endif

#endif /* EEPFS_H */
