#include "fixed_point.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

/* Convenience constants for the Q8.8 type.  */
#define Q8_8_ONE      ((q8_8_t)0x0100)
#define Q8_8_NEG_ONE  ((q8_8_t)-0x0100)
#define Q8_8_MAX      INT16_MAX
#define Q8_8_MIN      INT16_MIN

static void boundary_tests(void)
{
    assert(q8_8_mul(Q8_8_MAX, Q8_8_ONE) == Q8_8_MAX);
    assert(q8_8_mul(Q8_8_MIN, Q8_8_ONE) == Q8_8_MIN);
    assert(q8_8_mul(Q8_8_MAX, Q8_8_MIN) == (q8_8_t)0x8081);
    assert(q8_8_mul(Q8_8_MIN, Q8_8_MIN) == (q8_8_t)0x0000);
    assert(q8_8_mul(Q8_8_ONE, Q8_8_ONE) == Q8_8_ONE);
    assert(q8_8_mul(Q8_8_MAX, 0) == 0);
    assert(q8_8_mul(Q8_8_MIN, 0) == 0);
    assert(q8_8_mul(Q8_8_NEG_ONE, Q8_8_NEG_ONE) == Q8_8_ONE);
    assert(q8_8_mul(Q8_8_MAX, Q8_8_NEG_ONE) == (q8_8_t)0x8101);
    assert(q8_8_mul(Q8_8_MIN, Q8_8_NEG_ONE) == Q8_8_MIN);
    assert(q8_8_mul(Q8_8_NEG_ONE, Q8_8_ONE) == Q8_8_NEG_ONE);
}

int main(void)
{
    boundary_tests();
    puts("q8_8_mul boundary tests passed");
    return 0;
}
