/*
 * Copyright (C) 2012 Asymworks, LLC.  All Rights Reserved.
 * www.asymworks.com / info@asymworks.com
 *
 * This file is part of the Benthos Dive Log Package (benthos-log.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/*
 * Portions of the Uwatec Smart parser were adapted from libdivecomputer,
 * Copyright (C) 2008 Jef Driesen.  libdivecomputer can be found online at
 * http://www.divesoftware.org/libdc/
 *
 * Also, significant credit is due to Simon Naunton, whose work decoding
 * the Uwatec Smart protocol has been extremely helpful.  His work can be
 * found online at http://diversity.sourceforge.net/uwatec_smart_format.html
 */

#ifndef SMART_EXTRACT_H_
#define SMART_EXTRACT_H_

/**
 * @file src/common-smart/smart_extract.h
 * @brief Uwatec Smart Dive Extraction Function
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <benthos/divecomputer/plugin/driver.h>

/**@{
 * @name Result Codes for Extract Dives
 */
#define EXTRACT_SUCCESS			0		///< Success
#define EXTRACT_INVALID			1		///< Invalid Argument
#define EXTRACT_CORRUPT			2		///< Invalid/Corrupt Data
#define EXTRACT_TOO_SHORT		3		///< Length Extends Past End of Buffer
#define EXTRACT_EXTRA_DATA		4		///< Extra Data at End of Buffer
/*@}*/

/**
 * @brief Extract Dives from a Transferred Data Buffer
 * @param[in] Dive Data Buffer Pointer
 * @param[in] Dive Data Buffer Size
 * @param[in] Dive Callback Function
 * @param[in] Dive Callback Data
 *
 * Splits the data buffer into individual dives and calls the callback function
 * for each extracted dive.
 */
int smart_extract_dives(void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata);

#ifdef __cplusplus
}
#endif

#endif /* SMART_EXTRACT_H_ */
