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

#include <string.h>

#include "benthosdc_libdc.h"
#include "libdc_driver.h"
#include "libdc_parser.h"

static const driver_interface_t libdc_driver_interface =
{
	libdc_driver_create,		// driver_create
	libdc_driver_open,			// driver_open
	libdc_driver_close,			// driver_close
	libdc_driver_shutdown,		// driver_shutdown
	libdc_driver_name,			// driver_name
	libdc_driver_errmsg,		// driver_errmsg
	libdc_driver_get_model,		// driver_get_model
	libdc_driver_get_serial,	// driver_get_serial
	libdc_driver_transfer,		// driver_transfer
	libdc_driver_extract,		// driver_extract
	libdc_parser_create,		// parser_create
	libdc_parser_close,			// parser_close
	libdc_parser_reset,			// parser_reset
	libdc_parser_parse_header,	// parser_parse_header
	libdc_parser_parse_profile,	// parser_parse_profile
};

int plugin_load()
{
	return 0;
}

void plugin_unload()
{
}

const driver_interface_t * plugin_load_driver(const char * driver)
{
	if (strcmp(driver, "libdc") != 0)
		return 0;
	return & libdc_driver_interface;
}
