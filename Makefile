MCU ?= atmega328p
F_CPU ?= 16000000UL
CC=avr-gcc
# avr-gcc 7.x does not yet support the C23 standard.  The code is written to
# compile cleanly as C11 which remains the newest dialect available on this
# toolchain.
CFLAGS=-std=c11 -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -flto -ffunction-sections \
       -fdata-sections -Wall -Wextra -Werror -pedantic -Iinclude
LDFLAGS=-mmcu=$(MCU) -Wl,--gc-sections -flto

SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

all: libavrix.a

libavrix.a: $(OBJS)
	avr-ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libavrix.a

.PHONY: all clean
