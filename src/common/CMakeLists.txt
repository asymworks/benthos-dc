#------------------------------------------------------------------------------
# CMake File for the Benthos Dive Computer Library (benthos_dc)
#------------------------------------------------------------------------------
#
# Copyright (C) 2011 Asymworks, LLC.  All Rights Reserved.
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

# C Flags
if (NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -std=gnu99")
endif (NOT WIN32)

# Setup for IrDA
include(CheckIncludeFile)
include(CheckFunctionExists)

check_include_file(poll.h HAVE_POLL_H)
check_include_file(sys/poll.h HAVE_SYS_POLL_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)

check_function_exists(poll HAVE_POLL)
check_function_exists(select HAVE_SELECT)

if (NOT WIN32)
    # Win32 always uses select()
    set(HAVE_SELECT)
endif (NOT WIN32)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Build the Common Objects
add_library(common_module OBJECT
	arglist.c
	base64.c
	irda.c
	unpack.c
)