"""
A simple example of streaming data from 1 nRF24L01 transceiver to another.

This example was written to be used on 2 devices acting as 'nodes'.
"""
import sys
import argparse
import time
from RF24 import RF24, RF24_PA_LOW


parser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter
)
parser.add_argument(
    "-n",
    "--node",
    type=int,
    choices=range(2),
    help="the identifying radio number (or node ID number)"
)
parser.add_argument(
    "-r",
    "--role",
    type=int,
    choices=range(2),
    help="'1' specifies the TX role. '0' specifies the RX role."
)

########### USER CONFIGURATION ###########
# See https://github.com/TMRh20/RF24/blob/master/pyRF24/readme.md
# Radio CE Pin, CSN Pin, SPI Speed
# CE Pin uses GPIO number with BCM and SPIDEV drivers, other platforms use
# their own pin numbering
# CS Pin addresses the SPI bus number at /dev/spidev<a>.<b>
# ie: RF24 radio(<ce_pin>, <a>*10+<b>); spidev1.0 is 10, spidev1.1 is 11 etc..

# Generic:
radio = RF24(22, 0)
################## Linux (BBB,x86,etc) #########################
# See http://nRF24.github.io/RF24/pages.html for more information on usage
# See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
# See https://www.kernel.org/doc/Documentation/spi/spidev for more
# information on SPIDEV

# Specify the number of bytes in the payload. This is also used to
# specify the number of payloads in 1 stream of data
SIZE = 32  # this is the default maximum payload size


def make_buffer(buf_iter):
    """Returns a dynamically created payloads

    :param int buf_iter: The position of the payload in the data stream
    """
    # we'll use `SIZE` for the number of payloads in the list and the
    # payloads' length
    # prefix payload with a sequential letter to indicate which
    # payloads were lost (if any)
    buff = bytes([buf_iter + (65 if 0 <= buf_iter < 26 else 71)])
    for j in range(SIZE - 1):
        char = bool(j >= (SIZE - 1) / 2 + abs((SIZE - 1) / 2 - buf_iter))
        char |= bool(j < (SIZE - 1) / 2 - abs((SIZE - 1) / 2 - buf_iter))
        buff += bytes([char + 48])
    return buff


def master(count=1):
    """Uses all 3 levels of the TX FIFO to send a stream of data

    :param int count: how many times to transmit the stream of data.
    """
    radio.stopListening()  # put radio in TX mode
    radio.flush_tx()  # clear the TX FIFO so we can use all 3 levels
    failures = 0  # keep track of manual retries
    start_timer = time.monotonic_ns()  # start timer
    for multiplier in range(count):  # repeat transmit the same data stream
        buf_iter = 0  # iterator of payloads for the while loop
        while buf_iter < SIZE:  # cycle through all the payloads
            buffer = make_buffer(buf_iter)  # make a payload

            if not radio.writeFast(buffer):  # transmission failed
                failures += 1  # increment manual retry count
                if failures > 99 and buf_iter < 7 and multiplier < 2:
                    # we need to prevent an infinite loop
                    print(
                        "Too many failures detected. Aborting at payload ",
                        buffer[0]
                    )
                    multiplier = count  # be sure to exit the for loop
                    break  # exit the while loop
                radio.reUseTX()  # resend payload in top level of TX FIFO
            else:  # transmission succeeded
                buf_iter += 1
    end_timer = time.monotonic_ns()  # end timer
    print(
        "Time to transmit data = {} us. Detected {} failures.".format(
            (end_timer - start_timer) / 1000,
            failures
        )
    )


def slave(timeout=6):
    """Listen for any payloads and print them out (suffixed with received
    counter)

    :param int timeout: The number of seconds to wait (with no transmission)
        until exiting function.
    """
    radio.startListening()  # put radio in RX mode
    count = 0  # keep track of the number of received payloads
    start_timer = time.monotonic()  # start timer
    while (time.monotonic() - start_timer) < timeout:
        if radio.available():
            count += 1
            # retreive the received packet's payload
            receive_payload = radio.read(radio.payloadSize)
            print("Received: {} - {}".format(receive_payload, count))
            start_timer = time.monotonic()  # reset timer on every RX payload

    print("Nothing received in", timeout, "seconds. Leaving RX role")
    # recommended behavior is to keep in TX mode while idle
    radio.stopListening()  # put the radio in TX mode

    print("Nothing received in ", timeout, " seconds. Leaving RX role")

def set_role():
    """Set the role using stdin stream. Role args can be specified using space
    delimiters (e.g. 'R 10' calls `slave(10)` & 'T 3' calls `master(3)`)

    :return:
        - True when role is complete & app should continue running.
        - False when app should exit
    """
    user_input = input(
        "*** Enter 'R' for receiver role.\n"
        "*** Enter 'T' for transmitter role.\n"
        "*** Enter 'Q' to quit example.\n"
    ) or "?"
    user_input = user_input.split()
    if user_input[0].upper().startswith("R"):
        if len(user_input) > 1:
            slave(int(user_input[1]))
        else:
            slave()
        return True
    elif user_input[0].upper().startswith("T"):
        if len(user_input) > 1:
            master(int(user_input[1]))
        else:
            master()
        return True
    elif user_input[0].upper().startswith("Q"):
        radio.powerDown()
        return False
    print(user_input[0], "is an unrecognized input. Please try again.")
    return set_role()


if __name__ == "__main__":

    args = parser.parse_args()  # parse any CLI args

    # initialize the nRF24L01 on the spi bus
    if not radio.begin():
        raise RuntimeError("radio hardware is not responding")

    # For this example, we will use different addresses
    # An address need to be a buffer protocol object (bytearray)
    address = [b"1Node", b"2Node"]
    # It is very helpful to think of an address as a path instead of as
    # an identifying device destination

    print(sys.argv[0])  # print example name

    # to use different addresses on a pair of radios, we need a variable to
    # uniquely identify which address this radio will use to transmit
    # 0 uses address[0] to transmit, 1 uses address[1] to transmit
    radio_number = args.node  # uses default value from `parser`
    if args.node is None:  # if '--node' arg wasn't specified
        radio_number = bool(
            int(
                input(
                    "Which radio is this? Enter '0' or '1'. Defaults to '0' "
                ) or 0
            )
        )

    # set the Power Amplifier level to -12 dBm since this test example is
    # usually run with nRF24L01 transceivers in close proximity of each other
    radio.setPALevel(RF24_PA_LOW)  # RF24_PA_MAX is default

    # set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radio_number])  # always uses pipe 0

    # set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[not radio_number])  # using pipe 1

    # To save time during transmission, we'll set the payload size to be only
    # what we need. For this example, we'll be using the default maximum 32
    radio.payloadSize = SIZE

    # for debugging, we have 2 options that print a large block of details
    # (smaller) function that prints raw register values
    # radio.printDetails()
    # (larger) function that prints human readable data
    # radio.printPrettyDetails()

    try:
        if args.role is None:  # if not specified with CLI arg '-r'
            while set_role():
                pass  # continue example until 'Q' is entered
        else:  # if role was set using CLI args
            # run role once and exit
            master() if bool(args.role) else slave()
    except KeyboardInterrupt:
        print(" Keyboard Interrupt detected. Exiting...")
        radio.powerDown()
        sys.exit()
