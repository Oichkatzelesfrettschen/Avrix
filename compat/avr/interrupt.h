#ifndef AVR_INTERRUPT_H
#define AVR_INTERRUPT_H
#ifndef __AVR__
# define ISR(vector, ...) void vector(void)
# define ISR_NAKED
# define sei() ((void)0)
# define cli() ((void)0)
#endif
#endif /* AVR_INTERRUPT_H */
