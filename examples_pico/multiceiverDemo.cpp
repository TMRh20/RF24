/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty 2bndy5
 */

/**
 * A simple example of sending data from as many as 6 nRF24L01 transceivers to
 * 1 receiving transceiver. This technique is trademarked by
 * Nordic Semiconductors as "MultiCeiver".
 *
 * This example was written to be used on up to 6 devices acting as TX nodes &
 * only 1 device acting as the RX node (that's a maximum of 7 devices).
 * Use the Serial Terminal to change each node's behavior.
 */
#include "pico/stdlib.h"  // printf(), sleep_ms(), getchar_timeout_us(), to_us_since_boot(), get_absolute_time()
#include "pico/bootrom.h" // reset_usb_boot()
#include <tusb.h>         // tud_cdc_connected()
#include <RF24.h>         // RF24 radio object
#include "defaultPins.h"  // board presumptive default pin numbers for CE_PIN and CSN_PIN

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// For this example, we'll be using 6 addresses; 1 for each TX node
// It is very helpful to think of an address as a path instead of as
// an identifying device destination
// Notice that the last byte is the only byte that changes in the last 5
// addresses. This is a limitation of the nRF24L01 transceiver for pipes 2-5
// because they use the same first 4 bytes from pipe 1.
uint64_t address[6] = {0x7878787878LL,
                       0xB3B4B5B6F1LL,
                       0xB3B4B5B6CDLL,
                       0xB3B4B5B6A3LL,
                       0xB3B4B5B60FLL,
                       0xB3B4B5B605LL
                      };

// Because this example allow up to 6 nodes (specified by numbers 0-5) to
// transmit and only 1 node to receive, we will use a negative value in our
// role variable to signify this node is a receiver.
// role variable is used to control whether this node is sending or receiving
char role = 'R'; // integers 0-5 = TX node; character 'R' or integer 82 = RX node

// For this example, we'll be using a payload containing
// a node ID number and a single integer number that will be incremented
// on every successful transmission.
// Make a data structure to use as a payload.
struct PayloadStruct
{
  unsigned long nodeID;
  unsigned long payloadID;
};
PayloadStruct payload;

// This example uses all 6 pipes to receive while TX nodes only use 2 pipes
// To make this easier we'll use a function to manage the addresses, and the
// payload's nodeID
void setRole(); // declare a prototype; definition is found after the loop()


bool setup()
{
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
    printf("RF24/examples_pico/multiceiverDemo\n");

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity of
    // each other.
    radio.setPALevel(RF24_PA_LOW); // RF24_PA_MAX is default.

    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need to transmit a float
    radio.setPayloadSize(sizeof(payload)); // 2x int datatype occupy 8 bytes

    // Set the pipe addresses accordingly. This function additionally also
    // calls startListening() or stopListening() and sets the payload's nodeID
    setRole();

    // For debugging info
    // printf_begin();             // needed only once for printing details
    // radio.printDetails();       // (smaller) function that prints raw register values
    // radio.printPrettyDetails(); // (larger) function that prints human readable data

    // role variable is hardcoded to RX behavior, inform the user of this
    printf("*** Enter a number between 0 and 5 (inclusive) to change\n");
    printf("    the identifying node number that transmits.\n");

    return true;
} // setup()


void loop()
{
    if (role <= 53) {
        // This device is a TX node

        uint64_t start_timer = to_us_since_boot(get_absolute_time()); // start the timer
        bool report = radio.write(&payload, sizeof(payload));         // transmit & save the report
        uint64_t end_timer = to_us_since_boot(get_absolute_time());   // end the timer

        if (report) {
            // payload was delivered
            printf("Transmission of payloadID %ld as node %ld successful! Time to transmit: %llu us\n",
                   payload.payloadID,
                   payload.nodeID,
                   end_timer - start_timer);
        }
        else {
            printf("Transmission failed or timed out\n"); // payload was not delivered
        }

        payload.payloadID++; // increment payload number

        // to make this example readable in the serial terminal
        sleep_ms(1000); // slow transmissions down by 1 second
    }
    else if (role == 'R') {
        // This device is the RX node

        uint8_t pipe;
        while (radio.available(&pipe)) {             // is there a payload? get the pipe number that recieved it
            uint8_t bytes = radio.getPayloadSize(); // get the size of the payload
            radio.read(&payload, bytes);            // fetch payload from FIFO

            // print details about incoming payload
            printf("Received %d bytes on pipe %d from node %ld. PayloadID: %ld\n",
                   bytes,
                   pipe,
                   payload.nodeID,
                   payload.payloadID);
        }
    } // role

    char input = getchar_timeout_us(0); // get char from buffer for user input
    if (input != PICO_ERROR_TIMEOUT) {
        // change the role via the serial terminal

        if ((input == 'R' || input == 'r') && role <= 53) {
            // Become the RX node

            role = 'R';
            printf("*** CHANGING ROLE TO RECEIVER ***\n");
            printf("--- Enter a number between 0 and 5 (inclusive) to act as\n");
            printf("    a unique node number that transmits to the RX node.\n");
            setRole(); // change address on all pipes to TX nodes

        }
        else if (input >= 48 && input <= 53 && input != role) {
            // Become a TX node with identifier 'input'

            role = input - 48;
            printf("*** CHANGING ROLE TO NODE %c ***\n", input);
            printf("--- Enter a number between 0 and 5 (inclusive) to change\n");
            printf("    the identifying node number that transmits.\n");
            printf("--- PRESS 'R' to act as the RX node.\n");
            setRole(); // change address on pipe 0 to the RX node
        }
        else if (input == 'b' || input == 'B') {
            // reset to bootloader
            radio.powerDown();
            reset_usb_boot(0, 0);
        }
    }

} // loop

void setRole()
{
    if (role == 'R') {
        // For the RX node

        // Set the addresses for all pipes to TX nodes
        for (uint8_t i = 0; i < 6; ++i)
            radio.openReadingPipe(i, address[i]);

        radio.startListening(); // put radio in RX mode

    }
    else {
        // For the TX node

        // set the payload's nodeID & reset the payload's identifying number
        payload.nodeID = role;
        payload.payloadID = 0;

        // Set the address on pipe 0 to the RX node.
        radio.stopListening(); // put radio in TX mode
        radio.openWritingPipe(address[role]);

        // According to the datasheet, the auto-retry features's delay value should
        // be "skewed" to allow the RX node to receive 1 transmission at a time.
        // So, use varying delay between retry attempts and 15 (at most) retry attempts
        radio.setRetries(((role * 3) % 12) + 3, 15); // maximum value is 15 for both args
    }
} // setRole

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
