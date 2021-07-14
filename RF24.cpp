/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "nRF24L01.h"
#include "RF24_config.h"
#include "RF24.h"


/****************************************************************************/

void RF24::csn(bool mode)
{
    #if defined(RF24_TINY)
    if (ce_pin != csn_pin) {
        digitalWrite(csn_pin, mode);
    }
    else {
        if (mode == HIGH) {
            PORTB |= (1<<PINB2);  	                       // SCK->CSN HIGH
            delayMicroseconds(RF24_CSN_SETTLE_HIGH_DELAY); // allow csn to settle.
        }
        else {
            PORTB &= ~(1<<PINB2);	                      // SCK->CSN LOW
            delayMicroseconds(RF24_CSN_SETTLE_LOW_DELAY); // allow csn to settle
        }
    }
    // Return, CSN toggle complete
    return;

    #elif defined (ARDUINO) && !defined(RF24_SPI_TRANSACTIONS)
    // Minimum ideal SPI bus speed is 2x data rate
    // If we assume 2Mbs data rate and 16Mhz clock, a
    // divider of 4 is the minimum we want.
    // CLK:BUS 8Mhz:2Mhz, 16Mhz:4Mhz, or 20Mhz:5Mhz

        #if !defined(SOFTSPI)
        // applies to SPI_UART and inherent hardware SPI
            #if defined (RF24_SPI_PTR)
    _spi->setBitOrder(MSBFIRST);
    _spi->setDataMode(SPI_MODE0);

                #if !defined(F_CPU) || F_CPU < 20000000
    _spi->setClockDivider(SPI_CLOCK_DIV2);
                #elif F_CPU < 40000000
    _spi->setClockDivider(SPI_CLOCK_DIV4);
                #elif F_CPU < 80000000
    _spi->setClockDivider(SPI_CLOCK_DIV8);
                #elif F_CPU < 160000000
    _spi->setClockDivider(SPI_CLOCK_DIV16);
                #elif F_CPU < 320000000
    _spi->setClockDivider(SPI_CLOCK_DIV32);
                #elif F_CPU < 640000000
    _spi->setClockDivider(SPI_CLOCK_DIV64);
                #elif F_CPU < 1280000000
    _spi->setClockDivider(SPI_CLOCK_DIV128);
                #else // F_CPU >= 1280000000
                    #error "Unsupported CPU frequency. Please set correct SPI divider."
                #endif // F_CPU to SPI_CLOCK_DIV translation

            #else // !defined(RF24_SPI_PTR)
    _SPI.setBitOrder(MSBFIRST);
    _SPI.setDataMode(SPI_MODE0);

                #if !defined(F_CPU) || F_CPU < 20000000
    _SPI.setClockDivider(SPI_CLOCK_DIV2);
                #elif F_CPU < 40000000
    _SPI.setClockDivider(SPI_CLOCK_DIV4);
                #elif F_CPU < 80000000
    _SPI.setClockDivider(SPI_CLOCK_DIV8);
                #elif F_CPU < 160000000
    _SPI.setClockDivider(SPI_CLOCK_DIV16);
                #elif F_CPU < 320000000
    _SPI.setClockDivider(SPI_CLOCK_DIV32);
                #elif F_CPU < 640000000
    _SPI.setClockDivider(SPI_CLOCK_DIV64);
                #elif F_CPU < 1280000000
    _SPI.setClockDivider(SPI_CLOCK_DIV128);
                #else // F_CPU >= 1280000000
                    #error "Unsupported CPU frequency. Please set correct SPI divider."
                #endif // F_CPU to SPI_CLOCK_DIV translation
            #endif // !defined(RF24_SPI_PTR)
        #endif // !defined(SOFTSPI)

    #elif defined (RF24_RPi)
    if(!mode)
        _SPI.chipSelect(csn_pin);
    #endif // defined(RF24_RPi)

    #if !defined(RF24_LINUX)
    digitalWrite(csn_pin, mode);
    delayMicroseconds(csDelay);
    #endif // !defined(RF24_LINUX)
}

/****************************************************************************/

void RF24::ce(bool level)
{
    //Allow for 3-pin use on ATTiny
    if (ce_pin != csn_pin) {
        digitalWrite(ce_pin, level);
    }
}

/****************************************************************************/

inline void RF24::beginTransaction()
{
    #if defined (RF24_SPI_TRANSACTIONS)
        #if defined (RF24_SPI_PTR)
            #if defined (RF24_RP2)
    _spi->beginTransaction(spi_speed);
            #else // ! defined (RF24_RP2)
    _spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE0));
            #endif // ! defined (RF24_RP2)
        #else // !defined(RF24_SPI_PTR)
    _SPI.beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE0));
        #endif // !defined(RF24_SPI_PTR)
    #endif // defined (RF24_SPI_TRANSACTIONS)
    csn(LOW);
}

/****************************************************************************/

inline void RF24::endTransaction()
{
    csn(HIGH);
    #if defined (RF24_SPI_TRANSACTIONS)
        #if defined (RF24_SPI_PTR)
    _spi->endTransaction();
        #else // !defined(RF24_SPI_PTR)
    _SPI.endTransaction();
        #endif // !defined(RF24_SPI_PTR)
    #endif // defined (RF24_SPI_TRANSACTIONS)
}

/****************************************************************************/

void RF24::read_register(uint8_t reg, uint8_t* buf, uint8_t len)
{
    #if defined (RF24_LINUX) || defined (RF24_RP2)
    beginTransaction(); //configures the spi settings for RPi, locks mutex and setting csn low
    uint8_t * prx = spi_rxbuff;
    uint8_t * ptx = spi_txbuff;
    uint8_t size = len + 1; // Add register value to transmit buffer

    *ptx++ = (R_REGISTER | reg);

    while (len--){ *ptx++ = RF24_NOP; } // Dummy operation, just for reading

        #if defined (RF24_RP2)
    _spi->transfernb((const uint8_t*)spi_txbuff, spi_rxbuff, size);
        #else // !defined (RF24_RP2)
    _SPI.transfernb((char *)spi_txbuff, (char *)spi_rxbuff, size);
        #endif // !defined (RF24_RP2)

    status = *prx++; // status is 1st byte of receive buffer

    // decrement before to skip status byte
    while (--size) { *buf++ = *prx++; }

    endTransaction(); // unlocks mutex and setting csn high

    #else // !defined(RF24_LINUX) && !defined(RF24_RP2)

    beginTransaction();
        #if defined (RF24_SPI_PTR)
    status = _spi->transfer(R_REGISTER | reg);
    while (len--) { *buf++ = _spi->transfer(0xFF); }

        #else // !defined(RF24_SPI_PTR)
    status = _SPI.transfer(R_REGISTER | reg);
    while (len--) { *buf++ = _SPI.transfer(0xFF); }

        #endif // !defined(RF24_SPI_PTR)
    endTransaction();
    #endif // !defined(RF24_LINUX) && !defined(RF24_RP2)
}

/****************************************************************************/

uint8_t RF24::read_register(uint8_t reg)
{
    uint8_t result;

    #if defined (RF24_LINUX) || defined (RF24_RP2)
    beginTransaction();

    uint8_t * prx = spi_rxbuff;
    uint8_t * ptx = spi_txbuff;
    *ptx++ = (R_REGISTER | reg);
    *ptx++ = RF24_NOP ; // Dummy operation, just for reading

        #if defined (RF24_RP2)
    _spi->transfernb((const uint8_t*)spi_txbuff, spi_rxbuff, 2);
        #else // !defined(RF24_RP2)
    _SPI.transfernb((char *)spi_txbuff, (char *)spi_rxbuff, 2);
        #endif // !defined(RF24_RP2)

    status = *prx;     // status is 1st byte of receive buffer
    result = *++prx;   // result is 2nd byte of receive buffer

    endTransaction();
    #else // !defined(RF24_LINUX) && !defined(RF24_RP2)

    beginTransaction();
        #if defined (RF24_SPI_PTR)
    status = _spi->transfer(R_REGISTER | reg);
    result = _spi->transfer(0xff);

        #else // !defined(RF24_SPI_PTR)
    status = _SPI.transfer(R_REGISTER | reg);
    result = _SPI.transfer(0xff);

        #endif // !defined(RF24_SPI_PTR)
    endTransaction();
    #endif // !defined(RF24_LINUX) && !defined(RF24_RP2)

    return result;
}

