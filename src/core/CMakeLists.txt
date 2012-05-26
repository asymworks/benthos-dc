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

# C++ Flags
add_definitions( -std=c++0x -fPIC -g )

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON) 
set(Boost_USE_STATIC_RUNTIME OFF)

# Boost Headers Required
find_package( Boost 1.45 REQUIRED COMPONENTS filesystem regex )

# LibXML2 Required
find_package( LibXml2 2.7 REQUIRED )

# Include Paths
include_directories(
	${Boost_INCLUDE_DIR},
	${LIBXML2_INCLUDE_DIR}
)

# Configure Files
configure_file(
	${CMAKE_SOURCE_DIR}/include/benthos/divecomputer/config.hpp.in 
	${CMAKE_BINARY_DIR}/include/benthos/divecomputer/config.hpp 
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/benthos-dc.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/benthos-dc.pc
	@ONLY
)

# Build the Benthos Dive Computer Library
add_library(benthos-dc SHARED
	driver.cpp
	driverclass.cpp
	manifest_parser.cpp
	plugin.cpp
	registry.cpp
)

# Link the Benthos Dive Computer Library
target_link_libraries(benthos-dc
	${Boost_LIBRARIES}
	${LIBXML2_LIBRARIES}
)

# Package the Benthos Dive Computer Library
install(TARGETS benthos-dc
	LIBRARY DESTINATION ${BENTHOS_DC_LIBDIR}
	ARCHIVE DESTINATION ${BENTHOS_DC_LIBDIR}/static
)

# Install Headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include
	PATTERN "*.in" EXCLUDE)
install(FILES ${CMAKE_BINARY_DIR}/include/benthos/divecomputer/config.hpp DESTINATION include/benthos/divecomputer)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/benthos-dc.pc DESTINATION lib/pkgconfig)