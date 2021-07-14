/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.


 06/04/2021 : Brendan Doherty (https://github.com/2bndy5)
              Modified to use with PicoSDK

 */

/**
 * Channel scanner
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

#include "pico/stdlib.h"  // printf(), sleep_ms(), getchar_timeout_us(), to_us_since_boot(), get_absolute_time()
#include "pico/bootrom.h" // reset_usb_boot()
#include <tusb.h>         // tud_cdc_connected()
#include <RF24.h>         // RF24 radio object, rf24_min()
#include "defaultPins.h"  // board presumptive default pin numbers for CE_PIN and CSN_PIN

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// Channel info
const uint8_t num_channels = 126;
uint8_t values[num_channels];

const int num_reps = 100;
int reset_array = 0;

int main()
{
    stdio_init_all(); // init necessary IO for the RP2040

    // wait here until the CDC ACM (serial port emulation) is connected
    while (!tud_cdc_connected()) {
        sleep_ms(10);
    }

    // initialize the transceiver on the SPI bus
    while (!radio.begin()) {
        printf("radio hardware is not responding!!\n");
    }

    // print example's name
    printf("RF24/examples_pico/scanner\n");

    radio.setAutoAck(false);

    // Get into standby mode
    radio.startListening();
    radio.stopListening();

    // radio.printDetails();

    // Print out header, high then low digit
    int i = 0;

    while (i < num_channels) {
        printf("%x", i >> 4);
        ++i;
    }
    printf("\n");

    i = 0;
    while (i < num_channels) {
        printf("%x", i & 0xf);
        ++i;
    }
    printf("\n");

    // forever loop
    while (1) {
        // Clear measurement values
        memset(values, 0, sizeof(values));

        // Scan all channels num_reps times
        int rep_counter = num_reps;
        while (rep_counter--) {

            int i = num_channels;
            while (i--) {

                // Select this channel
                radio.setChannel(i);

                // Listen for a little
                radio.startListening();
                sleep_us(128);
                radio.stopListening();

                // Did we get a carrier?
                if (radio.testCarrier()) {
                    ++values[i];
                }
            }
        }

        // Print out channel measurements, clamped to a single hex digit
        i = 0;
        while (i < num_channels) {
            if (values[i])
                printf("%x", rf24_min(0xf, (values[i] & 0xf)));
            else
                printf("-");

            ++i;
        }
        printf("\n");
    }

    return 0;
}
