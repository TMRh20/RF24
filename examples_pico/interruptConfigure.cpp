/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty (2bndy5)
 */

/**
 * This example uses Acknowledgement (ACK) payloads attached to ACK packets to
 * demonstrate how the nRF24L01's IRQ (Interrupt Request) pin can be
 * configured to detect when data is received, or when data has transmitted
 * successfully, or when data has failed to transmit.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Terminal to change each node's behavior.
 */
#include "pico/stdlib.h"  // printf(), sleep_ms(), getchar_timeout_us(), to_us_since_boot(), get_absolute_time()
#include "pico/bootrom.h" // reset_usb_boot()
#include <tusb.h>         // tud_cdc_connected()
#include <RF24.h>         // RF24 radio object
#include "defaultPins.h"  // board presumptive default pin numbers for IRQ_PIN, CE_PIN, and CSN_PIN

// We will be using the nRF24L01's IRQ pin for this example
volatile bool wait_for_event = false; // used to wait for an IRQ event to trigger

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// Used to control whether this node is sending or receiving
bool role = false;  // true = TX node, false = RX node

// For this example, we'll be using a payload containing
// a string that changes on every transmission. (successful or not)
// Make a couple arrays of payloads & an iterator to traverse them
const uint8_t tx_pl_size = 5;
const uint8_t ack_pl_size = 4;
uint8_t pl_iterator = 0;
// The " + 1" compensates for the c-string's NULL terminating 0
char tx_payloads[][tx_pl_size + 1] = {"Ping ", "Pong ", "Radio", "1FAIL"};
char ack_payloads[][ack_pl_size + 1] = {"Yak ", "Back", " ACK"};

void interruptHandler(uint gpio, uint32_t events); // prototype to handle IRQ events
void printRxFifo();                                // prototype to print RX FIFO with 1 buffer


bool setup()
{
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
    printf("RF24/examples_pico/interruptConfigure\n");

    // To set the radioNumber via the Serial terminal on startup
    printf("Which radio is this? Enter '0' or '1'. Defaults to '0'\n");
    char input = getchar();
    radioNumber = input == 49;
    printf("radioNumber = %d\n", (int)radioNumber);

    // setup the IRQ_PIN
    gpio_set_irq_enabled_with_callback(IRQ_PIN, GPIO_IRQ_EDGE_FALL, true, &interruptHandler);
    // IMPORTANT: do not call radio.available() before calling
    // radio.whatHappened() when the interruptHandler() is triggered by the
    // IRQ pin FALLING event. According to the datasheet, the pipe information
    // is unreliable during the IRQ pin FALLING transition.

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW);    // RF24_PA_MAX is default.

    // For this example we use acknowledgment (ACK) payloads to trigger the
    // IRQ pin when data is received on the TX node.
    // to use ACK payloads, we need to enable dynamic payload lengths
    radio.enableDynamicPayloads();    // ACK payloads are dynamically sized

    // Acknowledgement packets have no payloads by default. We need to enable
    // this feature for all nodes (TX & RX) to use ACK payloads.
    radio.enableAckPayload();
    // Fot this example, we use the same address to send data back and forth

    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

    // additional setup specific to the node's role
    if (role) {
        // setup for TX mode
        radio.stopListening(); // put radio in TX mode

    }
    else {
        // setup for RX mode

        // let IRQ pin not trigger in RX mode
        radio.maskIRQ(1, 1, 1); // args = "data_sent", "data_fail", "data_ready"

        // Fill the TX FIFO with 3 ACK payloads for the first 3 received
        // transmissions on pipe 1
        radio.writeAckPayload(1, &ack_payloads[0], ack_pl_size);
        radio.writeAckPayload(1, &ack_payloads[1], ack_pl_size);
        radio.writeAckPayload(1, &ack_payloads[2], ack_pl_size);

        radio.startListening(); // put radio in RX mode
    }

    // For debugging info
    // printf_begin();             // needed only once for printing details
    // radio.printDetails();       // (smaller) function that prints raw register values
    // radio.printPrettyDetails(); // (larger) function that prints human readable data

    // role variable is hardcoded to RX behavior, inform the user of this
    printf("*** PRESS 'T' to begin transmitting to the other node\n");

    return true;
}