/****************************************************************************/

void RF24::write_register(uint8_t reg, const uint8_t* buf, uint8_t len)
{
    #if defined (RF24_LINUX) || defined (RF24_RP2)
    beginTransaction();
    uint8_t * prx = spi_rxbuff;
    uint8_t * ptx = spi_txbuff;
    uint8_t size = len + 1; // Add register value to transmit buffer

    *ptx++ = (W_REGISTER | (REGISTER_MASK & reg));
    while (len--) { *ptx++ = *buf++; }

        #if defined (RF24_RP2)
    _spi->transfernb((const uint8_t*)spi_txbuff, spi_rxbuff, size);
        #else // !defined(RF24_RP2)
    _SPI.transfernb((char *)spi_txbuff, (char *)spi_rxbuff, size);
        #endif // !defined(RF24_RP2)

    status = *prx; // status is 1st byte of receive buffer
    endTransaction();
    #else // !defined(RF24_LINUX) && !defined(RF24_RP2)

    beginTransaction();
        #if defined (RF24_SPI_PTR)
    status = _spi->transfer(W_REGISTER | reg);
    while (len--) { _spi->transfer(*buf++); }

        #else // !defined(RF24_SPI_PTR)
    status = _SPI.transfer(W_REGISTER | reg);
    while (len--) { _SPI.transfer(*buf++); }

        #endif // !defined(RF24_SPI_PTR)
    endTransaction();
    #endif // !defined(RF24_LINUX) && !defined(RF24_RP2)
}

/****************************************************************************/

void RF24::write_register(uint8_t reg, uint8_t value, bool is_cmd_only)
{
    if (is_cmd_only) {
        if (reg != RF24_NOP) { // don't print the get_status() operation
            IF_SERIAL_DEBUG(printf_P(PSTR("write_register(%02x)\r\n"), reg));
        }
        beginTransaction();
        #if defined (RF24_LINUX)
        status = _SPI.transfer(W_REGISTER | reg);
        #else // !defined(RF24_LINUX) || defined (RF24_RP2)
            #if defined (RF24_SPI_PTR)
        status = _spi->transfer(W_REGISTER | reg);
            #else // !defined (RF24_SPI_PTR)
        status = _SPI.transfer(W_REGISTER | reg);
            #endif // !defined (RF24_SPI_PTR)
        #endif // !defined(RF24_LINUX) || defined(RF24_RP2)
        endTransaction();
    }
    else {
        IF_SERIAL_DEBUG(printf_P(PSTR("write_register(%02x,%02x)\r\n"), reg, value));
        #if defined (RF24_LINUX) || defined (RF24_RP2)
        beginTransaction();
        uint8_t * prx = spi_rxbuff;
        uint8_t * ptx = spi_txbuff;
        *ptx++ = (W_REGISTER | reg);
        *ptx = value;

            #if defined (RF24_RP2)
        _spi->transfernb((const uint8_t*)spi_txbuff, spi_rxbuff, 2);
            #else // !defined(RF24_RP2)
        _SPI.transfernb((char *)spi_txbuff, (char *)spi_rxbuff, 2);
            #endif // !defined(RF24_RP2)

        status = *prx++; // status is 1st byte of receive buffer
        endTransaction();
        #else // !defined(RF24_LINUX) && !defined(RF24_RP2)

        beginTransaction();
            #if defined (RF24_SPI_PTR)
        status = _spi->transfer(W_REGISTER | reg);
        _spi->transfer(value);
            #else // !defined(RF24_SPI_PTR)
        status = _SPI.transfer(W_REGISTER | reg);
        _SPI.transfer(value);
            #endif // !defined(RF24_SPI_PTR)
        endTransaction();
        #endif // !defined(RF24_LINUX) && !defined(RF24_RP2)
    }
}

/****************************************************************************/

void RF24::write_payload(const void* buf, uint8_t data_len, const uint8_t writeType)
{
    const uint8_t* current = reinterpret_cast<const uint8_t*>(buf);

    uint8_t blank_len = !data_len ? 1 : 0;
    if (!dynamic_payloads_enabled) {
        data_len = rf24_min(data_len, payload_size);
        blank_len = payload_size - data_len;
    }
    else {
        data_len = rf24_min(data_len, 32);
    }

    //printf("[Writing %u bytes %u blanks]",data_len,blank_len);
    IF_SERIAL_DEBUG(printf("[Writing %u bytes %u blanks]\n", data_len, blank_len); );

    #if defined (RF24_LINUX) || defined (RF24_RP2)
    beginTransaction();
    uint8_t * prx = spi_rxbuff;
    uint8_t * ptx = spi_txbuff;
    uint8_t size;
    size = data_len + blank_len + 1 ; // Add register value to transmit buffer

    *ptx++ =  writeType;
    while (data_len--) { *ptx++ =  *current++; }
    while (blank_len--) { *ptx++ =  0; }

        #if defined (RF24_RP2)
    _spi->transfernb((const uint8_t*)spi_txbuff, spi_rxbuff, size);
        #else // !defined(RF24_RP2)
    _SPI.transfernb((char *)spi_txbuff, (char *)spi_rxbuff, size);
        #endif // !defined(RF24_RP2)

    status = *prx; // status is 1st byte of receive buffer
    endTransaction();

    #else // !defined(RF24_LINUX) && !defined(RF24_RP2)

    beginTransaction();
        #if defined (RF24_SPI_PTR)
    status = _spi->transfer(writeType);
    while (data_len--) { _spi->transfer(*current++); }
    while (blank_len--) { _spi->transfer(0); }

        #else // !defined(RF24_SPI_PTR)
    status = _SPI.transfer(writeType);
    while (data_len--) { _SPI.transfer(*current++); }

    while (blank_len--) { _SPI.transfer(0); }

        #endif // !defined(RF24_SPI_PTR)
    endTransaction();
    #endif // !defined(RF24_LINUX) && !defined(RF24_RP2)
}

/****************************************************************************/

void RF24::read_payload(void* buf, uint8_t data_len)
{
    uint8_t* current = reinterpret_cast<uint8_t*>(buf);

    uint8_t blank_len = 0;
    if (!dynamic_payloads_enabled) {
        data_len = rf24_min(data_len, payload_size);
        blank_len = payload_size - data_len;
    }
    else {
        data_len = rf24_min(data_len, 32);
    }

    //printf("[Reading %u bytes %u blanks]",data_len,blank_len);

    IF_SERIAL_DEBUG(printf("[Reading %u bytes %u blanks]\n", data_len, blank_len); );

    #if defined (RF24_LINUX) || defined (RF24_RP2)
    beginTransaction();
    uint8_t * prx = spi_rxbuff;
    uint8_t * ptx = spi_txbuff;
    uint8_t size;
    size = data_len + blank_len + 1; // Add register value to transmit buffer

    *ptx++ =  R_RX_PAYLOAD;
    while(--size) { *ptx++ = RF24_NOP; }

    size = data_len + blank_len + 1; // Size has been lost during while, re affect

        #if defined (RF24_RP2)
    _spi->transfernb((const uint8_t*)spi_txbuff, spi_rxbuff, size);
        #else // !defined(RF24_RP2)
    _SPI.transfernb((char *)spi_txbuff, (char *)spi_rxbuff, size);
        #endif // !defined(RF24_RP2)

    status = *prx++; // 1st byte is status

    if (data_len > 0) {
        // Decrement before to skip 1st status byte
        while (--data_len) { *current++ = *prx++; }

        *current = *prx;
    }
    endTransaction();
    #else // !defined(RF24_LINUX) && !defined(RF24_RP2)

    beginTransaction();
        #if defined (RF24_SPI_PTR)
    status = _spi->transfer(R_RX_PAYLOAD);
    while (data_len--) { *current++ = _spi->transfer(0xFF); }

    while (blank_len--) { _spi->transfer(0xFF); }

        #else // !defined(RF24_SPI_PTR)
    status = _SPI.transfer(R_RX_PAYLOAD);
    while (data_len--) { *current++ = _SPI.transfer(0xFF); }
    while (blank_len--) { _SPI.transfer(0xff); }

        #endif // !defined(RF24_SPI_PTR)
    endTransaction();

    #endif // !defined(RF24_LINUX) && !defined(RF24_RP2)
}

