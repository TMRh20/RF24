/**
 * @file spi.h
 * Class declaration for SPI wrapping the Pico SDK
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define RF24_SPI_BYTE_SIZE 8
#define RF24_SPI_ENDIAN SPI_MSB_FIRST
#define RF24_SPI_CPHA SPI_CPHA_0
#define RF24_SPI_CPOL SPI_CPOL_0

// this SPI class uses beginTransaction() & endTransaction() to
// implement spi_init() & spi_deinit()
#define SPI_HAS_TRANSACTION 1

class SPI {
public:

    /**
     * SPI constructor
     */
    SPI();

    /**
     * Start SPI
     * @param hw_id This is either `spi0` or `spi1` (provided by the Pico SDK)
     *
     * @note this function assumes using the default SPI pins defined for your
     * board in "pico-sdk/src/boards/include/boards/*.h" files of the Pico SDK.
     * @see begin(spi_inst_t, uint8_t, uint8_t, uint8_t) for using other pins as
     * your SPI bus.
     */
    static void begin(spi_inst_t* hw_id);

    /**
     * Start SPI
     * @param hw_id This is either `spi0` or `spi1` (provided by the Pico SDK)
     * @param _sck The pin to be used as the SPI bus' sck
     * @param _tx The pin to be used as the SPI bus' tx (MOSI)
     * @param _rx The pin to be used as the SPI bus' rx (MISO)
     *
     * @note this function assumes using the default SPI pins defined for your
     * board in "pico-sdk/src/boards/include/boards/*.h" files of the Pico SDK.
     * @see The [Pico SDK has a chart of applicable pins](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf#%5B%7B%22num%22%3A106%2C%22gen%22%3A0%7D%2C%7B%22name%22%3A%22XYZ%22%7D%2C115%2C377.118%2Cnull%5D)
     * that can be used for hardware driven SPI transactions.
     */
    static void begin(spi_inst_t* hw_id, uint8_t _sck, uint8_t _tx, uint8_t _rx);

    /**
     * Transfer a single byte
     * @param tx_ Byte to send
     * @return Data returned via spi
     */
    static uint8_t transfer(uint8_t tx_);

    /**
     * Transfer a buffer of data
     * @param tbuf Transmit buffer
     * @param rbuf Receive buffer
     * @param len Length of the data
     */
    static void transfernb(const uint8_t* tbuf, uint8_t* rbuf, uint32_t len);

    /**
     * Transfer a buffer of data without an rx buffer
     * @param buf Pointer to a buffer of data
     * @param len Length of the data
     */
    static void transfern(const uint8_t* buf, uint32_t len);

    /**
     * init the SPI bus (using hw_id passed to begin())
     * @param _spi_speed The frequency to use for SPI transactions with the radio.
     */
    static void beginTransaction(uint32_t _spi_speed);

    /** deinit the SPI bus (using hw_id passed to begin()) */
    static void endTransaction();

    virtual ~ SPI();

private:

    /** the ID of the hardware driven SPI bus */
    static spi_inst_t* _hw_id;

};

/**@}*/
