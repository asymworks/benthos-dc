/*
 * Copyright (C) 2013 Asymworks, LLC.  All Rights Reserved.
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

#ifndef PLUGIN_SETTINGS_H_
#define PLUGIN_SETTINGS_H_

/**
 * @file include/benthos/divecomputer/plugin/settings.h
 * @brief Plugin Dive Computer Settings Interface
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <benthos/divecomputer/plugin/driver.h>

/**
 * @brief Data Types supported by the Settings Module
 */
enum
{
	TYPE_UNKNOWN = -1,			///< Unknown Type
	TYPE_INT = 0,				///< Signed Number
	TYPE_UINT,					///< Unsigned Number
	TYPE_BOOL,					///< Boolean Value
	TYPE_DATETIME,				///< Date/Time Value
	TYPE_ASCII,					///< ASCII String Value
	TYPE_TUPLE,					///< Ordered List of Values
};

/**
 * @brief Opaque Pointer to a Value
 */
typedef struct value_t *		value_handle_t;

/**
 * @brief Read the Dive Computer Time of Day
 * @param[in] Device Handle
 * @param[out] Device Time of Day
 * @return Error value or 0 for success
 *
 * Reads the Time of Day from the Dive Computer as shown on the dive computer
 * translated to GMT.
 */
typedef int (* plugin_setting_get_tod_fn_t)(dev_handle_t, time_t *);

/**
 * @brief Write the Dive Computer Time of Day
 * @param[in] Device Handle
 * @param[in] Device Time of Day
 * @return Error value or 0 for success
 *
 * Writes the Time of Day to the Dive Computer as GMT.
 */
typedef int (* plugin_setting_set_tod_fn_t)(dev_handle_t, time_t);

/**
 * @brief Check if a Dive Computer Setting is available
 * @param[in] Device Handle
 * @param[in] Setting Name
 * @return Boolean Value
 *
 * Checks if a setting is defined for the dive computer.  Return value is zero
 * if the setting is not available; non-zero if it is available.
 */
typedef int (* plugin_setting_has_fn_t)(dev_handle_t, const char *);

/**
 * @brief Read a Dive Computer Setting
 * @param[in] Device Handle
 * @param[in] Setting Name
 * @param[out] Setting Value Pointer
 * @return Error value or 0 for success
 *
 * Reads a setting from the Dive Computer and stores it in the given value
 * object pointer.
 */
typedef int (* plugin_setting_get_fn_t)(dev_handle_t, const char *, value_handle_t *);

/**
 * @brief Write a Dive Computer Setting
 * @param[in] Device Handle
 * @param[in] Setting Name
 * @param[in] Setting Value
 * @return Error value or 0 for success
 *
 * Resets the parser state.  This function must be called in between parsing of
 * successive dives to ensure parser state is clean.
 */
typedef int (* plugin_setting_set_fn_t)(dev_handle_t, const char *, const value_handle_t);

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_SETTINGS_H_ */
