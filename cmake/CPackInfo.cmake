# This module will build a debian compatible package to install - handy for cross-compiling

# get target arch if not cross-compiling
if(NOT DEFINED TARGET_ARCH) # TARGET_ARCH is defined only in the toolchain_<ARCH_TYPE>.cmake files
    execute_process(COMMAND dpkg --print-architecture
        OUTPUT_VARIABLE TARGET_ARCH
        )
    string(STRIP "${TARGET_ARCH}" TARGET_ARCH)
endif()

# set the Cpack generators (specific to types of packages to create)
set(CPACK_GENERATOR DEB) # RPM requires rpmbuild executable

# assemble a debian package filename from known info
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_FILE_NAME "lib${LibTargetName}-${RF24_DRIVER}_${${LibName}_VERSION_MAJOR}.${${LibName}_VERSION_MINOR}-${${LibName}_VERSION_PATCH}_${TARGET_ARCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${${LibName}_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${${LibName}_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${${LibName}_VERSION_PATCH}")
###############################
# info specific debian packages
###############################
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${TARGET_ARCH})
set(CPACK_DEBIAN_PACKAGE_SECTION libs)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)

# set dependencies based on utility drivers
if("${RF24_DRIVER}" STREQUAL "MRAA")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS mraa)
elseif("${RF24_DRIVER}" STREQUAL "wiringPi")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS wiringPi)
elseif("${RF24_DRIVER}" STREQUAL "littlewire-spi")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS littlewire-spi)
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
list(JOIN POST_SCRIPTS ";" EXTRA_CTRL_FILES)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA EXTRA_CTRL_FILES)
message(STATUS "ready to package: ${CPACK_PACKAGE_FILE_NAME}.deb")

include(CPack)
#[[
# copy over templated control file while replacing variables within
configure_file(cmake/deb_pkg_control.in ${DEB_PKG_NAME}/DEBIAN/control)

# copy file describing the license information (could be done w/ file(COPY) but this way allows CMake to modify it if needed)
configure_file(cmake/deb_pkg_copyright.in ${DEB_PKG_NAME}/debian/copyright)
]]