/****************************************************************************/

uint8_t RF24::flush_rx(void)
{
    write_register(FLUSH_RX, RF24_NOP, true);
    return status;
}

/****************************************************************************/

uint8_t RF24::flush_tx(void)
{
    write_register(FLUSH_TX, RF24_NOP, true);
    return status;
}

/****************************************************************************/

uint8_t RF24::get_status(void)
{
    write_register(RF24_NOP, RF24_NOP, true);
    return status;
}

/****************************************************************************/
#if !defined(MINIMAL)

void RF24::print_status(uint8_t _status)
{
    printf_P(PSTR("STATUS\t\t= 0x%02x RX_DR=%x TX_DS=%x MAX_RT=%x RX_P_NO=%x TX_FULL=%x\r\n"), _status, (_status & _BV(RX_DR)) ? 1 : 0,
            (_status & _BV(TX_DS)) ? 1 : 0, (_status & _BV(MAX_RT)) ? 1 : 0, ((_status >> RX_P_NO) & 0x07), (_status & _BV(TX_FULL)) ? 1 : 0);
}

/****************************************************************************/


void RF24::print_observe_tx(uint8_t value)
{
    printf_P(PSTR("OBSERVE_TX=%02x: POLS_CNT=%x ARC_CNT=%x\r\n"), value, (value >> PLOS_CNT) & 0x0F, (value >> ARC_CNT) & 0x0F);
}

/****************************************************************************/

void RF24::print_byte_register(const char* name, uint8_t reg, uint8_t qty)
{
    //char extra_tab = strlen_P(name) < 8 ? '\t' : 0;
    //printf_P(PSTR(PRIPSTR"\t%c ="),name,extra_tab);
    #if defined(RF24_LINUX)
    printf("%s\t=", name);
    #else // !defined(RF24_LINUX)
    printf_P(PSTR(PRIPSTR"\t="),name);
    #endif // !defined(RF24_LINUX)
    while (qty--) {
        printf_P(PSTR(" 0x%02x"), read_register(reg++));
    }
    printf_P(PSTR("\r\n"));
}

/****************************************************************************/

void RF24::print_address_register(const char* name, uint8_t reg, uint8_t qty)
{

    #if defined(RF24_LINUX)
    printf("%s\t=", name);
    #else // !defined(RF24_LINUX)
    printf_P(PSTR(PRIPSTR"\t="), name);
    #endif // !defined(RF24_LINUX)
    while (qty--) {
        uint8_t buffer[addr_width];
        read_register(reg++ & REGISTER_MASK, buffer, sizeof(buffer));

        printf_P(PSTR(" 0x"));
        uint8_t* bufptr = buffer + sizeof(buffer);
        while (--bufptr >= buffer) {
            printf_P(PSTR("%02x"), *bufptr);
        }
    }

    printf_P(PSTR("\r\n"));
}

#endif // !defined(MINIMAL)

/****************************************************************************/

RF24::RF24(uint16_t _cepin, uint16_t _cspin, uint32_t _spi_speed)
        :ce_pin(_cepin), csn_pin(_cspin), spi_speed(_spi_speed), payload_size(32), dynamic_payloads_enabled(true), addr_width(5), _is_p_variant(false),
         csDelay(5)
{
    _init_obj();
}

/****************************************************************************/

RF24::RF24(uint32_t _spi_speed)
        :ce_pin(0xFFFF), csn_pin(0xFFFF), spi_speed(_spi_speed), payload_size(32), dynamic_payloads_enabled(true), addr_width(5), _is_p_variant(false),
         csDelay(5)
{
    _init_obj();
}

/****************************************************************************/

void RF24::_init_obj()
{
    // Use a pointer on the Arduino platform

    #if defined (RF24_SPI_PTR) && !defined (RF24_RP2)
    _spi = &SPI;
    #endif // defined (RF24_SPI_PTR)

    pipe0_reading_address[0] = 0;
    if(spi_speed <= 35000){ //Handle old BCM2835 speed constants, default to RF24_SPI_SPEED
        spi_speed = RF24_SPI_SPEED;
    }
}

/****************************************************************************/

void RF24::setChannel(uint8_t channel)
{
    const uint8_t max_channel = 125;
    write_register(RF_CH, rf24_min(channel, max_channel));
}

uint8_t RF24::getChannel()
{
    return read_register(RF_CH);
}

/****************************************************************************/

void RF24::setPayloadSize(uint8_t size)
{
    // payload size must be in range [1, 32]
    payload_size = rf24_max(1, rf24_min(32, size));

    // write static payload size setting for all pipes
    for (uint8_t i = 0; i < 6; ++i)
        write_register(RX_PW_P0 + i, payload_size);
}

/****************************************************************************/

uint8_t RF24::getPayloadSize(void)
{
    return payload_size;
}

/****************************************************************************/

#if !defined(MINIMAL)

static const PROGMEM char rf24_datarate_e_str_0[] = "= 1 MBPS";
static const PROGMEM char rf24_datarate_e_str_1[] = "= 2 MBPS";
static const PROGMEM char rf24_datarate_e_str_2[] = "= 250 KBPS";
static const PROGMEM char * const rf24_datarate_e_str_P[] = {
  rf24_datarate_e_str_0,
  rf24_datarate_e_str_1,
  rf24_datarate_e_str_2,
};
static const PROGMEM char rf24_model_e_str_0[] = "nRF24L01";
static const PROGMEM char rf24_model_e_str_1[] = "nRF24L01+";
static const PROGMEM char * const rf24_model_e_str_P[] = {
  rf24_model_e_str_0,
  rf24_model_e_str_1,
};
static const PROGMEM char rf24_crclength_e_str_0[] = "= Disabled";
static const PROGMEM char rf24_crclength_e_str_1[] = "= 8 bits";
static const PROGMEM char rf24_crclength_e_str_2[] = "= 16 bits" ;
static const PROGMEM char * const rf24_crclength_e_str_P[] = {
  rf24_crclength_e_str_0,
  rf24_crclength_e_str_1,
  rf24_crclength_e_str_2,
};
static const PROGMEM char rf24_pa_dbm_e_str_0[] = "= PA_MIN";
static const PROGMEM char rf24_pa_dbm_e_str_1[] = "= PA_LOW";
static const PROGMEM char rf24_pa_dbm_e_str_2[] = "= PA_HIGH";
static const PROGMEM char rf24_pa_dbm_e_str_3[] = "= PA_MAX";
static const PROGMEM char * const rf24_pa_dbm_e_str_P[] = {
  rf24_pa_dbm_e_str_0,
  rf24_pa_dbm_e_str_1,
  rf24_pa_dbm_e_str_2,
  rf24_pa_dbm_e_str_3,
};

    #if defined(RF24_LINUX)
static const char rf24_csn_e_str_0[] = "CE0 (PI Hardware Driven)";
static const char rf24_csn_e_str_1[] = "CE1 (PI Hardware Driven)";
static const char rf24_csn_e_str_2[] = "CE2 (PI Hardware Driven)";
static const char rf24_csn_e_str_3[] = "Custom GPIO Software Driven";
static const char * const rf24_csn_e_str_P[] = {
  rf24_csn_e_str_0,
  rf24_csn_e_str_1,
  rf24_csn_e_str_2,
  rf24_csn_e_str_3,
};
    #endif // defined(RF24_LINUX)

