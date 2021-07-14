/*
 * Copyright (C)
 *  2021 Jannis Achstetter (kripton)
 *  2021 Brendan Doherty (2bndy5)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * @file RF24_arch_config.h
 * General defines and includes for RF24 using The Pico SDK
 */

#ifndef __ARCH_CONFIG_H__
#define __ARCH_CONFIG_H__

#include "spi.h"
#include "gpio.h"
#include <string.h>

/** Define a specific platform name for this configuration */
#define RF24_RP2

#define _BV(x) (1 << (x))
#define _SPI SPI
#define RF24_SPI_PTR

static SPI spi;

#ifdef SERIAL_DEBUG
    #define IF_SERIAL_DEBUG(x) ({x;})
#else
    #define IF_SERIAL_DEBUG(x)
#endif

typedef uint16_t prog_uint16_t;
#define PSTR(x) (x)
#define printf_P printf
#define strlen_P strlen
#define PROGMEM
#define pgm_read_word(p) (*(p))
#define PRIPSTR "%s"
#define pgm_read_byte(p) (*(p))

#define pgm_read_ptr(p) (*(p))

// Function, constant map as a result of migrating from Arduino
#define LOW GPIO::OUTPUT_LOW
#define HIGH GPIO::OUTPUT_HIGH
#define INPUT GPIO::DIRECTION_IN
#define OUTPUT GPIO::DIRECTION_OUT
#define digitalWrite(pin, value) GPIO::write(pin, value)
#define pinMode(pin, direction) GPIO::open(pin, direction)
#define delay(milisec) sleep_ms(milisec)
#define delayMicroseconds(usec) sleep_us(usec)
#define millis() to_ms_since_boot(get_absolute_time())

#endif // __ARCH_CONFIG_H__

/**@}*/
