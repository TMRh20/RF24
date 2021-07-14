"""
A simple example of sending data from as many as 6 nRF24L01 transceivers to
1 receiving transceiver. This technique is trademarked by
Nordic Semiconductors as "MultiCeiver".

This example was written to be used on up to 6 devices acting as TX nodes &
only 1 device acting as the RX node (that's a maximum of 7 devices).
"""
import sys
import argparse
import time
import struct
from RF24 import RF24, RF24_PA_LOW


parser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter
)
parser.add_argument(
    "-n",
    "--node",
    choices=("0", "1", "2", "3", "4", "5", "R", "r"),
    help="the identifying node ID number for the TX role. "
         "Use 'R' or 'r' to specify the RX role"
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

# setup the addresses for all transmitting radio nodes
addresses = [
    b"\x78" * 5,
    b"\xF1\xB6\xB5\xB4\xB3",
    b"\xCD\xB6\xB5\xB4\xB3",
    b"\xA3\xB6\xB5\xB4\xB3",
    b"\x0F\xB6\xB5\xB4\xB3",
    b"\x05\xB6\xB5\xB4\xB3"
]
# It is very helpful to think of an address as a path instead of as
# an identifying device destination


def master(node_number):
    """start transmitting to the base station.

    :param int node_number: the node's identifying index (from the
        the `addresses` list). This is a required parameter
    """
    # According to the datasheet, the auto-retry features's delay value should
    # be "skewed" to allow the RX node to receive 1 transmission at a time.
    # So, use varying delay between retry attempts and 15 (at most) retry attempts
    radio.setRetries(((node_number * 3) % 12) + 3, 15) # maximum value is 15 for both args

    radio.stopListening()  # put radio in TX mode
    # set the TX address to the address of the base station.
    radio.openWritingPipe(addresses[node_number])
    counter = 0
    failures = 0
    while failures < 6:
        counter += 1
        # payloads will include the node_number and a payload ID character
        payload = struct.pack("<ii", node_number, counter)
        start_timer = time.monotonic_ns()
        report = radio.write(payload)
        end_timer = time.monotonic_ns()
        # show something to see it isn't frozen
        print(
            "Transmission of payloadID {} as node {}".format(
                counter,
                node_number
            ),
            end=" "
        )
        if report:
            print(
                "successfull! Time to transmit = {} us".format(
                    (end_timer - start_timer) / 1000
                )
            )
        else:
            failures += 1
            print("failed or timed out")
        time.sleep(1)  # slow down the test for readability
    print(failures, "failures detected. Leaving TX role.")


def slave(timeout=10):
    """Use the radio as a base station for lisening to all nodes

    :param int timeout: The number of seconds to wait (with no transmission)
        until exiting function.
    """
    # write the addresses to all pipes.
    for pipe_n, addr in enumerate(addresses):
        radio.openReadingPipe(pipe_n, addr)
    radio.startListening()  # put radio in RX mode
    start_timer = time.monotonic()  # start timer
    while time.monotonic() - start_timer < timeout:
        has_payload, pipe_number = radio.available_pipe()
        if has_payload:
            # unpack payload
            nodeID, payloadID = struct.unpack(
                "<ii",
                radio.read(radio.payloadSize)
            )
            # show the pipe number that received the payload
            print(
                "Received {} bytes on pipe {} from node {}. PayloadID: "
                "{}".format(
                    radio.payloadSize,
                    pipe_number,
                    nodeID,
                    payloadID
                )
            )
            start_timer = time.monotonic()  # reset timer with every payload

    print("Nothing received in", timeout, "seconds. Leaving RX role")
    radio.stopListening()


def set_role():
    """Set the role using stdin stream. Timeout arg for slave() can be
    specified using a space delimiter (e.g. 'R 10' calls `slave(10)`)

    :return:
        - True when role is complete & app should continue running.
        - False when app should exit
    """
    user_input = input(
        "*** Enter 'R' for receiver role.\n"
        "*** Enter a number in range [0, 5] to use a specific node ID for "
        "transmitter role.\n"
        "*** Enter 'Q' to quit example.\n"
    ) or "?"
    user_input = user_input.split()
    if user_input[0].upper().startswith("R"):
        if len(user_input) > 1:
            slave(int(user_input[1]))
        else:
            slave()
        return True
    elif user_input[0].isdigit() and 0 <= int(user_input[0]) <= 5:
        master(int(user_input[0]))
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

    print(sys.argv[0])  # print example name

    # set the Power Amplifier level to -12 dBm since this test example is
    # usually run with nRF24L01 transceivers in close proximity of each other
    radio.setPALevel(RF24_PA_LOW)  # RF24_PA_MAX is default

    # To save time during transmission, we'll set the payload size to be only what
    # we need.
    # 2 int occupy 8 bytes in memory using len(struct.pack())
    # "<ii" means 2x little endian unsigned int
    radio.payloadSize = len(struct.pack("<ii", 0, 0))

    # for debugging, we have 2 options that print a large block of details
    # radio.printDetails();  # (smaller) function that prints raw register values
    # radio.printPrettyDetails();  # (larger) function that prints human readable data

    try:
        if args.node is None:  # if not specified with CLI arg '-n'
            while set_role():
                pass  # continue example until 'Q' is entered
        else:  # if role was set using CLI args
            # run role once and exit
            if args.node.isdigit():
                master(int(args.node))
            else:
                slave()
    except KeyboardInterrupt:
        print(" Keyboard Interrupt detected. Exiting...")
        radio.powerDown()
        sys.exit()
