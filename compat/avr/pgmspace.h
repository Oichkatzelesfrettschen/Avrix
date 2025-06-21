#ifndef AVR_PGMSPACE_H
#define AVR_PGMSPACE_H
#include <stdint.h>
#ifndef __AVR__
# define PROGMEM
# define pgm_read_byte(addr) (*(const uint8_t*)(addr))
# define pgm_read_word(addr) (*(const uintptr_t*)(addr))
# define memcpy_P(dest, src, n) memcpy((dest), (src), (n))
#endif
#endif /* AVR_PGMSPACE_H */
