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

#include <common/irda.h>

#include "benthosdc_smart.h"
#include "smart_driver.h"
#include "smart_parser.h"

static const driver_interface_t smart_driver_interface =
{
	smart_driver_create,		// driver_create
	smart_driver_open,			// driver_open
	smart_driver_close,			// driver_close
	smart_driver_shutdown,		// driver_shutdown
	smart_driver_name,			// driver_name
	smart_driver_errmsg,		// driver_errmsg
	smart_driver_get_model,		// driver_get_model
	smart_driver_get_serial,	// driver_get_serial
	smart_driver_transfer,		// driver_transfer
	smart_driver_extract,		// driver_extract
	smart_parser_create,		// parser_create
	smart_parser_close,			// parser_close
	smart_parser_reset,			// parser_reset
	smart_parser_parse_header,	// parser_parse_header
	smart_parser_parse_profile,	// parser_parse_profile
};

int plugin_load()
{
	return irda_init();
}

void plugin_unload()
{
	irda_cleanup();
}

const driver_interface_t * plugin_load_driver(const char * driver)
{
	if (strcmp(driver, "smart") != 0)
		return 0;
	return & smart_driver_interface;
}
