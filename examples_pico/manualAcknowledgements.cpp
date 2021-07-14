/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty (2bndy5)
 */

/**
 * A simple example of sending data from 1 nRF24L01 transceiver to another
 * with manually transmitted (non-automatic) Acknowledgement (ACK) payloads.
 * This example still uses ACK packets, but they have no payloads. Instead the
 * acknowledging response is sent with `write()`. This tactic allows for more
 * updated acknowledgement payload data, where actual ACK payloads' data are
 * outdated by 1 transmission because they have to loaded before receiving a
 * transmission.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Terminal to change each node's behavior.
 */
#include "pico/stdlib.h"  // printf(), sleep_ms(), getchar_timeout_us(), to_us_since_boot(), get_absolute_time()
#include "pico/bootrom.h" // reset_usb_boot()
#include <tusb.h>         // tud_cdc_connected()
#include <RF24.h>         // RF24 radio object
#include "defaultPins.h"  // board presumptive default pin numbers for CE_PIN and CSN_PIN

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// Used to control whether this node is sending or receiving
bool role = false;  // true = TX node, false = RX node

// For this example, we'll be using a payload containing
// a string & an integer number that will be incremented
// on every successful transmission.
// Make a data structure to store the entire payload of different datatypes
struct PayloadStruct {
  char message[7];          // only using 6 characters for TX & RX payloads
  uint8_t counter;
};
PayloadStruct payload;

bool setup()
{
    // append a NULL terminating character for printing as a c-string
    payload.message[6] = 0;

    // Let these addresses be used for the pair
    uint8_t address[][6] = {"1Node", "2Node"};
    // It is very helpful to think of an address as a path instead of as
    // an identifying device destination

    // to use different addresses on a pair of radios, we need a variable to
    // uniquely identify which address this radio will use to transmit
    bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

    // wait here until the CDC ACM (serial port emulation) is connected
    while (!tud_cdc_connected()) {
        sleep_ms(10);
    }

    // initialize the transceiver on the SPI bus
    if (!radio.begin()) {
        printf("radio hardware is not responding!!\n");
        return false;
    }

    // print example's introductory prompt
    printf("RF24/examples_pico/manualAcknowledgements\n");

    // To set the radioNumber via the Serial monitor on startup
    printf("Which radio is this? Enter '0' or '1'. Defaults to '0'\n");
    char input = getchar();
    radioNumber = input == 49;
    printf("radioNumber = %d\n", (int)radioNumber);

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW); // RF24_PA_MAX is default.

    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need to transmit a float
    radio.setPayloadSize(sizeof(payload)); // char[7] & uint8_t datatypes occupy 8 bytes

    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

    if (role) {
        // setup the TX node

        memcpy(payload.message, "Hello ", 6); // set the outgoing message
        radio.stopListening();                // put radio in TX mode
    }
    else {
        // setup the RX node

        memcpy(payload.message, "World ", 6); // set the outgoing message
        radio.startListening();               // put radio in RX mode
    }

    // For debugging info
    // printf_begin();             // needed only once for printing details
    // radio.printDetails();       // (smaller) function that prints raw register values
    // radio.printPrettyDetails(); // (larger) function that prints human readable data

    // role variable is hardcoded to RX behavior, inform the user of this
    printf("*** PRESS 'T' to begin transmitting to the other node\n");

    return true;
} // setup()

void loop()
{
    if (role) {
        // This device is a TX node

        uint64_t start_timer = to_us_since_boot(get_absolute_time()); // start the timer
        bool report = radio.write(&payload, sizeof(payload));         // transmit & save the report

        if (report) {
            // transmission successful; wait for response and print results

            radio.startListening();                                              // put in RX mode
            uint64_t start_timeout = to_ms_since_boot(get_absolute_time());      // timer to detect timeout
            while (!radio.available()) {                                         // wait for response
                if (to_ms_since_boot(get_absolute_time()) - start_timeout > 200)                  // only wait 200 ms
                    break;
            }
            uint64_t end_timer = to_us_since_boot(get_absolute_time());          // end the timer
            radio.stopListening();                                               // put back in TX mode

            // print summary of transactions
            printf("Transmission successful!");            // payload was delivered
            uint8_t pipe;
            if (radio.available(&pipe)) {                  // is there a payload received
                // print details about outgoing payload
                printf(" Round-trip delay: %llu us. Sent: %s%d",
                       end_timer - start_timer,
                       payload.message,
                       payload.counter);

                PayloadStruct received;
                radio.read(&received, sizeof(received)); // get payload from RX FIFO

                // print details about incoming payload
                printf(" Received %d bytes on pipe %d: %s%d\n",
                       radio.getPayloadSize(),
                       pipe,
                       received.message,
                       received.counter);

                payload.counter = received.counter; // save incoming counter for next outgoing counter
            }
            else {
                printf(" Recieved no response.\n"); // no response received
            }
        }
        else {
            printf("Transmission failed or timed out\n"); // payload was not delivered
        } // report

        // to make this example readable in the serial terminal
        sleep_ms(1000); // slow transmissions down by 1 second
    }
    else {
        // This device is a RX node

        uint8_t pipe;
        if (radio.available(&pipe)) {                // is there a payload? get the pipe number that recieved it
            PayloadStruct received;
            radio.read(&received, sizeof(received)); // get incoming payload
            payload.counter = received.counter + 1;  // increment incoming counter for next outgoing response

            // transmit response & save result to `report`
            radio.stopListening();                       // put in TX mode
            radio.writeFast(&payload, sizeof(payload));  // load response to TX FIFO
            bool report = radio.txStandBy(150);          // keep retrying for 150 ms
            radio.startListening();                      // put back in RX mode

            // print summary of transactions, starting with details about incoming payload
            printf("Received %d bytes on pipe %d: %s%d",
                   radio.getPayloadSize(),
                   pipe,
                   received.message,
                   received.counter);

            if (report) {
                // print outgoing payload and its counter
                printf(" Sent: %s%d", payload.message, payload.counter);
            }
            else {
                printf(" Response failed.\n");   // failed to send response
            }
        }
    } // role

    char input = getchar_timeout_us(0); // get char from buffer for user input
    if (input != PICO_ERROR_TIMEOUT) {
        // change the role via the serial terminal

        if ((input == 'T' || input == 't') && !role) {
            // Become the TX node

            role = true;
            memcpy(payload.message, "Hello ", 6); // set the outgoing message
            printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n");
            radio.stopListening();                // put in TX mode

        }
        else if ((input == 'R' || input == 'r') && role) {
            // Become the RX node

            role = false;
            memcpy(payload.message, "World ", 6); // set the response message
            printf("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n");
            radio.startListening();               // put in RX mode
        }
        else if (input == 'b' || input == 'B') {
            // reset to bootloader
            radio.powerDown();
            reset_usb_boot(0, 0);
        }
    }
} // loop

int main()
{
    stdio_init_all(); // init necessary IO for the RP2040

    while (!setup()) { // if radio.begin() failed
        // hold program in infinite attempts to initialize radio
    }
    while (true) {
        loop();
    }
    return 0; // we will never reach this
}
