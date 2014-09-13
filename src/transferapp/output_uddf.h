/*
 * Copyright (C) 2011 Asymworks, LLC.  All Rights Reserved.
 *
 * Developed by: Asymworks, LLC <info@asymworks.com>
 * 				 http://www.asymworks.com
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

#ifndef BENTHOS_DC_OUTPUT_UDDF_HPP_
#define BENTHOS_DC_OUTPUT_UDDF_HPP_

/**
 * @file src/transferapp/output_uddf.hpp
 * @brief Universal Dive Data Format Output Formatter
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include "output_fmt.h"

//! Data Formatter Structure Magic Number
#define UDDF_FMT_MAGIC		0x0DDF

/**
 * @brief Initialize a Data Formatter Structure
 * @param[in] Data Formatter Structure Handle
 * @return Zero on Success, Non-Zero on Failure
 */
int uddf_init_formatter(struct output_fmt_data_t_ *);

/**
 * @brief Dive Header Callback Function
 * @param[in] Token Type
 * @param[in] Token Value
 * @param[in] Tank, Mix, or Flag Index
 * @param[in] Vendor Key or Flag Name
 */
void uddf_header_cb(void *, uint8_t, int32_t, uint8_t, const char *);

/**
 * @brief Dive Waypoint Callback Function
 * @param[in] Token Type
 * @param[in] Token Value
 * @param[in] Tank or Mix Index
 * @param[in] Alarm String or Vendor Key Name
 */
void uddf_waypoint_cb(void *, uint8_t, int32_t, uint8_t, const char *);

#endif /* BENTHOS_DC_OUTPUT_UDDF_HPP_ */
