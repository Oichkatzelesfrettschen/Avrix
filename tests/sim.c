#include <stdint.h>

/* Simulated hardware memory for host-side testing (provided by avr_stub.c) */
extern uint8_t nk_sim_io[0x40];
extern uint8_t nk_sim_eeprom[1024];
