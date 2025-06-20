#ifndef AVR_PGMSPACE_H
#define AVR_PGMSPACE_H
#include <stdint.h>
#ifndef __AVR__
# define PROGMEM
# define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif
#endif /* AVR_PGMSPACE_H */
