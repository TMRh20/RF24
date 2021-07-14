# try to get the CPU model using a Linux bash command
if(NOT SOC) # if SOC variable not defined by user at CLI
    if(EXISTS "/sys/class/sunxi_info/sys_info")
        execute_process(COMMAND grep sunxi_platform  /sys/class/sunxi_info/sys_info
            OUTPUT_VARIABLE CPU_MODEL
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        execute_process(COMMAND grep Hardware /proc/cpuinfo
            OUTPUT_VARIABLE CPU_MODEL
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    string(FIND "${CPU_MODEL}" ":" cpu_is_described)
    if(${cpu_is_described} GREATER 0) # Hardware field does exist
        math(EXPR cpu_is_described "${cpu_is_described} + 1")
        string(SUBSTRING "${CPU_MODEL}" ${cpu_is_described} -1 SOC)
        string(STRIP "${SOC}" SOC)
    else() # Hardware field does not exist
        set(SOC "UNKNOWN") # use this string as a sentinel
    endif()
    message(STATUS "detected SoC: ${SOC}")
else()
    message(STATUS "SOC set to ${SOC}")
endif()

string(FIND "${SOC}" "Generic AM33XX" is_AM33XX)

#[[ detect machine hardware name
This CPU_TYPE variable is not used anywhere.
It remains as useful prompt info & to be consistent with old build system ]]
execute_process(COMMAND uname -m
    OUTPUT_VARIABLE CPU_TYPE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "detected CPU type: ${CPU_TYPE}")

# identify the compiler base name for customizing flags
# THIS ONLY WORKS/TESTED FOR GNU COMPILERS
if(NOT CMAKE_CROSSCOMPILING) # need to use /usr/lib/gcc soft symlink
    # NOTE the following command doesn't work with " | tail -1" appended
    execute_process(COMMAND ls /usr/lib/gcc
        OUTPUT_VARIABLE tool_name
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # use only last entry if multiple entries are returned; this tactic is a possible point of error
    string(FIND "${tool_name}" "\n" last_list_delimiter REVERSE)
    if(last_list_delimiter GREATER -1)
        math(EXPR last_list_delimiter "${last_list_delimiter} + 1")
        string(SUBSTRING "${tool_name}" ${last_list_delimiter} -1 tool_name)
    endif()

else() # we can use the compiler's name of the path set in the toolchain file
    string(REGEX REPLACE "^\/usr\/bin\/(.*)-gcc.*" "\\1" tool_name "${CMAKE_C_COMPILER}")
endif()

message(STATUS "tool name being used: ${tool_name}")

# add compiler flags to optomize builds with arm-linux-gnueabihf-g* compilers
if("${tool_name}" STREQUAL "arm-linux-gnueabihf")
    if("${SOC}" STREQUAL "BCM2835" OR "${SOC}" STREQUAL "BCM2708")
        add_compile_options(-marm -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard)
    elseif("$SOC" STREQUAL "BCM2836" OR "${SOC}" STREQUAL "BCM2709")
        add_compile_options(-march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard)
    elseif(${is_AM33XX} GREATER -1)
        add_compile_options(-march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard)
    elseif("$SOC" STREQUAL "sun4i" OR "${SOC}" STREQUAL "Sun4iw1p1") # A10
        add_compile_options(-march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard)
    elseif("$SOC" STREQUAL "sun5i" OR "${SOC}" STREQUAL "Sun4iw2p1") # A13
        add_compile_options(-march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard)
    elseif("$SOC" STREQUAL "sun7i" OR "${SOC}" STREQUAL "Sun8iw2p1") # A20
        add_compile_options(-march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard)
    elseif("$SOC" STREQUAL "sun8i" OR "${SOC}" STREQUAL "Sun8iw7p1") # H3
        add_compile_options(-march=armv8-a -mtune=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard)
    endif()
endif()
