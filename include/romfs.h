/* SPDX-License-Identifier: MIT
 * See LICENSE file in the repository root for full license information.
 */

/**
 * @file romfs.h (LEGACY COMPATIBILITY HEADER)
 * @brief Forwarding header for backward compatibility
 *
 * DEPRECATED: This header is provided for backward compatibility only.
 * New code should directly include:
 *   #include "drivers/fs/romfs.h"
 *
 * This header will be removed in a future release.
 */

#ifndef ROMFS_H
#define ROMFS_H

/* Forward to new modular location (Phase 5 refactoring) */
#include "../drivers/fs/romfs.h"

/* Deprecated warning for GCC/Clang */
#if defined(__GNUC__) || defined(__clang__)
#  warning "include/romfs.h is deprecated. Use 'drivers/fs/romfs.h' instead."
#endif

#endif /* ROMFS_H */
