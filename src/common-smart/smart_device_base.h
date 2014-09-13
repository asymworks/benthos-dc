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
 * Portions of the Uwatec Smart driver were adapted from libdivecomputer,
 * Copyright (C) 2008 Jef Driesen.  libdivecomputer can be found online at
 * http://www.divesoftware.org/libdc/
 *
 * Also, significant credit is due to Simon Naunton, whose work decoding
 * the Uwatec Smart protocol has been extremely helpful.  His work can be
 * found online at http://diversity.sourceforge.net/uwatec_smart_format.html
 */

#ifndef SMART_DEVICE_BASE_H_
#define SMART_DEVICE_BASE_H_

/**
 * @file src/common-smart/smart_device_base.h
 * @brief Uwatec Smart Device Structure Base
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <stdint.h>
#include <time.h>

/**
 * @brief Uwatec Smart Device Structure
 *
 * Common base definition of a Uwatec Smart device which is shared by the
 * IrDA smart driver and the network Smart-I driver.  These common fields
 * are used by the smart dive parser so it can be shared among the two
 * drivers.
 *
 * When defining the device structure for specific drivers, this should be
 * the first member in the derived structure so it can be directly cast to
 * struct smart_device_base_t.
 */
struct smart_device_base_t
{
	uint16_t		magic;		///< Magic Number

	int				errcode;	///< Last Error Code
	const char *	errmsg;		///< Last Error Message
	int				errdyn;		///< Free Error Message

	time_t			epoch;		///< Epoch in Half-Seconds
	int32_t			tcorr;		///< Time Correction Value

	uint8_t			model;		///< Model Number
	uint32_t		serial;		///< Serial Number
	uint32_t		ticks;		///< Tick Count
};

/**
 * @brief Set the Smart Device Error
 * @param[in] Smart Device Pointer
 * @param[in] Error Code
 * @param[in] Error Message
 * @param[in] Create a Copy of the Message
 *
 * Updates the Smart Device Structure with the given error information, and
 * will optionally copy the given string before storing it.  This has the
 * effect of setting the errdyn flag to 1.  If the errdyn flag is already set
 * to 1, the errmsg string will be freed before assigning the new message.
 *
 * When free()'ing a smart_device_base_t structure, the client should check
 * the errdyn flag and call free() on the errmsg pointer if it is true.
 */
#define smart_device_set_error(dev, err_code, err_msg, err_copy) \
{\
	if ((dev).errdyn) \
		free((char *)(dev).errmsg); \
	(dev).errcode = (err_code); \
	(dev).errdyn = (err_copy); \
	if (err_copy) \
		(dev).errmsg = strdup(err_msg); \
	else \
		(dev).errmsg = (err_msg); \
}

#endif /* SMART_DEVICE_BASE_H_ */
