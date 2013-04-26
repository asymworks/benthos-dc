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

#ifndef PLUGIN_PARSER_H_
#define PLUGIN_PARSER_H_

/**
 * @file include/benthos/divecomputer/plugin/parser.h
 * @brief Plugin Parser Interface
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Opaque Pointer to a Device parser Instance
 */
typedef struct parser_t *		parser_handle_t;

/**
 * @brief Dive Header Tokens
 */
enum
{
	DIVE_HEADER_START_TIME,			///< Dive Start Time (UTC, time_t)
	DIVE_HEADER_UTC_OFFSET,			///< UTC Offset (minutes)
	DIVE_HEADER_DURATION,			///< Dive Duration (minutes)
	DIVE_HEADER_INTERVAL,			///< Surface Interval (minutes)
	DIVE_HEADER_REPETITION,			///< Repetition Number
	DIVE_HEADER_DESAT_BEFORE,		///< Desaturation Time Before Dive (minutes)
	DIVE_HEADER_DESAT_AFTER,		///< Desaturation Time After Dive (minutes)
	DIVE_HEADER_NOFLY_BEFORE,		///< No-Fly Time Before Dive (minutes)
	DIVE_HEADER_NOFLY_AFTER,		///< No-Fly Time After Dive (minutes)
	DIVE_HEADER_MAX_DEPTH,			///< Maximum Depth (centimeters)
	DIVE_HEADER_AVG_DEPTH,			///< Average Depth (centimeters)
	DIVE_HEADER_MIN_TEMP,			///< Minimum Temperature (centidegrees Celsius)
	DIVE_HEADER_MAX_TEMP,			///< Maximum Temperature (centidegrees Celsius)
	DIVE_HEADER_AIR_TEMP,			///< Air Temperature (centidegrees Celsius)
	DIVE_HEADER_PX_START,			///< Starting Pressure (mbar)
	DIVE_HEADER_PX_END,				///< Ending Pressure (mbar)
	DIVE_HEADER_PMO2,				///< Gas Mixture O2 concentration (permille)
	DIVE_HEADER_PMHe,				///< Gas Mixture He concentration (permille)
	DIVE_HEADER_VENDOR,				///< Vendor-Defined Data
	DIVE_HEADER_TOKEN,				///< Dive Computer Token
};

/**
 * @brief Dive Profile Waypoint Tokens
 */
enum
{
	DIVE_WAYPOINT_TIME,				///< Time (seconds)
	DIVE_WAYPOINT_DEPTH,			///< Depth (centimeters)
	DIVE_WAYPOINT_TEMP,				///< Temperature (centidegrees Celsius)
	DIVE_WAYPOINT_PX,				///< Tank Pressure (mbar)
	DIVE_WAYPOINT_MIX,				///< Current Mix (index)
	DIVE_WAYPOINT_TANK,				///< Current Tank (index)
	DIVE_WAYPOINT_ALARM,			///< Alarm
	DIVE_WAYPOINT_RBT,				///< Remaining Bottom Time (minutes)
	DIVE_WAYPOINT_NDL,				///< No-Decompression Limit (minutes)
	DIVE_WAYPOINT_HEARTRATE,		///< Heart Rate (bpm)
	DIVE_WAYPOINT_BEARING,			///< Bearing (degrees)
	DIVE_WAYPOINT_HEADING,			///< Heading (degrees)
	DIVE_WAYPOINT_VENDOR,			///< Vendor-Defined Data
	DIVE_WAYPOINT_FLAG,				///< Vendor-Defined Flag
};

/**
 * @brief Dive Header Callback Function
 * @param[in] Token Type
 * @param[in] Token Value
 * @param[in] Tank, Mix, or Flag Index
 * @param[in] Vendor Key or Flag Name
 */
typedef void (* header_callback_fn_t)(void *, uint8_t, int32_t, uint8_t, const char *);

/**
 * @brief Dive Waypoint Callback Function
 * @param[in] Token Type
 * @param[in] Token Value
 * @param[in] Tank or Mix Index
 * @param[in] Alarm String or Vendor Key Name
 */
typedef void (* waypoint_callback_fn_t)(void *, uint8_t, int32_t, uint8_t, const char *);

/**
 * @brief Create a Parser for the Transferred Data
 * @param[out] Pointer to Parser Handle
 * @param[in] Device Handle
 * @return Error value or 0 for success
 *
 * Creates a new parser instance for the given device.  If the return value
 * indicates an error, the device error data will be set and may be accessed
 * via the plugin_driver_errmsg_fn_t slot.
 */
typedef int (* plugin_parser_create_fn_t)(parser_handle_t *, dev_handle_t);

/**
 * @brief Close a Parser Instance
 * @param[in] Parser Handle
 */
typedef void (* plugin_parser_close_fn_t)(parser_handle_t);

/**
 * @brief Reset the Parser Instance
 * @param[in] Parser Handle
 * @return Error value or 0 for success
 *
 * Resets the parser state.  This function must be called in between parsing of
 * successive dives to ensure parser state is clean.
 */
typedef int (* plugin_parser_reset_fn_t)(parser_handle_t);

/**
 * @brief Parse the Dive Header Data
 * @param[in] Parser Handle
 * @param[in] Data Buffer Pointer
 * @param[in] Data Buffer Size
 * @param[in] Callback Function
 * @param[in] User Data
 *
 * Parses the dive header and calls the callback function once for each header
 * value found.
 */
typedef int (* plugin_parser_parse_header_fn_t)(parser_handle_t, const void *, uint32_t, header_callback_fn_t, void *);

/**
 * @brief Parse the Dive Profile Data
 * @param[in] Parser Handle
 * @param[in] Data Buffer Pointer
 * @param[in] Data Buffer Size
 * @param[in] Callback Function
 * @param[in] User Data
 *
 * Parses the dive profile and calls the callback function for each data value
 * in each waypoint found.  Note that a DIVE_WAYPOINT_TIME token is used to
 * indicate that a new waypoint has started; other tokens are assumed to belong
 * to the same waypoint.
 */
typedef int (* plugin_parser_parse_profile_fn_t)(parser_handle_t, const void *, uint32_t, waypoint_callback_fn_t, void *);

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_PARSER_H_ */