void loop()
{
    if (role && !wait_for_event) {
        // This device is a TX node. This if block is only triggered when
        // NOT waiting for an IRQ event to happen

        if (pl_iterator == 0) {
            // Test the "data ready" event with the IRQ pin

            printf("\nConfiguring IRQ pin to ignore the 'data sent' event\n");
            radio.maskIRQ(true, false, false); // args = "data_sent", "data_fail", "data_ready"
            printf("   Pinging RX node for 'data ready' event...\n");

        }
        else if (pl_iterator == 1) {
            // Test the "data sent" event with the IRQ pin

            printf("\nConfiguring IRQ pin to ignore the 'data ready' event\n");
            radio.maskIRQ(false, false, true); // args = "data_sent", "data_fail", "data_ready"
            printf("   Pinging RX node for 'data sent' event...\n");

        }
        else if (pl_iterator == 2) {
            // Use this iteration to fill the RX node's FIFO which sets us up for the next test.

            // write() uses virtual interrupt flags that work despite the masking of the IRQ pin
            radio.maskIRQ(1, 1, 1); // disable IRQ masking for this step

            printf("\nSending 1 payload to fill RX node's FIFO. IRQ pin is neglected.\n");
            // write() will call flush_tx() on 'data fail' events
            if (radio.write(&tx_payloads[pl_iterator], tx_pl_size)) {
                if (radio.rxFifoFull()) {
                    printf("RX node's FIFO is full; it is not listening any more\n");
                }
                else {
                    printf("Transmission successful, but the RX node might still be listening.\n");
                }
            }
            else {
                printf("Transmission failed or timed out. Continuing anyway.\n");
                radio.flush_tx(); // discard payload(s) that failed to transmit
            }

        }
        else if (pl_iterator == 3) {
            // test the "data fail" event with the IRQ pin

            printf("\nConfiguring IRQ pin to reflect all events\n");
            radio.maskIRQ(0, 0, 0); // args = "data_sent", "data_fail", "data_ready"
            printf("   Pinging inactive RX node for 'data fail' event...\n");
        }

        if (pl_iterator < 4 && pl_iterator != 2) {

            // IRQ pin is LOW when activated. Otherwise it is always HIGH
            // Wait until IRQ pin is activated.
            wait_for_event = true;

            // use the non-blocking call to write a payload and begin transmission
            // the "false" argument means we are expecting an ACK packet response
            radio.startFastWrite(tx_payloads[pl_iterator++], tx_pl_size, false);

            // In this example, the "data fail" event is always configured to
            // trigger the IRQ pin active. Because the auto-ACK feature is on by
            // default, we don't need a timeout check to prevent an infinite loop.
        }
        else if (pl_iterator == 4) {
            // all IRQ tests are done; flush_tx() and print the ACK payloads for fun

            // CE pin is still HIGH which consumes more power. Example is now idling so...
            radio.stopListening(); // ensure CE pin is LOW
            // stopListening() also calls flush_tx() when ACK payloads are enabled

            printRxFifo();
            pl_iterator++;

            // inform user what to do next
            printf("\n*** PRESS 'T' to restart the transmissions");
            printf("\n*** PRESS 'R' to change to Receive role\n");

        }
        else if (pl_iterator == 2) {
            pl_iterator++; // proceed from step 3 to last step (stop at step 4 for readability)
        }

    }
    else if (!role && radio.rxFifoFull()) {
        // This device is a RX node
        //
        // The RX role waits until RX FIFO is full then stops listening while
        // the FIFOs get reset and starts listening again.
        radio.stopListening(); // also calls flush_tx() when ACKs are enabled
        printRxFifo();         // also clears RX FIFO

        // Fill the TX FIFO with 3 ACK payloads for the first 3 received
        // transmissions on pipe 1.
        radio.writeAckPayload(1, &ack_payloads[0], ack_pl_size);
        radio.writeAckPayload(1, &ack_payloads[1], ack_pl_size);
        radio.writeAckPayload(1, &ack_payloads[2], ack_pl_size);

        sleep_ms(200);          // let radio TX role complete
        radio.startListening(); // We're ready to start over. Begin listening.

    } // role

    char input = getchar_timeout_us(0); // get char from buffer for user input
    if (input != PICO_ERROR_TIMEOUT) {
        // change the role via the serial terminal

        if (input == 'T' || input == 't') {
            // Become the TX node
            if (!role)
                printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n");
            else if (role && wait_for_event) // don't interrupt on ongoing test
                return;                      // exit now; start next loop()
            else
                printf("*** RESTARTING IRQ PIN TEST ***\n");

            role = true;
            wait_for_event = false;
            pl_iterator = 0;   // reset the iterator
            radio.flush_tx();  // discard any payloads in the TX FIFO

            // startListening() clears the IRQ masks also. This is required for
            // continued TX operations when a transmission fails.
            radio.stopListening(); // this also discards any unused ACK payloads
        }
        else if ((input == 'R' || input == 'r')/*  && role */) {
            // Become the RX node
            printf("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n");

            role = false;
            radio.maskIRQ(1, 1, 1); // the IRQ pin should not trigger in this example's RX rode

            // Fill the TX FIFO with 3 ACK payloads for the first 3 received
            // transmissions on pipe 1
            radio.flush_tx(); // make sure there is room for 3 new ACK payloads
            radio.writeAckPayload(1, &ack_payloads[0], ack_pl_size);
            radio.writeAckPayload(1, &ack_payloads[1], ack_pl_size);
            radio.writeAckPayload(1, &ack_payloads[2], ack_pl_size);
            radio.startListening();
        }
        else if (input == 'b' || input == 'B') {
            // reset to bootloader
            radio.powerDown();
            reset_usb_boot(0, 0);
        }
    } // user input
} // loop

