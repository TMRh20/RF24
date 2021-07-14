# Linux cross-compilation

@warning These instructions are no longer recommended because they involve disabling security measures
for the target system. Please try the [instructions using CMake](md_docs_using_cmake.html). <br>

RF24 library supports cross-compilation. Advantages of cross-compilation:
- development tools don't have to be installed on target machine
- resources of target machine don't have to be sufficient for compilation
- compilation time can be reduced for large projects

Following prerequisites need to be assured:
- ssh passwordless access to target machine (https://linuxconfig.org/passwordless-ssh)
- sudo of a remote user without password (http://askubuntu.com/questions/334318/sudoers-file-enable-nopasswd-for-user-all-commands)
- cross-compilation toolchain for your target machine; for RPi
  ```shell
  git clone https://github.com/raspberrypi/tools rpi_tools
  ```
  and cross-compilation tools must be in PATH, for example
  ```shell
  export PATH=$PATH:/your/dir/rpi-tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin
  ```

## Cross compilation steps:
1. clone RF24 to a machine for cross-compilation
   ```shell
   git clone https://github.com/nRF24/RF24
   cd RF24
   ```
2. configure for cross compilation
   ```shell
   ./configure --remote=pi@target_linux_host
   ```
   eventually
   ```shell
   ./configure --remote=pi@target_linux_host --driver=<driver>
   ```
3. build
   ```shell
   make
   ```
4. (optional) install library to cross-compilation machine into cross-exvironment - important for compilation of examples
   ```shell
   sudo make install
   ```
5. upload library to target machine
   ```shell
   make upload
   ```
6. (optional) compile examples
   ```shell
   cd examples_linux
   make
   ```
7. (optional) upload examples to target machine
   ```shell
   make upload
   ```

## Cross comilation steps for python wrapper

### Prerequisites
- Python setuptools must be installed on both target and cross-compilation machines
  ```shell
  sudo pip install setuptools
  ```
  or
  ```shell
  sudo apt-get install python-setuptools
  ```

### Installation steps
1. Assure having libboost-python-dev library in your cross-compilation environment. Alternatively, you can install it into your target machine and copy /usr and /lib directories to the cross-compilation machine.
   For example
   ```shell
   mkdir -p rpi_root && rsync -a pi@target_linux_host:/usr :/lib rpi_root
   export CFLAGS="--sysroot=/your/dir/rpi_root -I/your/dir/rpi_root/usr/include/python2.7/"
   ```

2. Build the python wrapper
   ```shell
   cd pyRF24
   ./setup.py build --compiler=crossunix
   ```

3. Make the egg package
   ```shell
   ./setup.py bdist_egg --plat-name=cross
   ```
   `dist/RF24-<version>-cross.egg` should be created.

4. Upload it to the target machine and install there:
   ```shell
    scp dist/RF24-*-cross.egg pi@target_linux_host:
    ssh pi@target_linux_host 'sudo easy_install RF24-*-cross.egg'
    ```