static const PROGMEM char rf24_feature_e_str_on[] = "= Enabled";
static const PROGMEM char rf24_feature_e_str_allowed[] = "= Allowed";
static const PROGMEM char rf24_feature_e_str_open[] = " open ";
static const PROGMEM char rf24_feature_e_str_closed[] = "closed";
static const PROGMEM char * const rf24_feature_e_str_P[] = {
    rf24_crclength_e_str_0,
    rf24_feature_e_str_on,
    rf24_feature_e_str_allowed,
    rf24_feature_e_str_closed,
    rf24_feature_e_str_open
};

void RF24::printDetails(void)
{

    #if defined(RF24_LINUX)
    printf("================ SPI Configuration ================\n" );
    uint8_t bus_ce = csn_pin % 10;
    uint8_t bus_numb = (csn_pin - bus_ce) / 10;
    printf("CSN Pin\t\t= /dev/spidev%d.%d\n", bus_numb, bus_ce);
    printf("CE Pin\t\t= Custom GPIO%d\n", ce_pin);
    #endif
    printf_P(PSTR("SPI Speedz\t= %d Mhz\n"),(uint8_t)(spi_speed/1000000)); //Print the SPI speed on non-Linux devices
    #if defined(RF24_LINUX)
    printf("================ NRF Configuration ================\n");
    #endif // defined(RF24_LINUX)

    print_status(get_status());

    print_address_register(PSTR("RX_ADDR_P0-1"), RX_ADDR_P0, 2);
    print_byte_register(PSTR("RX_ADDR_P2-5"), RX_ADDR_P2, 4);
    print_address_register(PSTR("TX_ADDR\t"), TX_ADDR);

    print_byte_register(PSTR("RX_PW_P0-6"), RX_PW_P0, 6);
    print_byte_register(PSTR("EN_AA\t"), EN_AA);
    print_byte_register(PSTR("EN_RXADDR"), EN_RXADDR);
    print_byte_register(PSTR("RF_CH\t"), RF_CH);
    print_byte_register(PSTR("RF_SETUP"), RF_SETUP);
    print_byte_register(PSTR("CONFIG\t"), NRF_CONFIG);
    print_byte_register(PSTR("DYNPD/FEATURE"), DYNPD, 2);

    printf_P(PSTR("Data Rate\t"
    PRIPSTR
    "\r\n"),(char*)pgm_read_ptr(&rf24_datarate_e_str_P[getDataRate()]));
    printf_P(PSTR("Model\t\t= "
    PRIPSTR
    "\r\n"),(char*)pgm_read_ptr(&rf24_model_e_str_P[isPVariant()]));
    printf_P(PSTR("CRC Length\t"
    PRIPSTR
    "\r\n"),(char*)pgm_read_ptr(&rf24_crclength_e_str_P[getCRCLength()]));
    printf_P(PSTR("PA Power\t"
    PRIPSTR
    "\r\n"),(char*)pgm_read_ptr(&rf24_pa_dbm_e_str_P[getPALevel()]));
    printf_P(PSTR("ARC\t\t= %d\r\n"), getARC());
}

void RF24::printPrettyDetails(void) {

    #if defined(RF24_LINUX)
    printf("================ SPI Configuration ================\n");
    uint8_t bus_ce = csn_pin % 10;
    uint8_t bus_numb = (csn_pin - bus_ce) / 10;
    printf("CSN Pin\t\t\t= /dev/spidev%d.%d\n", bus_numb, bus_ce);
    printf("CE Pin\t\t\t= Custom GPIO%d\n", ce_pin);
    #endif
    printf_P(PSTR("SPI Frequency\t\t= %d Mhz\n"), (uint8_t)(spi_speed / 1000000)); //Print the SPI speed on non-Linux devices
    #if defined(RF24_LINUX)
    printf("================ NRF Configuration ================\n");
    #endif // defined(RF24_LINUX)

    uint8_t channel = getChannel();
    uint16_t frequency = (uint16_t)channel + 2400;
    printf_P(PSTR("Channel\t\t\t= %u (~ %u MHz)\r\n"), channel, frequency);

    printf_P(PSTR("RF Data Rate\t\t"
    PRIPSTR
    "\r\n"), (char*)pgm_read_ptr(&rf24_datarate_e_str_P[getDataRate()]));
    printf_P(PSTR("RF Power Amplifier\t"
    PRIPSTR
    "\r\n"), (char*)pgm_read_ptr(&rf24_pa_dbm_e_str_P[getPALevel()]));
    printf_P(PSTR("RF Low Noise Amplifier\t"
    PRIPSTR
    "\r\n"), (char*)pgm_read_ptr(&rf24_feature_e_str_P[(bool)(read_register(RF_SETUP) & 1) * 1]));
    printf_P(PSTR("CRC Length\t\t"
    PRIPSTR
    "\r\n"), (char*)pgm_read_ptr(&rf24_crclength_e_str_P[getCRCLength()]));
    printf_P(PSTR("Address Length\t\t= %d bytes\r\n"), (read_register(SETUP_AW) & 3) + 2);
    printf_P(PSTR("Static Payload Length\t= %d bytes\r\n"), getPayloadSize());

    uint8_t setupRetry = read_register(SETUP_RETR);
    printf_P(PSTR("Auto Retry Delay\t= %d microseconds\r\n"), (uint16_t)(setupRetry >> ARD) * 250 + 250);
    printf_P(PSTR("Auto Retry Attempts\t= %d maximum\r\n"), setupRetry & 0x0F);

    uint8_t observeTx = read_register(OBSERVE_TX);
    printf_P(PSTR("Packets lost on\n    current channel\t= %d\r\n"), observeTx >> 4);
    printf_P(PSTR("Retry attempts made for\n    last transmission\t= %d\r\n"), observeTx & 0x0F);

    uint8_t features = read_register(FEATURE);
    printf_P(PSTR("Multicast\t\t"
    PRIPSTR
    "\r\n"), (char*)pgm_read_ptr(&rf24_feature_e_str_P[(bool)(features & _BV(EN_DYN_ACK)) * 2]));
    printf_P(PSTR("Custom ACK Payload\t"
    PRIPSTR
    "\r\n"), (char*)pgm_read_ptr(&rf24_feature_e_str_P[(bool)(features & _BV(EN_ACK_PAY)) * 1]));

    uint8_t dynPl = read_register(DYNPD);
    printf_P(PSTR("Dynamic Payloads\t"
    PRIPSTR
    "\r\n"), (char*)pgm_read_ptr(&rf24_feature_e_str_P[(dynPl && (features &_BV(EN_DPL))) * 1]));

    uint8_t autoAck = read_register(EN_AA);
    if (autoAck == 0x3F || autoAck == 0) {
        // all pipes have the same configuration about auto-ack feature
        printf_P(PSTR("Auto Acknowledgment\t"
        PRIPSTR
        "\r\n"), (char*)pgm_read_ptr(&rf24_feature_e_str_P[(bool)(autoAck) * 1]));
    } else {
        // representation per pipe
        printf_P(PSTR("Auto Acknowledgment\t= 0b%c%c%c%c%c%c\r\n"),
                 (char)((bool)(autoAck & _BV(ENAA_P5)) + 48),
                 (char)((bool)(autoAck & _BV(ENAA_P4)) + 48),
                 (char)((bool)(autoAck & _BV(ENAA_P3)) + 48),
                 (char)((bool)(autoAck & _BV(ENAA_P2)) + 48),
                 (char)((bool)(autoAck & _BV(ENAA_P1)) + 48),
                 (char)((bool)(autoAck & _BV(ENAA_P0)) + 48));
    }

    config_reg = read_register(NRF_CONFIG);
    printf_P(PSTR("Primary Mode\t\t= %cX\r\n"), config_reg & _BV(PRIM_RX) ? 'R' : 'T');
    print_address_register(PSTR("TX address\t"), TX_ADDR);

    uint8_t openPipes = read_register(EN_RXADDR);
    for (uint8_t i = 0; i < 6; ++i) {
        bool isOpen = openPipes & _BV(i);
        printf_P(PSTR("pipe %u ("
        PRIPSTR
        ") bound"), i, (char*)pgm_read_ptr(&rf24_feature_e_str_P[isOpen + 3]));
        if (i < 2) {
            print_address_register(PSTR(""), RX_ADDR_P0 + i);
        }
        else {
            print_byte_register(PSTR(""), RX_ADDR_P0 + i);
        }
    }
}
#endif // !defined(MINIMAL)

