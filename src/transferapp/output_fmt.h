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

#ifndef BENTHOS_DC_OUTPUT_FMT_HPP_
#define BENTHOS_DC_OUTPUT_FMT_HPP_

/**
 * @file src/transferapp/output_fmt.h
 * @brief Dive Output Formatter Common Definitions
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <cstdint>

#include <benthos/divecomputer/manifest.h>
#include <benthos/divecomputer/plugin/parser.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward Definition for output_fmt_data_t_
typedef struct output_fmt_data_t_ * output_fmt_data_t;

//! Data Formatter Close File Function
typedef int (* fmt_data_close_fn_t)(struct output_fmt_data_t_ *);

//! Data Formatter Structure Destructor
typedef void (* fmt_data_dispose_fn_t)(struct output_fmt_data_t_ *);

//! Data Formatter Pre-Parse Function
typedef int (* fmt_data_prolog_fn_t)(struct output_fmt_data_t_ *);

//! Data Formatter Post-Parse Function
typedef int (* fmt_data_epilog_fn_t)(struct output_fmt_data_t_ *);

/**
 * @brief Data Formatter Common Structure
 *
 * Contains the basic information that is passed to parser callbacks in the
 * output formatter.  Includes
 */
struct output_fmt_data_t_
{
	//! Magic Number (should be unique per output formatter)
	uint16_t				magic;

	const char *			driver_name;	///< Driver Name
	const char *			driver_args;	///< Driver Arguments
	const char *			device_path;	///< Device Path

	const driver_info_t *	driver_info;	///< Driver Manifest

	uint8_t					dev_model;		///< Device Model Number
	uint32_t				dev_serial;		///< Device Serial Number

	const char *			output_file;	///< Output File Name
	int						output_header;	///< Output Header Data
	int						output_profile;	///< Output Profile Data

	int						quiet;			///< Output Quietly

	header_callback_fn_t	header_cb;		///< Header Data Callback Function
	waypoint_callback_fn_t	profile_cb;		///< Profile Data Callback Function

	fmt_data_close_fn_t		close_fn;		///< Close File Function
	fmt_data_dispose_fn_t	dispose_fn;		///< Destructor Function
	fmt_data_prolog_fn_t	prolog_fn;		///< Prolog Function
	fmt_data_epilog_fn_t	epilog_fn;		///< Epilog Function

	//! Formatter-Specific Data
	void *					fmt_data;

};

#ifdef __cplusplus
}
#endif

#endif /* BENTHOS_DC_OUTPUT_FMT_HPP_ */
