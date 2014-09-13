#------------------------------------------------------------------------------
# Master CMake File for the Benthos Dive Computer Library (benthos_dc)
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

cmake_minimum_required(VERSION 2.8.8)
project(benthos_dc)

# Dive Computer Library Version
set(BENTHOS_DC_MAJOR_VERSION 0)
set(BENTHOS_DC_MINOR_VERSION 4)
set(BENTHOS_DC_PATCH_VERSION 0)
set(BENTHOS_DC_VERSION
	${BENTHOS_DC_MAJOR_VERSION}.${BENTHOS_DC_MINOR_VERSION}.${BENTHOS_DC_PATCH_VERSION})

set(CMAKE_MODULE_PATH 
	${CMAKE_MODULE_PATH} 
	${CMAKE_SOURCE_DIR}/cmake/modules/
)

# All Warnings
add_definitions( -Wall )

# Feature Summary
include(FeatureSummary)

# OS Detection
set(BDC_OS_LINUX FALSE)
set(BDC_OS_WIN32 FALSE)
set(BDC_OS_MACOS FALSE)
set(BDC_OS_POSIX FALSE)

if(WIN32)
  set(BDC_OS_WIN32 TRUE)
else(WIN32)
  set(BDC_OS_POSIX TRUE)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(BDC_OS_MACOS TRUE)
  endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(BDC_OS_LINUX TRUE)
  endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif(WIN32)

# Linux needs _XOPEN_SOURCE and POSIX 200809 defined
if(BDC_OS_LINUX)
  add_definitions( -D_XOPEN_SOURCE -D_POSIX_C_SOURCE=200809L )
endif(BDC_OS_LINUX)

# Global Compilation Options
option(BUILD_SHARED         "Build benthos-dc shared library"        ON)
option(BUILD_STATIC         "Build benthos-dc static library"        ON)
option(BUILD_PLUGINS        "Build dive computer plugins"            ON)
option(BUILD_SMARTID        "Build Smart-I protocol server"          OFF)
option(BUILD_TRANSFER_APP   "Build dive data transfer application"   ON)

# Plugin Compilation Options
option(WITH_SMARTI          "Build the Smart-I Device plugin"        ON)

# MacOS Specific Compilation Options
if(NOT BDC_OS_MACOS)
  option(WITH_IRDA          "Include IrDA device support"            ON)
  option(WITH_SMART         "Build the Uwatec Smart Device plugin"   ON)
  option(WITH_LIBDC         "Build the libdivecomputer plugin"       ON)
else(NOT BDC_OS_MACOS)
  # IrDA is not available on Mac OS X
  option(WITH_IRDA          "Include IrDA device support"            OFF)
  option(WITH_SMART         "Build the Uwatec Smart Device plugin"   OFF)
  option(WITH_LIBDC         "Build the libdivecomputer plugin"       OFF)
endif(NOT BDC_OS_MACOS)

# Smart Plugin and smartid require IrDA
if(BUILD_SMARTID OR WITH_SMART)
  if(NOT WITH_IRDA)
    message(WARNING "Enabling IrDA support")
    set(WITH_IRDA ON)
  endif(NOT WITH_IRDA)
endif(BUILD_SMARTID OR WITH_SMART)

# Check if IrDA is available
if(WITH_IRDA)
  include(CheckIncludeFiles)
  
  set(HAS_IRDA OFF)
  if(BDC_OS_WIN32)
    CHECK_INCLUDE_FILES("af_irda.h"       HAS_IRDA)
  endif(BDC_OS_WIN32)
  if(BDC_OS_LINUX)
    CHECK_INCLUDE_FILES("linux/irda.h"    HAS_IRDA)
  endif(BDC_OS_LINUX)
  
  if(NOT HAS_IRDA)
    message(FATAL_ERROR "IrDA support is not available on this platform")
  endif(NOT HAS_IRDA)
endif(WITH_IRDA)

# Setup Paths
SET(BENTHOS_DC_DATADIR		"${CMAKE_INSTALL_PREFIX}/share/benthos")
SET(BENTHOS_DC_MANDIR		"${CMAKE_INSTALL_PREFIX}/share/man")
SET(BENTHOS_DC_LIBDIR		"${CMAKE_INSTALL_PREFIX}/lib")
SET(BENTHOS_DC_PLUGINDIR	"${CMAKE_INSTALL_PREFIX}/lib/benthos")
SET(BENTHOS_DC_SPLUGINDIR	"${CMAKE_INSTALL_PREFIX}/lib/static/benthos")

# All source files are in the src directory
add_subdirectory(src)

# Generate documentation in the doc directory
add_subdirectory(doc)

# Show a Summary of Compilation Options
feature_summary(WHAT ALL)

# Install top-level files
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION share/benthos/dc/)
install(FILES ${CMAKE_SOURCE_DIR}/README DESTINATION share/benthos/dc/)
