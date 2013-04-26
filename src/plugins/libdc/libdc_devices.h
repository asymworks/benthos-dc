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

#ifndef LIBDC_DEVICES_H_
#define LIBDC_DEVICES_H_

/**
 * @file src/plugins/libdc/libdc_devices.h
 * @brief libdivecomputer Device Table
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <stdint.h>

#include <libdivecomputer/common.h>
#include <libdivecomputer/descriptor.h>

//! @brief Device Table Entry Type
typedef struct devtable_entry_t_ {
	int32_t			id;			///< Plugin Manifest Model Id
	dc_family_t		family;		///< libdivecomputer Family Id
	unsigned int	model;		///< libdivecomputer Model Id

} devtable_entry_t;

//! @brief Device Table
extern const devtable_entry_t g_libdc_plugin_devtable[];

#endif /* LIBDC_DEVICES_H_ */
