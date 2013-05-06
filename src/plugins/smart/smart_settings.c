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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <common/unpack.h>

#include "smart_driver.h"
#include "smart_io.h"
#include "smart_settings.h"

#include "smart_settings_tec2g.h"

typedef struct table_entry_
{
	uint8_t						model;
	struct setting_entry_t *	table;

} table_entry;

table_entry setting_tables[] = {
	{ 19,	tec2g_settings_table },		// Tec 2G
	{ 0, 	0 },
};

int smart_settings_get_tod(dev_handle_t abstract, time_t * out)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	uint32_t ticks;
	int ret = smart_read_ulong(dev, "\x1a", & ticks);
	if (ret != 0)
		return ret;

	// Return Timestamp
	* out = dev->epoch + (ticks / 2);
	return 0;
}

int smart_settings_set_tod(dev_handle_t abstract, time_t time)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	uint32_t hsticks = 2 * (time - dev->epoch);
	int rc = smart_write_ulong(dev, "\x2a", hsticks);
}

struct setting_entry_t * smart_find_setting(smart_device_t dev, const char * name)
{
	if ((dev == NULL) || (! dev->settings))
		return 0;

	int i;
	for (i = 0; i < 256; ++i)
	{
		if (! dev->settings[i].name)
			return 0;

		if (! strcmp(dev->settings[i].name, name))
			return & dev->settings[i];
	}

	return 0;
}

int smart_settings_has(dev_handle_t abstract, const char * name)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	return (smart_find_setting(dev, name) != 0);
}

int smart_settings_get(dev_handle_t abstract, const char * name, value_handle_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	struct setting_entry_t * s = smart_find_setting(dev, name);
	if (! s)
		return DRIVER_ERR_UNSUPPORTED;

	if (! s->getter)
		return DRIVER_ERR_WRITEONLY;

	return s->getter(abstract, outval);
}

int smart_settings_set(dev_handle_t abstract, const char * name, const value_handle_t value)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	struct setting_entry_t * s = smart_find_setting(dev, name);
	if (! s)
		return DRIVER_ERR_UNSUPPORTED;

	if (! s->setter)
		return DRIVER_ERR_READONLY;

	return s->setter(abstract, value);
}

struct setting_entry_t * smart_settings_table(uint8_t model)
{
	int i;
	for (i = 0; i < 256; ++i)
	{
		if (! setting_tables[i].model && ! setting_tables[i].table)
			return 0;

		if (setting_tables[i].model == model)
			return setting_tables[i].table;
	}

	return 0;
}
