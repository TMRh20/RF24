#!/usr/bin/env python

from setuptools import setup, Extension
from sys import version_info

if version_info >= (3,):
    BOOST_LIB = "boost_python3"
else:
    BOOST_LIB = "boost_python"

setup(
    name="RF24Network",
    version="1.0",
    classifiers = [
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 3",
        "Programming Language :: C++",
        "License :: OSI Approved :: GNU General Public License v2 (GPLv2)",
    ],
    ext_modules=[
        Extension(
            "RF24Network",
            libraries = ["rf24network", BOOST_LIB],
            sources=["pyRF24Network.cpp"]
        )
    ]
)
