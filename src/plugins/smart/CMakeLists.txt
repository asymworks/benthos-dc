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

# Build the Uwatec Smart Plugin
add_library(smart SHARED
	benthosdc_smart.c
	smart_driver.c
	smart_io.c
	smart_parser.c
	smart_settings_tec2g.c
	smart_settings.c
	$<TARGET_OBJECTS:common_module>
)

# Link the Uwatec Smart Plugin
target_link_libraries(smart
)

# Package the Uwatec Smart Plugin
install(TARGETS smart
	LIBRARY DESTINATION ${BENTHOS_DC_PLUGINDIR}
	ARCHIVE DESTINATION ${BENTHOS_DC_SPLUGINDIR}
)

# Install Manifest
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/smart.xml DESTINATION ${BENTHOS_DC_DATADIR}/plugins)
