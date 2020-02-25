# Allolib playground
[![Build Status](https://travis-ci.org/AlloSphere-Research-Group/allolib_playground.svg?branch=master)](https://travis-ci.org/AlloSphere-Research-Group/allolib_playground)

This repo provides a quick and simple way to build allolib applications. It also contains a set of tutorials and cookbook examples in addition to the examples in the allolib library.

## Setup

Get all dependencies and tools required by allolib. See the [readme file for allolib](https://github.com/AlloSphere-Research-Group/allolib/blob/master/readme.md).

Run the init.sh script to get the allolib and al_ext libraries or execute in
a bash shell:

    git submodule update --recursive --init

## Building and running applications

The allolib playground provides basic facilties for building and debugging 
single file applications. On a bash shell on Windows, Linux and OS X do:

    ./run.sh path/to/file.cpp

This will build allolib, and create an executable for the file.cpp called 'file' inside the '''path/to/bin''' directory. It will then run the application.

You can add a file called '''flags.cmake''' in the '''path/to/''' directory which will be added to the build scripts. Here you can add dependencies, include directories, linking and anything else that cmake could be used for. See the example in '''examples/user_flags'''.

For more complex projects follow the template provided in allotemplate
[https://github.com/AlloSphere-Research-Group/allotemplate](). This requires 
some knowledge of Cmake but allows more complex workflows and multifile
applications or multiple target binaries.

