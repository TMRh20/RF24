"""
Simple example of detecting (and verifying) the IRQ (interrupt) pin on the
nRF24L01
"""
import time
import RPi.GPIO as GPIO
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

# select your digital input pin that's connected to the IRQ pin on the nRF24L01
IRQ_PIN = 12

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
    raise RuntimeError("nRF24L01 hardware isn't responding")

# ACK payloads are dynamically sized.
radio.enableDynamicPayloads()  # to use ACK payloads

# this example uses the ACK payload to trigger the IRQ pin active for
# the "on data received" event
radio.enableAckPayload()  # enable ACK payloads

# set the Power Amplifier level to -12 dBm since this test example is
# usually run with nRF24L01 transceivers in close proximity of each other
radio.setPALevel(RF24_PA_LOW)  # RF24_PA_MAX is default

# set TX address of RX node into the TX pipe
radio.openWritingPipe(address[radio_number])  # always uses pipe 0

# set RX address of TX node into an RX pipe
radio.openReadingPipe(1, address[not radio_number])  # using pipe 1

# for debugging
radio.printDetails()

# For this example, we'll be using a payload containing
# a string that changes on every transmission. (successful or not)
# Make a couple tuples of payloads & an iterator to traverse them
pl_iterator = [0]  # use a 1-item list instead of python's global keyword
tx_payloads = (b"Ping ", b"Pong ", b"Radio", b"1FAIL")
ack_payloads = (b"Yak ", b"Back", b" ACK")


def interrupt_handler(channel):
    """This function is called when IRQ pin is detected active LOW"""
    print("IRQ pin", channel, "went active LOW.")
    tx_ds, tx_df, rx_dr = radio.whatHappened()   # get IRQ status flags
    if tx_df:
        radio.flush_tx()
    print("\ttx_ds: {}, tx_df: {}, rx_dr: {}".format(tx_ds, tx_df, rx_dr))
    if pl_iterator[0] == 0:
        print(
            "    'data ready' event test {}".format(
                "passed" if rx_dr else "failed"
            )
        )
    elif pl_iterator[0] == 1:
        print(
            "    'data sent' event test {}".format(
                "passed" if tx_ds else "failed"
            )
        )
    elif pl_iterator[0] == 3:
        print(
            "    'data fail' event test {}".format(
                "passed" if tx_df else "failed"
            )
        )


# setup IRQ GPIO pin
GPIO.setmode(GPIO.BCM)
GPIO.setup(IRQ_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(IRQ_PIN, GPIO.FALLING, callback=interrupt_handler)
# IMPORTANT: do not call radio.available(&pipe_number) before calling
# radio.whatHappened() when the interruptHandler() is triggered by the
# IRQ pin FALLING event. According to the datasheet, the pipe information
# is unreliable during the IRQ pin FALLING transition.


def _ping_n_wait(pl_iter):
    """private function to ping RX node and wait for IRQ pin to be handled

    :param int pl_iter: The index of the buffer in `tx_payloads` tuple to
        send. This number is also used to determine if event test was
        successful or not.
    """
    # set pl_iterator[0] so interrupt_handler() can determine if test was
    # successful or not
    pl_iterator[0] = pl_iter
    # the following False parameter means we're expecting an ACK packet
    radio.startFastWrite(tx_payloads[pl_iter], False)
    time.sleep(0.1) # wait 100 ms for interrupt_handler() to complete


def print_rx_fifo(pl_size):
    """fush RX FIFO by printing all available payloads with 1 buffer

    :param int pl_size: the expected size of each payload
    """
    if radio.rxFifoFull():
            # all 3 payloads received were 5 bytes each, and RX FIFO is full
            # so, fetching 15 bytes from the RX FIFO also flushes RX FIFO
            print(
                "Complete RX FIFO:",
                radio.read(pl_size * 3).decode("utf-8")
            )
    else:
        buffer = bytearray()
        while radio.available():
            buffer += radio.read(pl_size)
        if buffer:  # if any payloads were read from the RX FIFO
            print("Complete RX FIFO:", buffer.decode("utf-8"))


def master():
    """Transmits 3 times: successfully receive ACK payload first, successfully
    transmit on second, and intentionally fail transmit on the third"""
    radio.stopListening()  # ensures the nRF24L01 is in TX mode

    # on data ready test
    print("\nConfiguring IRQ pin to only ignore 'on data sent' event")
    radio.maskIRQ(True, False, False)  # args = tx_ds, tx_df, rx_dr
    print("    Pinging slave node for an ACK payload...", end=" ")
    _ping_n_wait(0)

    # on "data sent" test
    print("\nConfiguring IRQ pin to only ignore 'on data ready' event")
    radio.maskIRQ(False, False, True)  # args = tx_ds, tx_df, rx_dr
    print("    Pinging slave node again...             ", end=" ")
    _ping_n_wait(1)

    # trigger slave node to stopListening() by filling slave node's RX FIFO
    print("\nSending one extra payload to fill RX FIFO on slave node.")
    radio.maskIRQ(1, 1, 1)  # disable IRQ pin for this step
    if radio.write(tx_payloads[2]):
        # when send_only parameter is True, send() ignores RX FIFO usage
        if radio.rxFifoFull():
            print("RX node's FIFO is full; it is not listening any more")
        else:
            print(
                "Transmission successful, but the RX node might still be "
                "listening."
            )
    else:
        radio.flush_tx()
        print("Transmission failed or timed out. Continuing anyway.")

    # on "data fail" test
    print("\nConfiguring IRQ pin to go active for all events.")
    radio.maskIRQ(False, False, False)  # args = tx_ds, tx_df, rx_dr
    print("    Sending a ping to inactive slave node...", end=" ")
    _ping_n_wait(3)

    print_rx_fifo(len(ack_payloads[0]))  # empty RX FIFO


def slave(timeout=6):  # will listen for 6 seconds before timing out
    """Only listen for 3 payload from the master node"""
    pl_iterator[0] = 0  # reset this to indicate event is a 'data_ready' event
    # setup radio to recieve pings, fill TX FIFO with ACK payloads
    radio.writeAckPayload(1, ack_payloads[0])
    radio.writeAckPayload(1, ack_payloads[1])
    radio.writeAckPayload(1, ack_payloads[2])
    radio.startListening()  # start listening & clear status flags
    start_timer = time.monotonic()  # start timer now
    while not radio.rxFifoFull() and time.monotonic() - start_timer < timeout:
        # if RX FIFO is not full and timeout is not reached, then keep waiting
        pass
    time.sleep(0.1)  # wait for last ACK payload to transmit
    radio.stopListening()  # put radio in TX mode & discard any ACK payloads
    print_rx_fifo(len(tx_payloads[0]))

print(
    """\
    nRF24L01 Interrupt pin test.\n\
    Make sure the IRQ pin is connected to the RPi GPIO12\n\
    Run slave() on receiver\n\
    Run master() on transmitter"""
)
