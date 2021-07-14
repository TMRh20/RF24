set(RF24_LINKED_DRIVER "")
set(RF24_DRIVER "UNKNOWN" CACHE STRING "override automatic configuration of RF24's utility driver.
    Specify 1 of the following supported drivers (ie -DRF24_DRIVER=SPIDEV):
    wiringPi
    RPi
    SPIDEV
    MRAA
    LittleWire"
)

###########################
# detect pre-existing (locally installed) 3rd party libraries
###########################

# detect installed libraries despite what RF24_DRIVER is set to
# this is always done for cross-compiling purposes
find_library(LibMRAA mraa)
find_library(LibWiringPi wiringPi)
find_library(LibLittleWire littlewire-spi)
if(EXISTS /dev/spidev0.0)
    set(SPIDEV_EXISTS TRUE)
else()
    set(SPIDEV_EXISTS FALSE)
endif()


if(${RF24_DRIVER} STREQUAL "UNKNOWN") # invokes automatic configuration
    if("${SOC}" STREQUAL "BCM2708" OR "${SOC}" STREQUAL "BCM2709" OR "${SOC}" STREQUAL "BCM2835")
        set(RF24_DRIVER RPi CACHE STRING "using folder /utility/RPi" FORCE)
    elseif(NOT "${LibMRAA}" STREQUAL "LibMRAA-NOTFOUND")
        message(STATUS "Found MRAA library: ${LibMRAA}")
        set(RF24_DRIVER MRAA CACHE STRING "using folder /utility/MRAA" FORCE)
    elseif(NOT "${LibWiringPi}" STREQUAL "LibWiringPi-NOTFOUND")
        message(STATUS "Found wiringPi library: ${LibWiringPi}")
        set(RF24_DRIVER wiringPi CACHE STRING "using folder /utility/wiringPi" FORCE)
    elseif(NOT "${LibLittleWire}" STREQUAL "LibLittleWire-NOTFOUND")
        message(STATUS "Found LittleWire library: ${LibLittleWire}")
        set(RF24_DRIVER LittleWire CACHE STRING "using folder /utility/LittleWire" FORCE)
    elseif(SPIDEV_EXISTS) # should be a non-empty string if SPI is enabled
        message(STAUS "detected that SPIDEV is enabled: ${SPIDEV_EXISTS}")
        set(RF24_DRIVER SPIDEV CACHE STRING "using folder /utility/SPIDEV" FORCE)
    endif()
endif()

message(STATUS "Using driver: ${RF24_DRIVER}")