/****************************************************************************/
#if defined (RF24_SPI_PTR) || defined (DOXYGEN_FORCED)
// does not apply to RF24_LINUX

bool RF24::begin(_SPI* spiBus)
{
    _spi = spiBus;
    return _init_pins() && _init_radio();
}

/****************************************************************************/

bool RF24::begin(_SPI* spiBus, uint16_t _cepin, uint16_t _cspin)
{
    ce_pin = _cepin;
    csn_pin = _cspin;
    return begin(spiBus);
}

#endif // defined (RF24_SPI_PTR) || defined (DOXYGEN_FORCED)

/****************************************************************************/

bool RF24::begin(uint16_t _cepin, uint16_t _cspin)
{
    ce_pin = _cepin;
    csn_pin = _cspin;
    return begin();
}

/****************************************************************************/

bool RF24::begin(void)
{
    #if defined (RF24_LINUX)
        #if defined (RF24_RPi)
    switch(csn_pin) {                 // Ensure valid hardware CS pin
        case 0: break;
        case 1: break;
        // Allow BCM2835 enums for RPi
        case 8: csn_pin = 0; break;
        case 7: csn_pin = 1; break;
        case 18: csn_pin = 10; break; // to make it work on SPI1
        case 17: csn_pin = 11; break;
        case 16: csn_pin = 12; break;
        default: csn_pin = 0; break;
    }
        #endif // RF24_RPi

    _SPI.begin(csn_pin, spi_speed);

    #elif defined (XMEGA_D3)
    _spi->begin(csn_pin);

    #elif defined (RF24_RP2)
    _spi->begin(PICO_DEFAULT_SPI ? spi1 : spi0);

    #else // using an Arduino platform || defined (LITTLEWIRE)

        #if defined (RF24_SPI_PTR)
    _spi->begin();
        #else  // !defined(RF24_SPI_PTR)
    _SPI.begin();
        #endif // !defined(RF24_SPI_PTR)

    #endif // !defined(XMEGA_D3) && !defined(RF24_LINUX)

    return _init_pins() && _init_radio();
}

/****************************************************************************/

bool RF24::_init_pins()
{
    if (!isValid()) {
        // didn't specify the CSN & CE pins to c'tor nor begin()
        return false;
    }

    #if defined (RF24_LINUX)

        #if defined (MRAA)
    GPIO();
    gpio.begin(ce_pin, csn_pin);
        #endif

    pinMode(ce_pin, OUTPUT);
    ce(LOW);
    delay(100);

    #elif defined (LITTLEWIRE)
    pinMode(csn_pin, OUTPUT);
    csn(HIGH);

    #elif defined (XMEGA_D3)
    if (ce_pin != csn_pin) {
        pinMode(ce_pin, OUTPUT);
    };
    ce(LOW);
    csn(HIGH);
    delay(200);

    #else // using an Arduino platform

    // Initialize pins
    if (ce_pin != csn_pin) {
        pinMode(ce_pin, OUTPUT);
        pinMode(csn_pin, OUTPUT);
    }

    ce(LOW);
    csn(HIGH);

        #if defined (__ARDUINO_X86__)
    delay(100);
        #endif
    #endif // !defined(XMEGA_D3) && !defined(LITTLEWIRE) && !defined(RF24_LINUX)

    return true; // assuming pins are connected properly
}

/****************************************************************************/

bool RF24::_init_radio()
{
    // Must allow the radio time to settle else configuration bits will not necessarily stick.
    // This is actually only required following power up but some settling time also appears to
    // be required after resets too. For full coverage, we'll always assume the worst.
    // Enabling 16b CRC is by far the most obvious case if the wrong timing is used - or skipped.
    // Technically we require 4.5ms + 14us as a worst case. We'll just call it 5ms for good measure.
    // WARNING: Delay is based on P-variant whereby non-P *may* require different timing.
    delay(5);

    // Set 1500uS (minimum for 32B payload in ESB@250KBPS) timeouts, to make testing a little easier
    // WARNING: If this is ever lowered, either 250KBS mode with AA is broken or maximum packet
    // sizes must never be used. See datasheet for a more complete explanation.
    setRetries(5, 15);

    // Then set the data rate to the slowest (and most reliable) speed supported by all
    // hardware.
    setDataRate(RF24_1MBPS);

    // detect if is a plus variant & use old toggle features command accordingly
    uint8_t before_toggle = read_register(FEATURE);
    toggle_features();
    uint8_t after_toggle = read_register(FEATURE);
    _is_p_variant = before_toggle == after_toggle;
    if (after_toggle){
        if (_is_p_variant){
            // module did not experience power-on-reset (#401)
            toggle_features();
        }
        // allow use of multicast parameter and dynamic payloads by default
        write_register(FEATURE, 0);
    }
    ack_payloads_enabled = false;     // ack payloads disabled by default
    write_register(DYNPD, 0);         // disable dynamic payloads by default (for all pipes)
    dynamic_payloads_enabled = false;
    write_register(EN_AA, 0x3F);      // enable auto-ack on all pipes
    write_register(EN_RXADDR, 3);     // only open RX pipes 0 & 1
    setPayloadSize(32);               // set static payload size to 32 (max) bytes by default
    setAddressWidth(5);               // set default address length to (max) 5 bytes

    // Set up default configuration.  Callers can always change it later.
    // This channel should be universally safe and not bleed over into adjacent
    // spectrum.
    setChannel(76);

    // Reset current status
    // Notice reset and flush is the last thing we do
    write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));


    // Flush buffers
    flush_rx();
    flush_tx();

    // Clear CONFIG register:
    //      Reflect all IRQ events on IRQ pin
    //      Enable PTX
    //      Power Up
    //      16-bit CRC (CRC required by auto-ack)
    // Do not write CE high so radio will remain in standby I mode
    // PTX should use only 22uA of power
    write_register(NRF_CONFIG, (_BV(EN_CRC) | _BV(CRCO)) );
    config_reg = read_register(NRF_CONFIG);

    powerUp();

    // if config is not set correctly then there was a bad response from module
    return config_reg == (_BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP)) ? true : false;
}

/****************************************************************************/

bool RF24::isChipConnected()
{
    uint8_t setup = read_register(SETUP_AW);
    if (setup >= 1 && setup <= 3) {
        return true;
    }

    return false;
}

/****************************************************************************/

bool RF24::isValid()
{
    return ce_pin != 0xFFFF && csn_pin != 0xFFFF;
}

/****************************************************************************/

void RF24::startListening(void)
{
    #if !defined(RF24_TINY) && !defined(LITTLEWIRE)
    powerUp();
    #endif
    config_reg |= _BV(PRIM_RX);
    write_register(NRF_CONFIG, config_reg);
    write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));
    ce(HIGH);

    // Restore the pipe0 address, if exists
    if (pipe0_reading_address[0] > 0) {
        write_register(RX_ADDR_P0, pipe0_reading_address, addr_width);
    } else {
        closeReadingPipe(0);
    }
}

/****************************************************************************/
static const PROGMEM uint8_t child_pipe_enable[] = {ERX_P0, ERX_P1, ERX_P2,
                                                    ERX_P3, ERX_P4, ERX_P5};

void RF24::stopListening(void)
{
    ce(LOW);

    //delayMicroseconds(100);
    delayMicroseconds(txDelay);
    if (ack_payloads_enabled){
        flush_tx();
    }

    config_reg &= ~_BV(PRIM_RX);
    write_register(NRF_CONFIG, config_reg);

    #if defined(RF24_TINY) || defined(LITTLEWIRE)
    // for 3 pins solution TX mode is only left with additonal powerDown/powerUp cycle
    if (ce_pin == csn_pin) {
      powerDown();
      powerUp();
    }
    #endif
    write_register(EN_RXADDR, read_register(EN_RXADDR) | _BV(pgm_read_byte(&child_pipe_enable[0]))); // Enable RX on pipe0
}

