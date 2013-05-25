#------------------------------------------------------------------------------
# Master CMake File for the Benthos Dive Computer Library (benthos_dc)
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

cmake_minimum_required(VERSION 2.8.8)
project(benthos_dc)

set(BENTHOS_DC_MAJOR_VERSION 0)
set(BENTHOS_DC_MINOR_VERSION 2)
set(BENTHOS_DC_PATCH_VERSION 0)
set(BENTHOS_DC_VERSION
	${BENTHOS_DC_MAJOR_VERSION}.${BENTHOS_DC_MINOR_VERSION}.${BENTHOS_DC_PATCH_VERSION})

set(CMAKE_MODULE_PATH 
	${CMAKE_MODULE_PATH} 
	${CMAKE_SOURCE_DIR}/cmake/modules/
)

SET(BENTHOS_DC_DATADIR		"${CMAKE_INSTALL_PREFIX}/share/benthos")
SET(BENTHOS_DC_MANDIR		"${CMAKE_INSTALL_PREFIX}/share/man")
SET(BENTHOS_DC_LIBDIR		"${CMAKE_INSTALL_PREFIX}/lib")
SET(BENTHOS_DC_PLUGINDIR	"${CMAKE_INSTALL_PREFIX}/lib/benthos")
SET(BENTHOS_DC_SPLUGINDIR	"${CMAKE_INSTALL_PREFIX}/lib/static/benthos")

# Enforce C89 Builds for MSVC 10 Compatibility
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeclaration-after-statement -Werror=declaration-after-statement")

# All source files are in the src directory
add_subdirectory(src)

# Generate documentation in the doc directory
add_subdirectory(doc)

# Install top-level files
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION share/benthos/dc/)
install(FILES ${CMAKE_SOURCE_DIR}/README DESTINATION share/benthos/dc/)