/**
 * when the IRQ pin goes active LOW, call this fuction print out why
 */
void interruptHandler(uint gpio, uint32_t events)
{

    if (gpio != IRQ_PIN && !(events | GPIO_IRQ_EDGE_FALL)) {
        // the gpio pin and event does not match the configuration we specified
        return;
    }

    // print IRQ status and all masking flags' states
    printf("\tIRQ pin is actively LOW\n");   // show that this function was called
    bool tx_ds, tx_df, rx_dr;                // declare variables for IRQ masks
    radio.whatHappened(tx_ds, tx_df, rx_dr); // get values for IRQ masks
    // whatHappened() clears the IRQ masks also. This is required for
    // continued TX operations when a transmission fails.
    // clearing the IRQ masks resets the IRQ pin to its inactive state (HIGH)

    // print "data sent", "data fail", "data ready" mask states
    printf("\tdata_sent: %s, data_fail: %s, data_ready: %s\n",
           tx_ds ? "true" : "false",
           tx_df ? "true" : "false",
           rx_dr ? "true" : "false");

    if (tx_df)            // if TX payload failed
        radio.flush_tx(); // clear all payloads from the TX FIFO

    // print if test passed or failed. Unintentional fails mean the RX node was not listening.
    // pl_iterator has already been incremented by now
    if (pl_iterator <= 1) {
        printf("   'Data Ready' event test %s\n", rx_dr ? "passed" : "failed");
    }
    else if (pl_iterator == 2) {
        printf("   'Data Sent' event test %s\n", tx_ds ? "passed" : "failed");
    }
    else if (pl_iterator == 4) {
        printf("   'Data Fail' event test %s\n", tx_df ? "passed" : "failed");
    }
    wait_for_event = false; // ready to continue with loop() operations
} // interruptHandler


/**
 * Print the entire RX FIFO with one buffer. This will also flush the RX FIFO.
 * Remember that the payload sizes are declared as tx_pl_size and ack_pl_size.
 */
void printRxFifo()
{
    if (radio.available()) {                   // if there is data in the RX FIFO
        // to flush the data from the RX FIFO, we'll fetch it all using 1 buffer

        uint8_t pl_size = !role ? tx_pl_size : ack_pl_size;
        char rx_fifo[pl_size * 3 + 1];         // RX FIFO is full & we know ACK payloads' size
        if (radio.rxFifoFull()) {
            rx_fifo[pl_size * 3] = 0;          // add a NULL terminating char to use as a c-string
            radio.read(&rx_fifo, pl_size * 3); // this clears the RX FIFO (for this example)
        }
        else {
            uint8_t i = 0;
            while (radio.available()) {
                radio.read(&rx_fifo + (i * pl_size), pl_size);
                i++;
            }
            rx_fifo[i * pl_size] = 0;            // add a NULL terminating char to use as a c-string
        }
        printf("Complete RX FIFO: %s\n", rx_fifo); // print the entire RX FIFO with 1 buffer
    }
}

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
