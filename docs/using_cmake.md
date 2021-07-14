# Using CMake
A more modern approach instead of using hand-crafted _Makefiles_ & _configure_ scripts
to build & install software. Please note that these instructions are not needed if you
have already installed the library using [these older instructions](md_docs_linux_install.html)

## Installing the library

### Using a package manager
The RF24 library now (as of v1.4.1) has pre-built packages (*.deb or *.rpm files) that can be installed on a
Raspberry Pi. These packages can be found on the library's
[GitHub release page](https://GitHub.com/nRF24/RF24/releases)

@warning If you have previously installed the library from source code, then you will need
to uninstall it manually to avoid runtime conflicts.
```shell
sudo rm /usr/local/lib/librf24.*
sudo rm /usr/local/lib/librf24-bcm.so
sudo rm -r /usr/local/include/RF24
```
The _librf24-bcm.so_ file may not exist if you used CMake to install the library.

1. Download the appropriate package for your machine

   Go to the library's [GitHub release page](https://GitHub.com/nRF24/RF24/releases), and look for
   the latest release's assets.

   For all Raspberry Pi variants using the Raspberry Pi OS (aka Raspbian), you need the file marked
   for _armhf_ architecture.

   For Raspberry Pi variants using a 64-bit OS (like Ubuntu), you need the file marked for
   _arm64_ architecture.

   Notice that the filenames will include the name of the utility driver that the package was built with.
   This does not mean that the LittleWire, MRAA, or wiringPi libraries are included in the package (you will still
   need to install those yourself beforehand).
2. Install the downloaded pkg

   If you downloaded the file directly from your target machine using the desktop environment, then
   you only need to double-click the package (deb or rpm) file, and the OS should do the rest. <br>

   If you downloaded the file remotely and want to copy it over ssh, then use the `scp` command in a terminal.
   ```shell
   scp pkg_filename.deb pi@host_name:~/Downloads
   ```

   @note You do not need to do this from within an ssh session. Also, you can use the target machine's IP
   address instead of its host name.<br>

   The `scp` command will ask you for a password belonging to the user's name on the remote machine (we used
   `pi` in the above example).<br>
   Now you can open up a ssh session and install the copied package from the terminal.
   ```shell
   ssh pi@host_name
   cd Downloads
   dpkg -i pkg_filename.deb
   ```

### Building from source code
1. Install prerequisites if there are any (wiringPi, MRAA, LittleWire libraries, setup SPI device etc)

   CMake may need to be installed
   ```shell
   sudo apt-get install cmake
   ```
   @note See the [MRAA](http://iotdk.intel.com/docs/master/mraa/index.html) documentation for
   more info on installing MRAA <br>
2. Make a directory to contain the RF24 library and possibly other RF24* libraries and enter it
   ```shell
   mkdir ~/rf24libs
   cd ~/rf24libs
   ```
3. Clone the [RF24 repo](https://github.com/nRF24/RF24) and navigate to it
   ```shell
   git clone https://github.com/nRF24/RF24.git RF24
   cd RF24
   ```
4. Create a build directory inside the RF24 directory and navigate to it.
   ```shell
   mkdir build
   cd build
   ```
5. Configure build environment

   @note When using these instructions to install RF24Mesh, RF24Network, or RF24Gateway,
   the following `RF24_DRIVER` option is not needed (it is only for the RF24 library).

   ```shell
   cmake .. -D RF24_DRIVER=SPIDEV
   ```
   Instead of using `SPIDEV` driver (recommended), you can also specify the `RPi`, `wiringPi`,
   `MRAA`, or `LittleWire` as alternative drivers.

   If the `RF24_DRIVER` option is not specified, then it will be automatically configured based
   on the detected CPU or installed libraries (defaults to `SPIDEV` when auto-detection fails).
6. Build and install the library
   ```shell
   make
   sudo make install
   ```
7. Build the examples

   Navigate to the _examples_linux_ directory
   ```shell
   cd ../examples_linux
   ```

   Make sure the pins used in the examples match the pins you used to connect the radio module
   ```shell
   nano gettingstarted.cpp
   ```
   and edit the pin numbers as directed in the [linux/RPi general documation](md_docs_rpi_general.html).

   Create a build directory in the examples_linux directory and navigate to it.
   ```shell
   mkdir build
   cd build
   ```

   Now you are ready to build the examples.<br>
   ```shell
   cmake ..
   make
   ```
   If using the `MRAA` or `wiringPi` drivers, then you may need to specify the `RF24_DRIVER`
   option again.
   ```shell
   cmake .. -D RF24_DRIVER=wiringPi
   make
   ```
8. Run an example file
   ```shell
   sudo ./gettingstarted
   ```

## Cross-compiling the library
The RF24 library comes with some pre-made toolchain files (located in the _RF24/cmake/toolchains_
directory) to use in CMake. To use these toolchain files, additional command line options are needed
when configuring CMake to build the library (step 5 in the above instructions to build from source).
```shell
cmake .. -D CMAKE_TOOLCHAIN_FILE=cmake/toolchains/armhf.cmake
make
```
If you plan on using the cross-compiled library with your personal cross-compiled project, then
it is advised to specify the path that your project will look in when linking to the RF24 library:
```shell
cmake .. -D CMAKE_INSTALL_PREFIX=/usr/arm-linux-gnueabihf -D CMAKE_TOOLCHAIN_FILE=cmake/toolchains/armhf.cmake
make
sudo make install
```

Remember to also specify the `RF24_DRIVER` option if not using the auto-configuration feature (see step 5
in the above instructions to build from source).

### Installing the library remotely
To install remotely, you can create an installable package file using CMake's built-in program called CPack.
```shell
cmake .. -D CMAKE_TOOLCHAIN_FILE=cmake/toolchains/armhf.cmake
make
cpack
```
This will create a deb file and a rpm file in a new sub-directory called "pkgs" within the build directory.
You can use either of these packages to install the library to your target machine (see the above
instructions about using a package manager).
