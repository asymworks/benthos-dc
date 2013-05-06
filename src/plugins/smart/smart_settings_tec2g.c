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

#include <common/setting_value.h>

#include "smart_io.h"
#include "smart_settings_tec2g.h"

struct setting_entry_t tec2g_settings_table[] = {
	{ "hw_version",			smart_settings_tec2g_get_hw_version,	0									},
	{ "sw_version",			smart_settings_tec2g_get_sw_version,	0									},
	{ "battery_level",		smart_settings_tec2g_get_batt_level,	0									},
	{ "ambient_pressure",	smart_settings_tec2g_get_pressure,		0									},
	{ "utc_offset",			smart_settings_tec2g_get_utc_offset,	smart_settings_tec2g_set_utc_offset	},
	{ 0, 0, 0 }
};

int smart_settings_tec2g_get_hw_version(dev_handle_t abstract, value_handle_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	uint8_t value;
	int ret = smart_read_uchar(dev, "\x11", & value);
	if (ret != DRIVER_ERR_SUCCESS)
		return ret;

	(* outval) = value_create_uint(value);
	return DRIVER_ERR_SUCCESS;
}

int smart_settings_tec2g_get_sw_version(dev_handle_t abstract, value_handle_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	uint8_t value;
	int ret = smart_read_uchar(dev, "\x13", & value);
	if (ret != DRIVER_ERR_SUCCESS)
		return ret;

	(* outval) = value_create_uint(value);
	return DRIVER_ERR_SUCCESS;
}

int smart_settings_tec2g_get_batt_level(dev_handle_t abstract, value_handle_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	uint8_t value;
	int ret = smart_read_uchar(dev, "\xff\xf5", & value);
	if (ret != DRIVER_ERR_SUCCESS)
		return ret;

	(* outval) = value_create_uint(value);
	return DRIVER_ERR_SUCCESS;
}

int smart_settings_tec2g_get_pressure(dev_handle_t abstract, value_handle_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	uint16_t value;
	int ret = smart_read_ushort(dev, "\xff\xf2", & value);
	if (ret != DRIVER_ERR_SUCCESS)
		return ret;

	(* outval) = value_create_uint(value);
	return DRIVER_ERR_SUCCESS;
}

int smart_settings_tec2g_get_utc_offset(dev_handle_t abstract, value_handle_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	int16_t value;
	int ret = smart_read_sshort(dev, "\xff\x32", & value);
	if (ret != DRIVER_ERR_SUCCESS)
		return ret;

	(* outval) = value_create_int(value);
	return DRIVER_ERR_SUCCESS;
}

int smart_settings_tec2g_set_utc_offset(dev_handle_t abstract, const value_handle_t value_)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	int32_t i;
	int16_t value;
	int ret = value_get_int(value_, & i);
	if (ret != DRIVER_ERR_SUCCESS)
		return DRIVER_ERR_INVALID;

	if ((value < INT16_MIN) || (value > INT16_MAX))
		return DRIVER_ERR_INVALID;

	return smart_write_sshort(dev, "\xff\x52", value);
}
