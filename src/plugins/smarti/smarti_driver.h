/*
 * Copyright (C) 2014 Asymworks, LLC.  All Rights Reserved.
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

#ifndef SMARTI_DRIVER_H_
#define SMARTI_DRIVER_H_

/**
 * @file src/plugins/smarti/smarti_driver.h
 * @brief Smart-I Device Driver
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <benthos/divecomputer/plugin/driver.h>
#include <benthos/divecomputer/plugin/plugin.h>

/**@{
 * @name Driver Functions
 */
int smarti_driver_create(dev_handle_t * dev);
int smarti_driver_open(dev_handle_t dev, const char *, const char * args);
void smarti_driver_close(dev_handle_t dev);
void smarti_driver_shutdown(dev_handle_t dev);
const char * smarti_driver_name(dev_handle_t dev);

const char * smarti_driver_errmsg(dev_handle_t dev);

int smarti_driver_get_model(dev_handle_t dev, uint8_t * outval);
int smarti_driver_get_serial(dev_handle_t dev, uint32_t * outval);

int smarti_driver_transfer(dev_handle_t dev, void ** buffer, uint32_t * size, device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata);
int smarti_driver_extract(dev_handle_t dev, void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata);
/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* SMART_DRIVER_H_ */
