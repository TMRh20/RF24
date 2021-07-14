# Linux Installation
Generic Linux devices are supported via SPIDEV, MRAA, RPi native via BCM2835, or using LittleWire.

@note The SPIDEV option should work with most Linux systems supporting spi userspace device. <br>

@warning These instructions are beginning to age because they were designed with the assumption that
the arm-linux-gnueabihf-g** compilers were available and default for the system. If you have problems
using these instructions, please try the [instructions using CMake](md_docs_using_cmake.html). <br>

## Automated Install
**Designed & Tested on RPi** - Defaults to SPIDEV on devices supporting it

1. Install prerequisites if there are any (MRAA, LittleWire libraries, setup SPI device etc)
2. Download the install.sh file from http://tmrh20.github.io/RF24Installer/RPi/install.sh
   ```shell
   wget http://tmrh20.github.io/RF24Installer/RPi/install.sh
   ```
3. Make it executable
   ```shell
   chmod +x install.sh
   ```
4. Run it and choose your options
   ```shell
   ./install.sh
   ```
5. Run an example from one of the libraries
   ```shell
   cd rf24libs/RF24/examples_linux
   ```
   Edit the gettingstarted example, to set your pin configuration
   ```shell
   nano gettingstarted.cpp
   make
   sudo ./gettingstarted
   ```
## Manual Install

1. Install prerequisites if there are any (MRAA, LittleWire libraries, setup SPI device etc)
@note See the [MRAA](http://iotdk.intel.com/docs/master/mraa/index.html) documentation for more info on installing MRAA <br>
2. Make a directory to contain the RF24 and possibly RF24Network lib and enter it
   ```shell
   mkdir ~/rf24libs
   cd ~/rf24libs
   ```
3. Clone the RF24 repo
   ```shell
   git clone https://github.com/tmrh20/RF24.git RF24
   ```
4. Change to the new RF24 directory
   ```shell
   cd RF24
   ```
5. Configure build environment using
   ```shell
   ./configure
   ```
   script. It auto detectes device and build environment.<br>
   For overriding autodetections, use command-line switches, see
   ```shell
   ./configure --help
   ```
   for description.
6. Build the library, and run an example file
   ```shell
   make
   sudo make install
   ```
   ```shell
   cd examples_linux
   ```
   Edit the gettingstarted example, to set your pin configuration
   ```shell
   ano gettingstarted.cpp
   make
   sudo ./gettingstarted
   ```

### Build using SPIDEV

1. Make sure that spi device support is enabled and /dev/spidev\<a\>.\<b\> is present
2. Manual Install using SPIDEV:
   ```shell
   ./configure --driver=SPIDEV
   make
   sudo make install
   ```
3. See the gettingstarted example for an example of pin configuration