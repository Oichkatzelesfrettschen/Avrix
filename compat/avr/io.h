#ifndef AVR_IO_H
#define AVR_IO_H
#include <stdint.h>
#ifndef __AVR__
extern uint8_t nk_sim_io[0x40];
# define _SFR_IO8(x) nk_sim_io[(x)]
# define _BV(x) (1U << (x))
# define TCCR0A nk_sim_io[0]
# define TCCR0B nk_sim_io[1]
# define OCR0A  nk_sim_io[2]
# define TIMSK0 nk_sim_io[3]
# define WGM01  0
# define CS01   0
# define CS00   0
# define OCIE0A 0
#endif
#endif /* AVR_IO_H */
