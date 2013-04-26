/*
 * Copyright (C) 2011 Asymworks, LLC.  All Rights Reserved.
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

#ifndef PLUGIN_PLUGIN_H_
#define PLUGIN_PLUGIN_H_

/**
 * @file include/benthos/divecomputer/plugin/plugin.h
 * @brief Plugin Core Interface
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <benthos/divecomputer/plugin/driver.h>
#include <benthos/divecomputer/plugin/parser.h>

/**@{
 * @name Driver Error Codes
 */
#define DRIVER_ERR_SUCCESS		0		///< Operation was Successful
#define DRIVER_ERR_INVALID		-1		///< Invalid Argument
#define DRIVER_ERR_TIMEOUT		-2		///< Operation Timed Out
#define DRIVER_ERR_READ			-3		///< Read Operation Error
#define DRIVER_ERR_WRITE		-4		///< Write Operation Error
#define DRIVER_ERR_NO_DEVICE	-5		///< No Device Present
#define DRIVER_ERR_HANDSHAKE	-6		///< Error in Device Handshake
#define DRIVER_ERR_UNSUPPORTED	-7		///< Unsupported Device or Operation
#define DRIVER_ERR_CANCELLED	-8		///< Operation Cancelled
/*@}*/

/**
 * @brief Driver Interface Structure
 *
 * Contains pointers to the required device driver entry points in a plugin.
 */
typedef struct
{
	plugin_driver_create_fn_t			driver_create;
	plugin_driver_open_fn_t				driver_open;
	plugin_driver_close_fn_t			driver_close;
	plugin_driver_shutdown_fn_t			driver_shutdown;
	plugin_driver_name_fn_t				driver_name;

	plugin_driver_errmsg_fn_t			driver_errmsg;

	plugin_driver_transfer_fn_t			driver_transfer;
	plugin_driver_extract_fn_t			driver_extract;

	plugin_parser_create_fn_t			parser_create;
	plugin_parser_close_fn_t			parser_close;
	plugin_parser_reset_fn_t			parser_reset;

	plugin_parser_parse_header_fn_t		parser_parse_header;
	plugin_parser_parse_profile_fn_t	parser_parse_profile;

} driver_interface_t;

/**
 * @brief Plugin Load Function
 * @return Error value or 0 for success
 *
 * Called once when the plugin is loaded and allows the plugin to perform any
 * necessary initialization.  This function must be named plugin_load.
 */
typedef int (* plugin_load_fn_t)(void);

/**
 * @brief Plugin Unload Function
 *
 * Called once when the plugin is unloaded and allows the plugin to perform any
 * cleanup before exit.  This function must be named plugin_unload.
 */
typedef void (* plugin_unload_fn_t)(void);

/**
 * @brief Load the Driver Function Table
 * @param[in] Driver Name
 * @return Driver Interface Function Table
 *
 * Returns a pointer to the driver function interface table for a driver.  The
 * driver name parameter is provided for plugins which provide multiple drivers
 * within the same shared library, and will be called once for each driver
 * specified in the plugin manifest file.
 *
 * This function must be named plugin_load_driver.
 */
typedef const driver_interface_t * (* plugin_driver_table_fn_t)(const char *);

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_PLUGIN_H_ */
