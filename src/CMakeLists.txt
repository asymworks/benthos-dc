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

# Setup Include Directories
include_directories(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_BINARY_DIR}/include
)

# Build the Common Utility module
add_subdirectory(common-util)

# Build the Smart Device Support module
if(WITH_SMART OR WITH_SMARTI)
  add_subdirectory(common-smart)
endif(WITH_SMART OR WITH_SMARTI)

# Build the IrDA module
if(WITH_IRDA)
  add_subdirectory(common-irda)
endif(WITH_IRDA)

# Build the Library
if(BUILD_SHARED OR BUILD_STATIC)
  add_subdirectory(library)
endif(BUILD_SHARED OR BUILD_STATIC)

# Build the Plugins
if(BUILD_PLUGINS)
  add_subdirectory(plugins)
endif(BUILD_PLUGINS)

# Build the Smart-I Daemon
if(BUILD_SMARTID)
  add_subdirectory(smartid)
endif(BUILD_SMARTID)

# Build the transfer application
if(BUILD_TRANSFER_APP)
  add_subdirectory(transferapp)
endif(BUILD_TRANSFER_APP)