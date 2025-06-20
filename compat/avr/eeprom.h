#ifndef AVR_EEPROM_H
#define AVR_EEPROM_H
#include <stdint.h>
#ifndef __AVR__
extern uint8_t nk_sim_eeprom[1024];
static inline uint8_t eeprom_read_byte(const uint8_t *addr)
{
    return nk_sim_eeprom[(uintptr_t)addr];
}
static inline void eeprom_update_byte(uint8_t *addr, uint8_t value)
{
    nk_sim_eeprom[(uintptr_t)addr] = value;
}
#endif
#endif /* AVR_EEPROM_H */
