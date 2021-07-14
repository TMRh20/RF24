# This module will build a debian compatible package to install - handy for cross-compiling

if(NOT PKG_REV)
    set(PKG_REV "1")
endif()

# get target arch if not cross-compiling
if(NOT TARGET_ARCH) # TARGET_ARCH is defined only in the toolchain_<ARCH_TYPE>.cmake files
    execute_process(COMMAND dpkg --print-architecture
        OUTPUT_VARIABLE TARGET_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

# set the Cpack generators (specific to types of packages to create)
set(CPACK_GENERATOR DEB RPM) # RPM requires rpmbuild executable

# assemble a debian package filename from known info
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_FILE_NAME "lib${LibTargetName}-${RF24_DRIVER}_${${LibName}_VERSION_STRING}-${PKG_REV}_${TARGET_ARCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${${LibName}_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${${LibName}_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${${LibName}_VERSION_PATCH}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/pkgs") # for easy uploading to github releases

###############################
# info specific debian packages
###############################
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${TARGET_ARCH})
set(CPACK_DEBIAN_PACKAGE_SECTION libs)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)


###############################
# info specific rpm (fedora) packages
###############################
set(CPACK_RPM_FILE_NAME "lib${LibTargetName}-${RF24_DRIVER}-${${LibName}_VERSION_STRING}-${PKG_REV}.${TARGET_ARCH}.rpm")
set(CPACK_RPM_PACKAGE_ARCHITECTURE ${TARGET_ARCH})
set(CPACK_RPM_PACKAGE_LICENSE "GPLv2.0")
set(CPACK_RPM_PACKAGE_VENDOR "Humanity")

# set dependencies based on utility drivers
if("${RF24_DRIVER}" STREQUAL "MRAA")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS mraa)
    set(CPACK_RPM_PACKAGE_REQUIRES "mraa")
elseif("${RF24_DRIVER}" STREQUAL "wiringPi")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS wiringPi)
    set(CPACK_RPM_PACKAGE_REQUIRES "wiringPi")
elseif("${RF24_DRIVER}" STREQUAL "littlewire-spi")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS littlewire-spi)
    set(CPACK_RPM_PACKAGE_REQUIRES "littlewire-spi")
endif()

# create a post-install & post-removal scripts to update linker
set(POST_SCRIPTS
    ${CMAKE_BINARY_DIR}/DEBIAN/postrm
    ${CMAKE_BINARY_DIR}/DEBIAN/postinst
)
foreach(script ${POST_SCRIPTS})
    file(WRITE ${script} ldconfig)
    execute_process(COMMAND chmod +x ${script})
    execute_process(COMMAND chmod 775 ${script})
endforeach()
# declare scripts for deb pkgs
list(JOIN POST_SCRIPTS ";" EXTRA_CTRL_FILES)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${EXTRA_CTRL_FILES})
# declare scripts for rpm pkgs
list(POP_FRONT POST_SCRIPTS CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE)
list(POP_FRONT POST_SCRIPTS CPACK_RPM_POST_INSTALL_SCRIPT_FILE)

message(STATUS "ready to package: ${CPACK_PACKAGE_FILE_NAME}.deb")
message(STATUS "ready to package: ${CPACK_RPM_FILE_NAME}")

include(CPack)
