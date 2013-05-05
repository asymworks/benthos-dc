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

# Enable Warnings, C++0x, etc.
add_definitions( -std=c++0x -Wall -g )

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON) 
set(Boost_USE_STATIC_RUNTIME OFF)

# Boost Headers Required
find_package( Boost 1.45 REQUIRED COMPONENTS filesystem program_options system )

# LibXML2 Required
find_package( LibXml2 2.7 REQUIRED )

# Include Paths
include_directories(
	${Boost_INCLUDE_DIR},
	${LIBXML2_INCLUDE_DIR}
)

# Build Transfer Application
add_executable( benthos-xfr 
	main.cpp
)

target_link_libraries( benthos-xfr 
	${Boost_LIBRARIES}
	${LIBXML2_LIBRARIES}
	benthos-dc
)

# Create the man page
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/benthos-xfr.1.man.in
	${CMAKE_CURRENT_BINARY_DIR}/benthos-xfr.1.man
	@ONLY
)

add_custom_command(
	TARGET benthos-xfr POST_BUILD
	COMMAND gzip -c benthos-xfr.1.man > benthos-xfr.1.gz
	DEPENDS benthos-xfr.1.man
	COMMENT "Generating man page benthos-xfr.1..."
)

# Install Transfer Application
install(TARGETS benthos-xfr
	DESTINATION bin
)

# Install man page
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/benthos-xfr.1.gz 
	DESTINATION ${BENTHOS_DC_MANDIR}/man1
)	
