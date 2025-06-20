#pragma once
#ifndef AVR_COMPAT_H
#define AVR_COMPAT_H

#if __STDC_VERSION__ >= 202311L
#  define AVR_UNUSED [[maybe_unused]]
#else
#  define AVR_UNUSED __attribute__((unused))
#endif

#endif /* AVR_COMPAT_H */
