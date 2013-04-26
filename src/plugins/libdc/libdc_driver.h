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

#ifndef LIBDC_DRIVER_H_
#define LIBDC_DRIVER_H_

/**
 * @file src/plugins/libdc/libdc_driver.h
 * @brief libdivecomputer Device Driver
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>

#include <benthos/divecomputer/plugin/driver.h>
#include <benthos/divecomputer/plugin/plugin.h>

#include <libdivecomputer/common.h>
#include <libdivecomputer/context.h>
#include <libdivecomputer/descriptor.h>
#include <libdivecomputer/device.h>

//! libdivecomputer Device Structure
struct libdc_device_
{
	uint32_t 					modelid;	///< Device Model Id

	dc_context_t *				context;	///< libdivecomputer Error Context

	dc_family_t					family;		///< libdivecomputer Device Family
	unsigned int				model;		///< libdivecomputer Device Model
	dc_descriptor_t *			descriptor;	///< libdivecomputer Device Descriptor

	dc_device_t *				device;		///< libdivecomputer Device Handle
	unsigned int 				devtime;	///< libdivecomputer Device Clock
	dc_ticks_t					systime;	///< libdivecomputer System Clock

	int							errcode;	///< Last Error Code
	const char *				errmsg;		///< Last Error Message

	device_callback_fn_t		dcb;		///< Device Callback Function
	transfer_callback_fn_t 		pcb;		///< Transfer Callback Function
	void *						cb_data;	///< Transfer Callback Data
	int							cancel;		///< Transfer Cancel Flag

	struct dive_list_t_ *		dives;		///< Dive List

};

//! libdivecomputer Device Handle
typedef struct libdc_device_ *	libdc_device_t;

int libdc_driver_create(dev_handle_t * dev);
int libdc_driver_open(dev_handle_t dev, const char *, const char * args);
void libdc_driver_shutdown(dev_handle_t dev);
void libdc_driver_close(dev_handle_t dev);
const char * libdc_driver_name(dev_handle_t dev);

const char * libdc_driver_errmsg(dev_handle_t dev);

int libdc_driver_transfer(dev_handle_t dev, void ** buffer, uint32_t * size, device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata);
int libdc_driver_extract(dev_handle_t dev, void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata);

#ifdef __cplusplus
}
#endif

#endif /* LIBDC_DRIVER_H_ */
