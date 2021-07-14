#!/usr/bin/env python

import os
from sys import version_info
from setuptools import setup, Extension
import crossunixccompiler

version = ""
cflags = os.getenv("CFLAGS", "")
symlink_directory = []

# NOTE current repo directory structure requires the use of
# `python3 setup.py build` and `python3 setup.py install`
# where `pip3 install ./pyRF24` copies pyRF24 directory to
# `tmp` folder that doesn't have the needed `../Makefile.inc`
# NOTE can't access "../Makefile.inc" from working dir because
# it's relative. Brute force absolute path dynamically.
rf24_git_dir = os.path.split(os.path.abspath(os.getcwd()))[0]

try:  # get compiler options from the generated Makefile.inc
    with open(os.path.join(rf24_git_dir, "Makefile.inc"), "r") as f:
        for line in f.read().splitlines():
            identifier, value = line.split('=', 1)
            if identifier == "CPUFLAGS":
                # this is never reached as CPUFLAGS var is concatenated into CFLAGS var.
                # however the CFLAGS var may conditionally contain "-lwiringPi"
                cflags += " " + value
            elif identifier == "HEADER_DIR":
                cflags += " -I" + os.path.dirname(value)
            elif identifier == "LIB_DIR":
                cflags += " -L" + value
            elif identifier == "LIB_VERSION":
                version = value
            elif identifier in ("CC", "CXX"):
                os.environ[identifier] = value

except FileNotFoundError:  # assuming lib was built & installed with CMake

    # get LIB_VERSION from library.properties file for Arduino IDE
    with open(os.path.join(rf24_git_dir, "library.properties"), "r") as f:
        for line in f.read().splitlines():
            if line.startswith("version"):
                version = line.split("=")[1]

# check C++ RF24 lib is installed
finally:

    # check for possible linker flags set via CFLAGS environment varible
    for flag in cflags.split("-"):
        if flag.startswith("L"):
            symlink_directory.append(
                flag[1:].strip() + (" " if len(symlink_directory) else "")
            )

    if not symlink_directory:
        print("install location of librf24.so isn't explicit; trying default locations.")
        symlink_directory = ["/usr/local/lib", "/usr/lib"]

    found_symlink = False
    for symlink_loc in symlink_directory:
        if os.path.isfile(symlink_loc + "/librf24.so"):
            print("librf24.so found at", symlink_loc)
            found_symlink = True

    if not found_symlink:
        raise FileNotFoundError(
            "RF24 library is not installed. librf24.so does not exist in {}.".format(
                 " or ".join(symlink_directory)
            )
        )

# append any additionally found compiler flags
os.environ["CFLAGS"] = cflags

# get the proper boost.python lib symlink name according to version of python
if version_info >= (3,):
    BOOST_LIB = "boost_python3"
else:
    BOOST_LIB = "boost_python"

crossunixccompiler.register()

setup(
    name="RF24",
    version=version,
    classifiers = [
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 3",
        "Programming Language :: C++",
        "License :: OSI Approved :: GNU General Public License v2 (GPLv2)",
    ],
    ext_modules=[
        Extension(
            "RF24",
            libraries=["rf24", BOOST_LIB],
            sources=["pyRF24.cpp"]
        )
    ]
)