/****************************************************************************/

void RF24::powerDown(void)
{
    ce(LOW); // Guarantee CE is low on powerDown
    config_reg &= ~_BV(PWR_UP);
    write_register(NRF_CONFIG, config_reg);
}

/****************************************************************************/

//Power up now. Radio will not power down unless instructed by MCU for config changes etc.
void RF24::powerUp(void)
{
    // if not powered up then power up and wait for the radio to initialize
    if (!(config_reg & _BV(PWR_UP))) {
        config_reg |= _BV(PWR_UP);
        write_register(NRF_CONFIG, config_reg);

        // For nRF24L01+ to go from power down mode to TX or RX mode it must first pass through stand-by mode.
        // There must be a delay of Tpd2stby (see Table 16.) after the nRF24L01+ leaves power down mode before
        // the CEis set high. - Tpd2stby can be up to 5ms per the 1.0 datasheet
        delayMicroseconds(RF24_POWERUP_DELAY);
    }
}

/******************************************************************/
#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)

void RF24::errNotify()
{
    #if defined (SERIAL_DEBUG) || defined (RF24_LINUX)
    printf_P(PSTR("RF24 HARDWARE FAIL: Radio not responding, verify pin connections, wiring, etc.\r\n"));
    #endif
    #if defined (FAILURE_HANDLING)
    failureDetected = 1;
    #else
    delay(5000);
    #endif
}

#endif
/******************************************************************/

//Similar to the previous write, clears the interrupt flags
bool RF24::write(const void* buf, uint8_t len, const bool multicast)
{
    //Start Writing
    startFastWrite(buf, len, multicast);

    //Wait until complete or failed
    #if defined(FAILURE_HANDLING) || defined(RF24_LINUX)
    uint32_t timer = millis();
    #endif // defined(FAILURE_HANDLING) || defined(RF24_LINUX)

    while (!(get_status() & (_BV(TX_DS) | _BV(MAX_RT)))) {
        #if defined(FAILURE_HANDLING) || defined(RF24_LINUX)
        if (millis() - timer > 95) {
            errNotify();
            #if defined(FAILURE_HANDLING)
            return 0;
            #else
            delay(100);
            #endif
        }
        #endif
    }

    ce(LOW);

    write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));

    //Max retries exceeded
    if (status & _BV(MAX_RT)) {
        flush_tx(); // Only going to be 1 packet in the FIFO at a time using this method, so just flush
        return 0;
    }
    //TX OK 1 or 0
    return 1;
}

bool RF24::write(const void* buf, uint8_t len)
{
    return write(buf, len, 0);
}
/****************************************************************************/

//For general use, the interrupt flags are not important to clear
bool RF24::writeBlocking(const void* buf, uint8_t len, uint32_t timeout)
{
    //Block until the FIFO is NOT full.
    //Keep track of the MAX retries and set auto-retry if seeing failures
    //This way the FIFO will fill up and allow blocking until packets go through
    //The radio will auto-clear everything in the FIFO as long as CE remains high

    uint32_t timer = millis();                               // Get the time that the payload transmission started

    while ((get_status() & (_BV(TX_FULL)))) {                // Blocking only if FIFO is full. This will loop and block until TX is successful or timeout

        if (status & _BV(MAX_RT)) {                          // If MAX Retries have been reached
            reUseTX();                                       // Set re-transmit and clear the MAX_RT interrupt flag
            if (millis() - timer > timeout) {
                return 0;                                    // If this payload has exceeded the user-defined timeout, exit and return 0
            }
        }
        #if defined(FAILURE_HANDLING) || defined(RF24_LINUX)
        if (millis() - timer > (timeout + 95)) {
            errNotify();
            #if defined(FAILURE_HANDLING)
            return 0;
            #endif
        }
        #endif

    }

    //Start Writing
    startFastWrite(buf, len, 0);                             // Write the payload if a buffer is clear

    return 1;                                                // Return 1 to indicate successful transmission
}

/****************************************************************************/

void RF24::reUseTX()
{
    write_register(NRF_STATUS, _BV(MAX_RT));  //Clear max retry flag
    write_register(REUSE_TX_PL, RF24_NOP, true);
    ce(LOW);                                  //Re-Transfer packet
    ce(HIGH);
}

/****************************************************************************/

bool RF24::writeFast(const void* buf, uint8_t len, const bool multicast)
{
    //Block until the FIFO is NOT full.
    //Keep track of the MAX retries and set auto-retry if seeing failures
    //Return 0 so the user can control the retrys and set a timer or failure counter if required
    //The radio will auto-clear everything in the FIFO as long as CE remains high

    #if defined(FAILURE_HANDLING) || defined(RF24_LINUX)
    uint32_t timer = millis();
    #endif

    //Blocking only if FIFO is full. This will loop and block until TX is successful or fail
    while ((get_status() & (_BV(TX_FULL)))) {
        if (status & _BV(MAX_RT)) {
            return 0;                                        //Return 0. The previous payload has not been retransmitted
            // From the user perspective, if you get a 0, just keep trying to send the same payload
        }
        #if defined(FAILURE_HANDLING) || defined(RF24_LINUX)
        if (millis() - timer > 95) {
            errNotify();
            #if defined(FAILURE_HANDLING)
            return 0;
            #endif // defined(FAILURE_HANDLING)
        }
        #endif
    }
    startFastWrite(buf, len, multicast);                     // Start Writing

    return 1;
}

bool RF24::writeFast(const void* buf, uint8_t len)
{
    return writeFast(buf, len, 0);
}

/****************************************************************************/

//Per the documentation, we want to set PTX Mode when not listening. Then all we do is write data and set CE high
//In this mode, if we can keep the FIFO buffers loaded, packets will transmit immediately (no 130us delay)
//Otherwise we enter Standby-II mode, which is still faster than standby mode
//Also, we remove the need to keep writing the config register over and over and delaying for 150 us each time if sending a stream of data

void RF24::startFastWrite(const void* buf, uint8_t len, const bool multicast, bool startTx)
{ //TMRh20

    write_payload(buf, len, multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD);
    if (startTx) {
        ce(HIGH);
    }
}

/****************************************************************************/

//Added the original startWrite back in so users can still use interrupts, ack payloads, etc
//Allows the library to pass all tests
bool RF24::startWrite(const void* buf, uint8_t len, const bool multicast)
{

    // Send the payload
    write_payload(buf, len, multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD);
    ce(HIGH);
    #if !defined(F_CPU) || F_CPU > 20000000
    delayMicroseconds(10);
    #endif
    ce(LOW);
    return !(status & _BV(TX_FULL));
}

/****************************************************************************/

bool RF24::rxFifoFull()
{
    return read_register(FIFO_STATUS) & _BV(RX_FULL);
}

/****************************************************************************/

bool RF24::txStandBy()
{

    #if defined(FAILURE_HANDLING) || defined(RF24_LINUX)
    uint32_t timeout = millis();
    #endif
    while (!(read_register(FIFO_STATUS) & _BV(TX_EMPTY))) {
        if (status & _BV(MAX_RT)) {
            write_register(NRF_STATUS, _BV(MAX_RT));
            ce(LOW);
            flush_tx();    //Non blocking, flush the data
            return 0;
        }
        #if defined(FAILURE_HANDLING) || defined(RF24_LINUX)
        if (millis() - timeout > 95) {
            errNotify();
            #if defined(FAILURE_HANDLING)
            return 0;
            #endif
        }
        #endif
    }

    ce(LOW);               //Set STANDBY-I mode
    return 1;
}

/****************************************************************************/

