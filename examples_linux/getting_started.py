"""
Simple example of using the RF24 class.
"""
import time
import struct
from RF24 import RF24, RF24_PA_LOW


########### USER CONFIGURATION ###########
# See https://github.com/TMRh20/RF24/blob/master/pyRF24/readme.md
# Radio CE Pin, CSN Pin, SPI Speed
# CE Pin uses GPIO number with BCM and SPIDEV drivers, other platforms use
# their own pin numbering
# CS Pin addresses the SPI bus number at /dev/spidev<a>.<b>
# ie: RF24 radio(<ce_pin>, <a>*10+<b>); spidev1.0 is 10, spidev1.1 is 11 etc..

# Generic:
radio = RF24(22, 0)
# RPi Alternate, with SPIDEV - Note: Edit RF24/arch/BBB/spi.cpp and
# set 'this->device = "/dev/spidev0.0";;' or as listed in /dev

# using the python keyword global is bad practice. Instead we'll use a 1 item
# list to store our float number for the payloads sent
payload = [0.0]

# For this example, we will use different addresses
# An address need to be a buffer protocol object (bytearray)
address = [b"1Node", b"2Node"]
# It is very helpful to think of an address as a path instead of as
# an identifying device destination

# to use different addresses on a pair of radios, we need a variable to
# uniquely identify which address this radio will use to transmit
# 0 uses address[0] to transmit, 1 uses address[1] to transmit
radio_number = bool(
    int(
        input(
            "Which radio is this? Enter '0' or '1'. Defaults to '0' "
        ) or 0
    )
)

# initialize the nRF24L01 on the spi bus
if not radio.begin():
    raise RuntimeError("radio is not responding")

# set the Power Amplifier level to -12 dBm since this test example is
# usually run with nRF24L01 transceivers in close proximity of each other
radio.setPALevel(RF24_PA_LOW)  # RF24_PA_MAX is default

# set the TX address of the RX node into the TX pipe
radio.openWritingPipe(address[radio_number])  # always uses pipe 0

# set the RX address of the TX node into a RX pipe
radio.openReadingPipe(1, address[not radio_number])  # using pipe 1

# To save time during transmission, we'll set the payload size to be only what
# we need. A float value occupies 4 bytes in memory using struct.pack()
# "<f" means a little endian unsigned float
radio.payloadSize = len(struct.pack("<f", payload[0]))

# for debugging, we have 2 options that print a large block of details
# radio.printDetails();  # (smaller) function that prints raw register values
# radio.printPrettyDetails();  # (larger) function that prints human readable data


def master(count=5):
    """Transmits an incrementing float every second

    :param int count: The number of payloads to transmmit (failed or
        successful)
    """
    radio.stopListening()  # ensures the nRF24L01 is in TX mode

    while count:
        # use struct.pack() to packet your data into the payload
        # "<f" means a single little endian (4 byte) float value.
        buffer = struct.pack("<f", payload[0])
        start_timer = time.monotonic_ns()  # start timer
        result = radio.write(buffer)
        end_timer = time.monotonic_ns()  # end timer
        if not result:
            print("Transmission failed or timed out")
        else:
            print(
                "Transmission successful! Time to Transmit: "
                "{} us. Sent: {}".format(
                    (end_timer - start_timer) / 1000,
                    payload[0]
                )
            )
            payload[0] += 0.01
        time.sleep(1)
        count -= 1


def slave(timeout=6):
    """Listen for any payloads and print the transaction

    :param int timeout: The number of seconds to wait (with no transmission)
        until exiting function.
    """
    radio.startListening()  # put radio into RX mode and power up

    start_timer = time.monotonic()
    while (time.monotonic() - start_timer) < timeout:
        has_payload, pipe_number = radio.available_pipe()
        if has_payload:
            # fetch 1 payload from RX FIFO
            buffer = radio.read(radio.payloadSize)
            # use struct.unpack() to convert the buffer into usable data
            # expecting a little endian float, thus the format string "<f"
            # buffer[:4] truncates padded 0s in case payloadSize was not set
            payload[0] = struct.unpack("<f", buffer[:4])[0]
            # print details about the received packet
            print(
                "Received {} bytes on pipe {}: {}".format(
                    radio.payloadSize,
                    pipe_number,
                    payload[0]
                )
            )
            start_timer = time.monotonic()  # reset the timeout timer

    # recommended behavior is to keep in TX mode while idle
    radio.stopListening()  # put the nRF24L01 is in TX mode


print(
    """\
    RF24/examples_linux/getting_started\n\
    Run slave() on receiver\n\
    Run master() on transmitter"""
)
