###########################
### declare the appropriate sources and install rules based on driver selected
###########################
if ("${RF24_DRIVER}" STREQUAL  "wiringPi") # Use wiringPi
    set(RF24_LINKED_DRIVER ${LibWiringPi} PARENT_SCOPE)
    add_compile_options(-pthread)
    set(RF24_DRIVER_SOURCES
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/includes.h
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/spi.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/RF24_arch_config.h
        PARENT_SCOPE
    )
    install(FILES
            ${RF24_DRIVER}/includes.h
            ${RF24_DRIVER}/spi.h
            ${RF24_DRIVER}/RF24_arch_config.h
        DESTINATION include/RF24/utility/${RF24_DRIVER}
    )
elseif("${RF24_DRIVER}" STREQUAL "RPi") # use RPi
    set(RF24_DRIVER_SOURCES
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/includes.h
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/bcm2835.c
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/spi.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/compatibility.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/RF24_arch_config.h
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/interrupt.cpp
        PARENT_SCOPE
    )
    install(FILES
            ${RF24_DRIVER}/includes.h
            ${RF24_DRIVER}/bcm2835.h
            ${RF24_DRIVER}/spi.h
            ${RF24_DRIVER}/compatibility.h
            ${RF24_DRIVER}/RF24_arch_config.h
            ${RF24_DRIVER}/interrupt.h
        DESTINATION include/RF24/utility/${RF24_DRIVER}
    )
elseif("${RF24_DRIVER}" STREQUAL "SPIDEV") # use SPIDEV
    if(NOT SPIDEV_EXISTS)
        message(WARNING "Detecting /dev/spidev0.0 failed - continuing anyway. Please make sure SPI is enabled.")
    endif()
    set(RF24_DRIVER_SOURCES
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/includes.h
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/gpio.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/spi.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/compatibility.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/RF24_arch_config.h
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/interrupt.cpp
        PARENT_SCOPE
    )
    install(FILES
            ${RF24_DRIVER}/includes.h
            ${RF24_DRIVER}/gpio.h
            ${RF24_DRIVER}/spi.h
            ${RF24_DRIVER}/compatibility.h
            ${RF24_DRIVER}/RF24_arch_config.h
            ${RF24_DRIVER}/interrupt.h
        DESTINATION include/RF24/utility/${RF24_DRIVER}
    )
elseif("${RF24_DRIVER}" STREQUAL "MRAA") # use MRAA
    set(RF24_LINKED_DRIVER ${LibMRAA} PARENT_SCOPE)
    set(RF24_DRIVER_SOURCES
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/includes.h
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/gpio.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/spi.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/compatibility.cpp
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/RF24_arch_config.h
        PARENT_SCOPE
    )
    install(FILES
            ${RF24_DRIVER}/includes.h
            ${RF24_DRIVER}/gpio.h
            ${RF24_DRIVER}/spi.h
            ${RF24_DRIVER}/compatibility.h
            ${RF24_DRIVER}/RF24_arch_config.h
        DESTINATION include/RF24/utility/${RF24_DRIVER}
    )
elseif("${RF24_DRIVER}" STREQUAL "LittleWire") # use LittleWire
    set(RF24_LINKED_DRIVER ${LibLittleWire} PARENT_SCOPE)
    set(RF24_DRIVER_SOURCES
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/includes.h
        ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/RF24_arch_config.h
        PARENT_SCOPE
    )
    install(FILES
            ${RF24_DRIVER}/includes.h
            ${RF24_DRIVER}/RF24_arch_config.h
        DESTINATION include/RF24/utility/${RF24_DRIVER}
    )
else() # No valid/supported driver selected nor detected... this is vital
    message(FATAL_ERROR "No valid/supported driver selected or auto-detection failed to resolve one.
        Please specify 1 of the following supported drivers (ie `-D RF24_DRIVER=SPIDEV`):
        \twiringPi
        \tRPi
        \tSPIDEV
        \tMRAA
        \tLittleWire"
    )
endif()

# copy the includes file to the project source directory's utility folder
execute_process(COMMAND cp ${CMAKE_SOURCE_DIR}/utility/${RF24_DRIVER}/includes.h ${CMAKE_SOURCE_DIR}/utility/includes.h)