bool RF24::txStandBy(uint32_t timeout, bool startTx)
{

    if (startTx) {
        stopListening();
        ce(HIGH);
    }
    uint32_t start = millis();

    while (!(read_register(FIFO_STATUS) & _BV(TX_EMPTY))) {
        if (status & _BV(MAX_RT)) {
            write_register(NRF_STATUS, _BV(MAX_RT));
            ce(LOW); // Set re-transmit
            ce(HIGH);
            if (millis() - start >= timeout) {
                ce(LOW);
                flush_tx();
                return 0;
            }
        }
        #if defined(FAILURE_HANDLING) || defined(RF24_LINUX)
        if (millis() - start > (timeout + 95)) {
            errNotify();
            #if defined(FAILURE_HANDLING)
            return 0;
            #endif
        }
        #endif
    }

    ce(LOW);  //Set STANDBY-I mode
    return 1;

}

/****************************************************************************/

void RF24::maskIRQ(bool tx, bool fail, bool rx)
{
    /* clear the interrupt flags */
    config_reg &= ~(1 << MASK_MAX_RT | 1 << MASK_TX_DS | 1 << MASK_RX_DR);
    /* set the specified interrupt flags */
    config_reg |= fail << MASK_MAX_RT | tx << MASK_TX_DS | rx << MASK_RX_DR;
    write_register(NRF_CONFIG, config_reg);
}

/****************************************************************************/

uint8_t RF24::getDynamicPayloadSize(void)
{
    uint8_t result = read_register(R_RX_PL_WID);

    if (result > 32) {
        flush_rx();
        delay(2);
        return 0;
    }
    return result;
}

/****************************************************************************/

bool RF24::available(void)
{
    return available(NULL);
}

/****************************************************************************/

bool RF24::available(uint8_t* pipe_num)
{
    // get implied RX FIFO empty flag from status byte
    uint8_t pipe = (get_status() >> RX_P_NO) & 0x07;
    if (pipe > 5)
        return 0;

    // If the caller wants the pipe number, include that
    if (pipe_num)
        *pipe_num = pipe;

    return 1;
}

/****************************************************************************/

void RF24::read(void* buf, uint8_t len)
{

    // Fetch the payload
    read_payload(buf, len);

    //Clear the only applicable interrupt flags
    write_register(NRF_STATUS, _BV(RX_DR));

}

/****************************************************************************/

void RF24::whatHappened(bool& tx_ok, bool& tx_fail, bool& rx_ready)
{
    // Read the status & reset the status in one easy call
    // Or is that such a good idea?
    write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));

    // Report to the user what happened
    tx_ok = status & _BV(TX_DS);
    tx_fail = status & _BV(MAX_RT);
    rx_ready = status & _BV(RX_DR);
}

/****************************************************************************/

void RF24::openWritingPipe(uint64_t value)
{
    // Note that AVR 8-bit uC's store this LSB first, and the NRF24L01(+)
    // expects it LSB first too, so we're good.

    write_register(RX_ADDR_P0, reinterpret_cast<uint8_t*>(&value), addr_width);
    write_register(TX_ADDR, reinterpret_cast<uint8_t*>(&value), addr_width);
}

/****************************************************************************/
void RF24::openWritingPipe(const uint8_t* address)
{
    // Note that AVR 8-bit uC's store this LSB first, and the NRF24L01(+)
    // expects it LSB first too, so we're good.
    write_register(RX_ADDR_P0, address, addr_width);
    write_register(TX_ADDR, address, addr_width);
}

/****************************************************************************/
static const PROGMEM uint8_t child_pipe[] = {RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2,
                                             RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5};

void RF24::openReadingPipe(uint8_t child, uint64_t address)
{
    // If this is pipe 0, cache the address.  This is needed because
    // openWritingPipe() will overwrite the pipe 0 address, so
    // startListening() will have to restore it.
    if (child == 0) {
        memcpy(pipe0_reading_address, &address, addr_width);
    }

    if (child <= 5) {
        // For pipes 2-5, only write the LSB
        if (child < 2) {
            write_register(pgm_read_byte(&child_pipe[child]), reinterpret_cast<const uint8_t*>(&address), addr_width);
        } else {
            write_register(pgm_read_byte(&child_pipe[child]), reinterpret_cast<const uint8_t*>(&address), 1);
        }

        // Note it would be more efficient to set all of the bits for all open
        // pipes at once.  However, I thought it would make the calling code
        // more simple to do it this way.
        write_register(EN_RXADDR, read_register(EN_RXADDR) | _BV(pgm_read_byte(&child_pipe_enable[child])));
    }
}

/****************************************************************************/
void RF24::setAddressWidth(uint8_t a_width)
{

    if (a_width -= 2) {
        write_register(SETUP_AW, a_width % 4);
        addr_width = (a_width % 4) + 2;
    } else {
        write_register(SETUP_AW, 0);
        addr_width = 2;
    }

}

/****************************************************************************/

void RF24::openReadingPipe(uint8_t child, const uint8_t* address)
{
    // If this is pipe 0, cache the address.  This is needed because
    // openWritingPipe() will overwrite the pipe 0 address, so
    // startListening() will have to restore it.
    if (child == 0) {
        memcpy(pipe0_reading_address, address, addr_width);
    }
    if (child <= 5) {
        // For pipes 2-5, only write the LSB
        if (child < 2) {
            write_register(pgm_read_byte(&child_pipe[child]), address, addr_width);
        } else {
            write_register(pgm_read_byte(&child_pipe[child]), address, 1);
        }

        // Note it would be more efficient to set all of the bits for all open
        // pipes at once.  However, I thought it would make the calling code
        // more simple to do it this way.
        write_register(EN_RXADDR, read_register(EN_RXADDR) | _BV(pgm_read_byte(&child_pipe_enable[child])));

    }
}

/****************************************************************************/

void RF24::closeReadingPipe(uint8_t pipe)
{
    write_register(EN_RXADDR, read_register(EN_RXADDR) & ~_BV(pgm_read_byte(&child_pipe_enable[pipe])));
}

/****************************************************************************/

void RF24::toggle_features(void)
{
    beginTransaction();
    #if defined (RF24_SPI_PTR)
    status = _spi->transfer(ACTIVATE);
    _spi->transfer(0x73);
    #else
    status = _SPI.transfer(ACTIVATE);
    _SPI.transfer(0x73);
    #endif
    endTransaction();
}

/****************************************************************************/

void RF24::enableDynamicPayloads(void)
{
    // Enable dynamic payload throughout the system

    //toggle_features();
    write_register(FEATURE, read_register(FEATURE) | _BV(EN_DPL));

    IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n", read_register(FEATURE)));

    // Enable dynamic payload on all pipes
    //
    // Not sure the use case of only having dynamic payload on certain
    // pipes, so the library does not support it.
    write_register(DYNPD, read_register(DYNPD) | _BV(DPL_P5) | _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) | _BV(DPL_P1) | _BV(DPL_P0));

    dynamic_payloads_enabled = true;
}

/****************************************************************************/
void RF24::disableDynamicPayloads(void)
{
    // Disables dynamic payload throughout the system.  Also disables Ack Payloads

    //toggle_features();
    write_register(FEATURE, 0);

    IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n", read_register(FEATURE)));

    // Disable dynamic payload on all pipes
    //
    // Not sure the use case of only having dynamic payload on certain
    // pipes, so the library does not support it.
    write_register(DYNPD, 0);

    dynamic_payloads_enabled = false;
    ack_payloads_enabled = false;
}

/****************************************************************************/

void RF24::enableAckPayload(void)
{
    // enable ack payloads and dynamic payload features

    if (!ack_payloads_enabled){
        write_register(FEATURE, read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL));

        IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n", read_register(FEATURE)));

        // Enable dynamic payload on pipes 0 & 1
        write_register(DYNPD, read_register(DYNPD) | _BV(DPL_P1) | _BV(DPL_P0));
        dynamic_payloads_enabled = true;
        ack_payloads_enabled = true;
    }
}

/****************************************************************************/

void RF24::disableAckPayload(void)
{
    // disable ack payloads (leave dynamic payload features as is)
    if (ack_payloads_enabled){
        write_register(FEATURE, read_register(FEATURE) | ~_BV(EN_ACK_PAY));

        IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n", read_register(FEATURE)));

        ack_payloads_enabled = false;
    }
}

