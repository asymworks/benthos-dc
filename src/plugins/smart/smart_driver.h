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

#ifndef SMART_DRIVER_H_
#define SMART_DRIVER_H_

/**
 * @file src/plugins/smart/smart_driver.h
 * @brief Uwatec Smart Device Driver
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <common-irda/irda.h>
#include <common-smart/smart_device_base.h>

#include <benthos/divecomputer/plugin/driver.h>
#include <benthos/divecomputer/plugin/plugin.h>

/* Smart Device Structure Magic Number */
#define SMART_DEV_MAGIC		0x5347

/* Smart Device Structure */
struct smart_device_
{
	/* Must be First Member */
	struct smart_device_base_t	base;		///< Smart Device Base

	irda_t						s;			///< IrDA Socket

	unsigned int				epaddr;		///< IrDA Endpoint Address
	char *						epname;		///< IrDA Endpoint Name
	const char *				devname;	///< Device Name

	int							lsap;		///< IrDA LSAP Identifier
	unsigned int				csize;		///< IrDA ChunK Size

};

/* Smart-I Device Handle */
typedef struct smart_device_ *		smart_device_t;

/* Check Device Handle and Magic Number */
#define CHECK_DEV(d) (d && (((struct smart_device_base_t *)(d))->magic == SMART_DEV_MAGIC))

/**@{
 * @name Driver Functions
 */
int smart_driver_create(dev_handle_t * dev);
int smart_driver_open(dev_handle_t dev, const char *, const char * args);
void smart_driver_close(dev_handle_t dev);
void smart_driver_shutdown(dev_handle_t dev);
const char * smart_driver_name(dev_handle_t dev);

const char * smart_driver_errmsg(dev_handle_t dev);

int smart_driver_get_model(dev_handle_t dev, uint8_t * outval);
int smart_driver_get_serial(dev_handle_t dev, uint32_t * outval);

int smart_driver_transfer(dev_handle_t dev, void ** buffer, uint32_t * size, device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata);
int smart_driver_extract(dev_handle_t dev, void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata);
/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* SMART_DRIVER_H_ */
