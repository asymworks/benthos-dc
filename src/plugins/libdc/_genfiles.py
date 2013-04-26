#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
_genfiles.py

Generates the backend translation header file and plugin manifest XML for the
LibDiveComputer plugin.  Note that this python file basically just reproduces
the table in the descriptor.c file in LibDiveComputer so it can be linked to
the model ID in the plugin manifest.  As Linus so eloquently put it in the
subsurface source:

/* Christ. Libdivecomputer has the worst configuration system ever. */
"""

import os, sys

SRC_TEMPLATE = u'''/*
 * Copyright (C) 2013 Asymworks, LLC.  All Rights Reserved.
 *
 * Developed by: Asymworks, LLC <info@asymworks.com>
 *                  http://www.asymworks.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimers.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimers in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of Asymworks, LLC, nor the names of its contributors
 *      may be used to endorse or promote products derived from this Software
 *      without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

/*
 * This file is generated automatically by src/plugins/libdc/_genfiles.py
 */

#include "libdc_devices.h"

const devtable_entry_t g_libdc_plugin_devtable[] = {
%(devices)s
    { -1, 0, 0 }
};

'''

XML_TEMPLATE = '''<?xml version="1.0" encoding="UTF-8" ?>
<!-- This file is generated automatically by src/plugins/libdc/_genfiles.py -->
<plugin name="benthosdc-libdc" library="libdc" version="1.0">
    <driver name="libdc">
        <description>libdivecomputer Driver</description>
        <interface>serial</interface>
        <parameters>
            <parameter name="model" type="model">Dive Computer Model</parameter>
        </parameters>
        <models>
%(devices)s
        </models>
    </driver>
</plugin>
'''

if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.stderr.write("Usage: " + sys.argv[0] + " [CFILE] [XMLFILE]\n")
        exit(1)
    
    src_devs = []
    xml_devs = []
    
    id = 1
    lines = sys.stdin.readlines()
    devs = []
    for l in lines:
        devs.append(l.strip().split(','))
        
    # Sort by Vendor
    devs.sort(key=lambda k: (k[1], k[2]))
        
    for f, v, n, m in devs:
        src_devs.append('\t{%u, %s, %s},' % (id, f, m))
        xml_devs.append('\t\t\t<model id="%u" manufacturer="%s">%s %s</model>' % (id, v, v, n))
        
        id = id + 1
    
    src_file = open(sys.argv[1], 'w')
    src_file.write(SRC_TEMPLATE % {'devices': '\n'.join(src_devs)})
    src_file.close()
    
    xml_file = open(sys.argv[2], 'w')
    xml_file.write(XML_TEMPLATE % {'devices': '\n'.join(xml_devs)})
    xml_file.close()
    
    exit(0)