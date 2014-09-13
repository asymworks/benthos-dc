#------------------------------------------------------------------------------
# CMake File for the Benthos Dive Computer Library (benthos_dc)
#------------------------------------------------------------------------------
#
# Copyright (C) 2014 Asymworks, LLC.  All Rights Reserved.
#
# Developed by: Asymworks, LLC <info@asymworks.com>
# 				 http://www.asymworks.com
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal with the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#   1. Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimers.
#   2. Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimers in the
#      documentation and/or other materials provided with the distribution.
#   3. Neither the names of Asymworks, LLC, nor the names of its contributors
#      may be used to endorse or promote products derived from this Software
#      without specific prior written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# WITH THE SOFTWARE.
#

# Application Version Information
set(SMARTID_MAJOR_VERSION 1)
set(SMARTID_MINOR_VERSION 0)
set(SMARTID_PATCH_VERSION 5)
set(SMARTID_VERSION
	${SMARTID_MAJOR_VERSION}.${SMARTID_MINOR_VERSION}.${SMARTID_PATCH_VERSION})

set(SMARTID_VERSION_STRING "${SMARTID_VERSION}")

# Application Name/Copyright Info
set(SMARTID_APP_NAME "smartid")
set(SMARTID_APP_TITLE "Smart-I Daemon")
set(SMARTID_COPYRIGHT "Copyright (c) 2011-2014 Asymworks, LLC.  All Rights Reserved.")

# Application Host Processor Info
execute_process(COMMAND uname -m OUTPUT_VARIABLE SMARTID_MACHINE OUTPUT_STRIP_TRAILING_WHITESPACE)

# Default Listening Port
set(SMARTID_DEFAULT_PORT 6740)

# C/C++ Source Files
set(SMARTID_SRCS
    smartid_cmd.c
    smartid_conn.c
    smartid_device.c
    smartid_logging.c
    smartid_main.c
    smartid_server.c
    $<TARGET_OBJECTS:common_irda>
	$<TARGET_OBJECTS:common_util>
)

# Setup SysV Init Script
configure_file(
    ${CMAKE_SOURCE_DIR}/dist/sysvinit-smartid.in 
    ${CMAKE_BINARY_DIR}/sysvinit-smartid 
    @ONLY)

# Use C99
add_definitions( -std=c99 )

# Find LibEvent
find_package( Event REQUIRED )

# Configure Header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/smartid_version.h.in 
    ${CMAKE_CURRENT_BINARY_DIR}/smartid_version.h 
    @ONLY)

# Setup Include Directories
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_BINARY_DIR}/include
	${EVENT_INCLUDE_DIRS}
)

# Build the Application
add_executable(smartid
	${SMARTID_SRCS}
)

# Link the Application
target_link_libraries(smartid
    ${EVENT_CORE_LIBRARIES}
)

# Install the Application
install(
    TARGETS smartid
    DESTINATION sbin
)

# Install SysV Init File
install(
    FILES ${CMAKE_BINARY_DIR}/sysvinit-smartid
    DESTINATION /etc/init.d 
)
