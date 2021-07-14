/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.


 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
              Modified to use with Arduipi board http://hallard.me/arduipi
						  Changed to use modified bcm2835 and RF24 library

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

#include <cstdlib>
#include <iostream>
#include <RF24/RF24.h>

using namespace std;

/****************** Linux ***********************/
// Radio CE Pin, CSN Pin, SPI Speed
// CE Pin uses GPIO number with BCM and SPIDEV drivers, other platforms use their own pin numbering
// CS Pin addresses the SPI bus number at /dev/spidev<a>.<b>
// ie: RF24 radio(<ce_pin>, <a>*10+<b>); spidev1.0 is 10, spidev1.1 is 11 etc..

// Generic:
RF24 radio(22, 0);
/****************** Linux (BBB,x86,etc) ***********************/
// See http://nRF24.github.io/RF24/pages.html for more information on usage
// See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
// See https://www.kernel.org/doc/Documentation/spi/spidev for more information on SPIDEV

// Channel info
const uint8_t num_channels = 126;
uint8_t values[num_channels];

const int num_reps = 100;
int reset_array = 0;

int main(int argc, char** argv)
{
    // Print preamble

    // print example's name
    printf("%s", argv[0]);

    //
    // Setup and configure rf radio
    //
    radio.begin();

    radio.setAutoAck(false);

    // Get into standby mode
    radio.startListening();
    radio.stopListening();

    radio.printDetails();

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
                delayMicroseconds(128);
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
                printf("%x", min(0xf, (values[i] & 0xf)));
            else
                printf("-");

            ++i;
        }
        printf("\n");
    }

    return 0;
}

// vim:ai:cin:sts=2 sw=2 ft=cpp
