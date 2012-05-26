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

#ifndef BENTHOSDC_SMART_H_
#define BENTHOSDC_SMART_H_

/**
 * @file src/plugins/smart/benthosdc_smart.h
 * @brief Uwatec Smart Device Driver
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <benthos/divecomputer/plugin/plugin.h>

/**
 * @brief Plugin Load Function
 * @return Error value or 0 for success
 *
 * Called once when the plugin is loaded and allows the plugin to perform any
 * necessary initialization.
 */
int plugin_load(void);

/**
 * @brief Plugin Unload Function
 *
 * Called once when the plugin is unloaded and allows the plugin to perform any
 * cleanup before exit.
 */
void plugin_unload(void);

/**
 * @brief Load the Driver Function Table
 * @param[in] Driver Name
 * @return Driver Interface Function Table
 *
 * Returns a pointer to the driver function interface table for a driver.  The
 * driver name parameter is provided for plugins which provide multiple drivers
 * within the same shared library, and will be called once for each driver
 * specified in the plugin manifest file.
 */
const driver_interface_t * plugin_load_driver(const char *);

#ifdef __cplusplus
}
#endif

#endif /* BENTHOSDC_SMART_H_ */
