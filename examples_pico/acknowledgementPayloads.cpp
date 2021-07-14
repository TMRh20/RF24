/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty (2bndy5)
 */

/**
 * A simple example of sending data from 1 nRF24L01 transceiver to another
 * with Acknowledgement (ACK) payloads attached to ACK packets.
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
bool role = false;  // true = TX role, false = RX role

// For this example, we'll be using a payload containing
// a string & an integer number that will be incremented
// on every successful transmission.
// Make a data structure to store the entire payload of different datatypes
struct PayloadStruct {
    char message[7];   // only using 6 characters for TX & ACK payloads
    uint8_t counter;
};
PayloadStruct payload;


bool setup() {

    // an identifying device destination
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
    printf("RF24/examples_pico/acknowledgementPayloads\n");

    // To set the radioNumber via the Serial monitor on startup
    printf("Which radio is this? Enter '0' or '1'. Defaults to '0'\n");
    char input = getchar();
    radioNumber = input == 49;
    printf("radioNumber = %d\n", (int)radioNumber);

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW);     // RF24_PA_MAX is default.

    // to use ACK payloads, we need to enable dynamic payload lengths (for all nodes)
    radio.enableDynamicPayloads();    // ACK payloads are dynamically sized

    // Acknowledgement packets have no payloads by default. We need to enable
    // this feature for all nodes (TX & RX) to use ACK payloads.
    radio.enableAckPayload();

    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

    // additional setup specific to the node's role
    if (role) {
        // setup the TX payload

        memcpy(payload.message, "Hello ", 6);                       // set the payload message
        radio.stopListening();                                      // put radio in TX mode
    }
    else {
        // setup the ACK payload & load the first response into the FIFO

        memcpy(payload.message, "World ", 6);                       // set the payload message
        // load the payload for the first received transmission on pipe 0
        radio.writeAckPayload(1, &payload, sizeof(payload));

        radio.startListening();                                     // put radio in RX mode
    }

    // For debugging info
    // radio.printDetails();       // (smaller) function that prints raw register values
    // radio.printPrettyDetails(); // (larger) function that prints human readable data

    // role variable is hardcoded to RX behavior, inform the user of this
    printf("*** PRESS 'T' to begin transmitting to the other node\n");

    return true;
}

void loop() {

    if (role) {
        // This device is a TX node

        uint64_t start_timer = to_us_since_boot(get_absolute_time()); // start the timer
        bool report = radio.write(&payload, sizeof(payload));         // transmit & save the report
        uint64_t end_timer = to_us_since_boot(get_absolute_time());   // end the timer

        if (report) {

            // print details about outgoing payload
            printf("Transmission successful! Time to transmit = %llu us. Sent: %s%d",
                   end_timer - start_timer,
                   payload.message,
                   payload.counter);

            uint8_t pipe;
            if (radio.available(&pipe)) {                          // is there an ACK payload? grab the pipe number that received it
                PayloadStruct received;
                radio.read(&received, sizeof(received));             // get incoming ACK payload

                // print details about incoming payload
                printf(" Recieved %d bytes on pipe %d: %s%d\n",
                       radio.getDynamicPayloadSize(),
                       pipe,
                       received.message,
                       received.counter);

                // save incoming counter & increment for next outgoing
                payload.counter = received.counter + 1;

            }
            else {
                printf(" Recieved: an empty ACK packet\n"); // empty ACK packet received
            }


        }
        else {
            printf("Transmission failed or timed out\n");    // payload was not delivered
        }

        // to make this example readable in the serial terminal
        sleep_ms(1000); // slow transmissions down by 1 second
    }
    else {
        // This device is a RX node

        uint8_t pipe;
        if (radio.available(&pipe)) {                    // is there a payload? get the pipe number that recieved it
            uint8_t bytes = radio.getDynamicPayloadSize(); // get the size of the payload
            PayloadStruct received;
            radio.read(&received, sizeof(received));       // get incoming payload

            // print the details of transaction
            printf("Received %d bytes on pipe %d: %s%d Sent: %s%d\n",
                   bytes,
                   pipe,
                   received.message,
                   received.counter,
                   payload.message,
                   payload.counter);

            // save incoming counter & increment for next outgoing
            payload.counter = received.counter + 1;
            // load the payload for the first received transmission on pipe 0
            radio.writeAckPayload(1, &payload, sizeof(payload));
        }
    } // role

    char input = getchar_timeout_us(0); // get char from buffer for user input
    if (input != PICO_ERROR_TIMEOUT) {
        // change the role via the serial terminal

        if ((input == 'T' || input == 't') && !role) {
            // Become the TX node

            role = true;
            printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n");

            memcpy(payload.message, "Hello ", 6); // change payload message
            radio.stopListening();                // this also discards any unused ACK payloads

        }
        else if ((input == 'R' || input == 'r') && role) {
            // Become the RX node

            role = false;
            printf("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n");
            memcpy(payload.message, "World ", 6); // change payload message

            // load the payload for the first received transmission on pipe 0
            radio.writeAckPayload(1, &payload, sizeof(payload));
            radio.startListening();
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