/****************************************************************************/

void RF24::enableDynamicAck(void)
{
    //
    // enable dynamic ack features
    //
    //toggle_features();
    write_register(FEATURE, read_register(FEATURE) | _BV(EN_DYN_ACK));

    IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n", read_register(FEATURE)));
}

/****************************************************************************/

bool RF24::writeAckPayload(uint8_t pipe, const void* buf, uint8_t len)
{
    if (ack_payloads_enabled){
        const uint8_t* current = reinterpret_cast<const uint8_t*>(buf);

        write_payload(current, len, W_ACK_PAYLOAD | (pipe & 0x07));
        return !(status & _BV(TX_FULL));
    }
    return 0;
}

/****************************************************************************/

bool RF24::isAckPayloadAvailable(void)
{
    return available(NULL);
}

/****************************************************************************/

bool RF24::isPVariant(void)
{
    return _is_p_variant;
}

/****************************************************************************/

void RF24::setAutoAck(bool enable)
{
    if (enable){
        write_register(EN_AA, 0x3F);
    }else{
        write_register(EN_AA, 0);
        // accomodate ACK payloads feature
        if (ack_payloads_enabled){
            disableAckPayload();
        }
    }
}

/****************************************************************************/

void RF24::setAutoAck(uint8_t pipe, bool enable)
{
    if (pipe < 6) {
        uint8_t en_aa = read_register(EN_AA);
        if (enable) {
            en_aa |= _BV(pipe);
        }else{
            en_aa &= ~_BV(pipe);
            if (ack_payloads_enabled && !pipe){
                disableAckPayload();
            }
        }
        write_register(EN_AA, en_aa);
    }
}

/****************************************************************************/

bool RF24::testCarrier(void)
{
    return (read_register(CD) & 1);
}

/****************************************************************************/

bool RF24::testRPD(void)
{
    return (read_register(RPD) & 1);
}

/****************************************************************************/

void RF24::setPALevel(uint8_t level, bool lnaEnable)
{

    uint8_t setup = read_register(RF_SETUP) & 0xF8;

    if (level > 3) {                            // If invalid level, go to max PA
        level = (RF24_PA_MAX << 1) + lnaEnable; // +1 to support the SI24R1 chip extra bit
    } else {
        level = (level << 1) + lnaEnable;       // Else set level as requested
    }

    write_register(RF_SETUP, setup |= level);   // Write it to the chip
}

/****************************************************************************/

uint8_t RF24::getPALevel(void)
{

    return (read_register(RF_SETUP) & (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH))) >> 1;
}

/****************************************************************************/

uint8_t RF24::getARC(void)
{

    return read_register(OBSERVE_TX) & 0x0F;
}

/****************************************************************************/

bool RF24::setDataRate(rf24_datarate_e speed)
{
    bool result = false;
    uint8_t setup = read_register(RF_SETUP);

    // HIGH and LOW '00' is 1Mbs - our default
    setup &= ~(_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));

    #if !defined(F_CPU) || F_CPU > 20000000
    txDelay = 280;
    #else //16Mhz Arduino
    txDelay=85;
    #endif
    if (speed == RF24_250KBPS) {
        // Must set the RF_DR_LOW to 1; RF_DR_HIGH (used to be RF_DR) is already 0
        // Making it '10'.
        setup |= _BV(RF_DR_LOW);
        #if !defined(F_CPU) || F_CPU > 20000000
        txDelay = 505;
        #else //16Mhz Arduino
        txDelay = 155;
        #endif
    } else {
        // Set 2Mbs, RF_DR (RF_DR_HIGH) is set 1
        // Making it '01'
        if (speed == RF24_2MBPS) {
            setup |= _BV(RF_DR_HIGH);
            #if !defined(F_CPU) || F_CPU > 20000000
            txDelay = 240;
            #else // 16Mhz Arduino
            txDelay = 65;
            #endif
        }
    }
    write_register(RF_SETUP, setup);

    // Verify our result
    if (read_register(RF_SETUP) == setup) {
        result = true;
    }
    return result;
}

/****************************************************************************/

rf24_datarate_e RF24::getDataRate(void)
{
    rf24_datarate_e result;
    uint8_t dr = read_register(RF_SETUP) & (_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));

    // switch uses RAM (evil!)
    // Order matters in our case below
    if (dr == _BV(RF_DR_LOW)) {
        // '10' = 250KBPS
        result = RF24_250KBPS;
    } else if (dr == _BV(RF_DR_HIGH)) {
        // '01' = 2MBPS
        result = RF24_2MBPS;
    } else {
        // '00' = 1MBPS
        result = RF24_1MBPS;
    }
    return result;
}

/****************************************************************************/

void RF24::setCRCLength(rf24_crclength_e length)
{
    config_reg &= ~(_BV(CRCO) | _BV(EN_CRC));

    // switch uses RAM (evil!)
    if (length == RF24_CRC_DISABLED) {
        // Do nothing, we turned it off above.
    } else if (length == RF24_CRC_8) {
        config_reg |= _BV(EN_CRC);
    } else {
        config_reg |= _BV(EN_CRC);
        config_reg |= _BV(CRCO);
    }
    write_register(NRF_CONFIG, config_reg);
}

/****************************************************************************/

rf24_crclength_e RF24::getCRCLength(void)
{
    rf24_crclength_e result = RF24_CRC_DISABLED;
    uint8_t AA = read_register(EN_AA);
    config_reg = read_register(NRF_CONFIG);

    if (config_reg & _BV(EN_CRC) || AA) {
        if (config_reg & _BV(CRCO)) {
            result = RF24_CRC_16;
        } else {
            result = RF24_CRC_8;
        }
    }

    return result;
}

/****************************************************************************/

void RF24::disableCRC(void)
{
    config_reg &= ~_BV(EN_CRC);
    write_register(NRF_CONFIG, config_reg);
}

/****************************************************************************/
void RF24::setRetries(uint8_t delay, uint8_t count)
{
    write_register(SETUP_RETR, rf24_min(15, delay) << ARD | rf24_min(15, count));
}

/****************************************************************************/
void RF24::startConstCarrier(rf24_pa_dbm_e level, uint8_t channel)
{
    stopListening();
    write_register(RF_SETUP, read_register(RF_SETUP) | _BV(CONT_WAVE) | _BV(PLL_LOCK));
    if (isPVariant()){
        setAutoAck(0);
        setRetries(0, 0);
        uint8_t dummy_buf[32];
        for (uint8_t i = 0; i < 32; ++i)
            dummy_buf[i] = 0xFF;

        // use write_register() instead of openWritingPipe() to bypass
        // truncation of the address with the current RF24::addr_width value
        write_register(TX_ADDR, reinterpret_cast<uint8_t*>(&dummy_buf), 5);
        flush_tx();  // so we can write to top level

        // use write_register() instead of write_payload() to bypass
        // truncation of the payload with the current RF24::payload_size value
        write_register(W_TX_PAYLOAD, reinterpret_cast<const uint8_t*>(&dummy_buf), 32);

        disableCRC();
    }
    setPALevel(level);
    setChannel(channel);
    IF_SERIAL_DEBUG(printf_P(PSTR("RF_SETUP=%02x\r\n"), read_register(RF_SETUP)));
    ce(HIGH);
    if (isPVariant()){
        delay(1); // datasheet says 1 ms is ok in this instance
        ce(LOW);
        reUseTX();
    }
}

/****************************************************************************/
void RF24::stopConstCarrier()
{
    /*
     * A note from the datasheet:
     * Do not use REUSE_TX_PL together with CONT_WAVE=1. When both these
     * registers are set the chip does not react when setting CE low. If
     * however, both registers are set PWR_UP = 0 will turn TX mode off.
     */
    powerDown();  // per datasheet recommendation (just to be safe)
    write_register(RF_SETUP, read_register(RF_SETUP) & ~_BV(CONT_WAVE) & ~_BV(PLL_LOCK));
    ce(LOW);
}
