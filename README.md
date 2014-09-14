benthos-dc
==========
Copyright (c) 2011-2014 Asymworks, LLC.  All Rights Reserved.

The Benthos Dive Computer library is a collection of libraries and applications
to interface to scuba diving computer devices.  Similar to [libdivecomputer][libdc],
benthos-dc provides a device-agnostic interface, but benthos-dc is organized as
a set of plugins so that support for new devices may be seamlessly added after
the library and any client programs are installed.

Currently benthos-dc supports [ScubaPro/Uwatec][uwatec] Smart devices, either 
through a local IrDA connection, or over a network using an IrDA connection at 
a remote computer. A plugin which wraps [libdivecomputer][libdc] is also provided.

License
=======

The Benthos Dive Computer Library is licensed under the [Lesser GNU Public License,
version 2.1][lgpl]
Individual Plugins are licensed under the [BSD 3-Clause][bsd] License

Platform Support
================

Benthos supports both Linux and OS X platforms, and Windows support is planned.
Because recent versions of OS X do not include support for IrDA, plugins which
use IrDA are not supported on OS X.

Plugin/Device Support
=====================

Benthos currently includes the following plugins:

| Plugin | Description                                                                                                         | Supported Platforms  |
|--------|---------------------------------------------------------------------------------------------------------------------|----------------------|
| smart  | Interfaces to [ScubaPro/Uwatec][uwatec] Smart devices using IrDA                                                    | Linux, Windows       |
| smarti | Interfaces to [ScubaPro/Uwatec][uwatec] Smart devices on a remote computer running the Smart-I protocol (see below) | Linux, Windows, OS X |
| libdc  | Wrapper around [libdivecomputer][libdc] which supports all devices which libdivecomputer supports natively          | Linux, Windows, OS X |
           
Compiling and Installing Benthos-DC
===================================

Benthos uses the [CMake][cmake] build system.  The recommended method to
compile and install Benthos is to use an out-of-source build.  Starting
in the main source directory, run

	mkdir build-release
	cd build-release
	cmake -DCMAKE_INSTALL_PREFIX=/usr ..
	make && sudo make install
	
Benthos includes the following compilation options that determine what
is compiled.  They are specified in typical CMake option format 

	-D<OPTION>=[ON|OFF]
	
	
| Option             | Description                                    | Default                       |
|--------------------|------------------------------------------------|-------------------------------|
| BUILD_SHARED       | Build the shared benthos dive computer library | ON                            |
| BUILD_STATIC       | Build the static benthos dive computer library | (not implemented)             |
| BUILD_PLUGINS      | Build the plugin shared libraries              | ON                            |
| BUILD_SMARTID      | Build the Smart-I protocol daemon              | OFF                           |
| BUILD_TRANSFER_APP | Build the benthos-xfr transfer application     | ON                            |
| WITH_IRDA          | Include support for IrDA devices               | Linux, Windows: ON, OS X: OFF |
| WITH_SMART         | Build the `smart` plugin                       | Linux, Windows: ON, OS X: OFF |
| WITH_SMARTI        | Build the `smarti` plugin                      | ON                            |
| WITH_LIBDC         | Build the `libdc` plugin                       | OFF                           |

Transfer Application
====================

Benthos includes a small console application which transfers data from a 
dive computer using the benthos-dc library and saves the result in the 
[Universal Dive Data Format][uddf].  It is typically run as

	benthos-xfr -d smart
	
The `-d smart` option specifies which driver to use (in this case, `smart`).
IrDA drivers do not need a device path specified; for drivers which cannot
automatically determine where the device is located, the device path must
be specified.

	benthos-xfr -d smarti smarti-server.local
	
Options may be passed to change the driver behavior.  Driver options are
colon-delimited name-value lists of the form

	key1=val1:key2=val2:key3:val3

The manual page for benthos-xfr lists which options may be passed to drivers
and the effect they will have.

Smart-I Protocol
================

Benthos was originally conceived to communicate with a Uwatec Aladin Tec 2G
dive computer over an IrDA interface; however, when the author changed from
Linux to OS X it was no longer possible to use IrDA natively.  Instead of
constantly running through a virtual machine, Benthos now includes a small
server application which runs on a Linux machine and enables dive computer
communication over the network.

The Smart-I daemon is designed to be built and installed on a small system
such as a [Raspberry Pi][rpi] running the standard Linux IrDA stack.  The
setup using a Raspberry Pi, running Raspbian with 3.12.22 kernel and an
MCS7780 IrDA dongle has been tested and confirmed to work.

The Smart-I daemon includes a System V init file so it may be started as
any normal daemon:

	service smartid start

The default service port for Smart-I is 6740 but it can be overridden 
either at compile time in the CMakeLists.txt file or at runtime on the
command line. 

[lgpl]: http://opensource.org/licenses/lgpl-2.1.php
[bsd]: http://opensource.org/licenses/BSD-3-Clause
[libdc]: http://www.divesoftware.org/libdc/
[uwatec]: http://www.scubapro.com/en-US/USA/instruments/computers.aspx
[cmake]: http://www.cmake.org
[uddf]: http://www.streit.cc/extern/uddf_v310/en/index.html
[rpi]: http://www.raspberrypi.org/
