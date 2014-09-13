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

#include <common-smart/smart_parser.h>

#include "benthosdc_smarti.h"
#include "smarti_client.h"
#include "smarti_driver.h"

static const driver_interface_t smarti_driver_interface =
{
	smarti_driver_create,		// driver_create
	smarti_driver_open,			// driver_open
	smarti_driver_close,		// driver_close
	smarti_driver_shutdown,		// driver_shutdown
	smarti_driver_name,			// driver_name
	smarti_driver_errmsg,		// driver_errmsg
	smarti_driver_get_model,	// driver_get_model
	smarti_driver_get_serial,	// driver_get_serial
	smarti_driver_transfer,		// driver_transfer
	smarti_driver_extract,		// driver_extract
	smart_parser_create,		// parser_create
	smart_parser_close,			// parser_close
	smart_parser_reset,			// parser_reset
	smart_parser_parse_header,	// parser_parse_header
	smart_parser_parse_profile,	// parser_parse_profile
};

int plugin_load()
{
	return smarti_client_init();
}

void plugin_unload()
{
	smarti_client_cleanup();
}

const driver_interface_t * plugin_load_driver(const char * driver)
{
	if (strcmp(driver, "smarti") != 0)
		return 0;
	return & smarti_driver_interface;
}
