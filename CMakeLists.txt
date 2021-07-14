# Check if we are building a pico-sdk based project
# (or more exactly: if we just got included in a pico-sdk based project)
if (PICO_SDK_PATH)
    # If so, load the relevant CMakeLists-file but don't do anything else
    include(${CMAKE_CURRENT_LIST_DIR}/utility/rp2/CMakeLists.txt)
    return()
endif()

cmake_minimum_required(VERSION 3.15)

# Set the project name to your project name
project(RF24 C CXX)
include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)

# allow CMake CLI options to configure RF24_config.h macros
option(SERIAL_DEBUG "enable/disable debugging output" OFF)
option(MINIMAL "exclude optional source code to keep compile size compact" OFF)

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)
add_compile_options(-Ofast -Wall -pthread)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" OFF)
    if(ENABLE_BUILD_WITH_TIME_TRACE)
        add_compile_definitions(project_options INTERFACE -ftime-trace)
    endif()
endif()

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# enable cache system
include(cmake/Cache.cmake)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
option(ENABLE_TESTING "Enable Test Builds" OFF) # for end-user projects
option(ENABLE_FUZZING "Enable Fuzzing Builds" OFF) # for end-user projects

if(ENABLE_TESTING)
    enable_testing()
    message("Building Tests.")
    add_subdirectory(test) # directory doesn't exist, so this does nothing.
endif()

if(ENABLE_FUZZING)
    message("Building Fuzz Tests, using fuzzing sanitizer https://www.llvm.org/docs/LibFuzzer.html")
    add_subdirectory(fuzz_test) # directory doesn't exist, so this does nothing.
endif()

#####################################
### Now we actually build the library
#####################################

# get library info from Arduino IDE's required library.properties file
include(cmake/GetLibInfo.cmake)

# detect the CPU make and type
include(cmake/detectCPU.cmake) # sets the variable SOC accordingly

# auto-detect what driver to use
# auto-detect can be overriden using `cmake .. -D RF24_DRIVER=<supported driver>`
include(cmake/AutoConfig_RF24_DRIVER.cmake)

#[[ adding the utility sub-directory will
    1. set variables RF24_DRIVER, RF24_LINKED_DRIVER, and RF24_DRIVER_SOURCES
    2. copy the approriate /utility/*/includes.h file to the /utility folder
    3. set additional install rules according to the RF24_DRIVER specified
]]
add_subdirectory(utility)

# setup CPack options
# package dependencies are resolved correctly only after utility subdirectory is added
include(cmake/CPackInfo.cmake)

add_library(${LibTargetName} SHARED
    RF24.cpp
    ${RF24_DRIVER_SOURCES}
)

target_include_directories(${LibTargetName} PUBLIC utility)

set_target_properties(
    ${LibTargetName}
    PROPERTIES
    SOVERSION ${${LibName}_VERSION_MAJOR}
    VERSION ${${LibName}_VERSION_STRING}
)

if(NOT "${RF24_LINKED_DRIVER}" STREQUAL "") # linking to a pre-compiled utility driver
    message(STATUS "Using utility library: ${RF24_LINKED_DRIVER}")
    target_link_libraries(${LibTargetName} INTERFACE
        project_options
        project_warnings
        STATIC RF24_LINKED_DRIVER
    )
else() # utility driver is compiled with the library - not linking to a pre-compiled utility driver
    target_link_libraries(${LibTargetName} INTERFACE
        project_options
        project_warnings
    )
endif()

# assert the appropriate preprocessor macros for RF24_config.h
if(SERIAL_DEBUG)
    message(STATUS "SERIAL_DEBUG asserted")
    target_compile_definitions(${LibTargetName} PUBLIC SERIAL_DEBUG)
endif()
if(MINIMAL)
    message(STATUS "MINIMAL asserted")
    target_compile_definitions(${LibTargetName} PUBLIC MINIMAL)
endif()
# for RF24_POWERUP_DELAY & RF24_SPI_SPEED, let the default be configured in source code
if(DEFINED RF24_POWERUP_DELAY)
    message(STATUS "RF24_POWERUP_DELAY set to ${RF24_POWERUP_DELAY}")
    target_compile_definitions(${LibTargetName} PUBLIC
        RF24_POWERUP_DELAY=${RF24_POWERUP_DELAY}
    )
endif()
if(DEFINED RF24_SPI_SPEED)
    message(STATUS "RF24_SPI_SPEED set to ${RF24_SPI_SPEED}")
    target_compile_definitions(${LibTargetName} PUBLIC
        RF24_SPI_SPEED=${RF24_SPI_SPEED}
    )
endif()


#####################################
### Install rules for root source dir
### There are separate install rules defined for each utility driver
### Installing the library requires sudo privileges
#####################################
install(TARGETS ${LibTargetName}
    DESTINATION lib
)

install(FILES
        RF24.h
        nRF24L01.h
        printf.h
        RF24_config.h
    DESTINATION include/RF24
)

install(FILES
        utility/includes.h
    DESTINATION include/RF24/utility
)

# CMAKE_CROSSCOMPILING is only TRUE when CMAKE_TOOLCHAIN_FILE is specified via CLI
if("${CMAKE_CROSSCOMPILING}" STREQUAL "FALSE")
    install(CODE "message(STATUS \"Updating ldconfig\")")
    install(CODE "execute_process(COMMAND ldconfig)")
endif()
