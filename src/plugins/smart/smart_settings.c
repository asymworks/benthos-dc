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
#include <time.h>

#include <common/unpack.h>

#include "smart_driver.h"
#include "smart_io.h"
#include "smart_settings.h"

int smart_settings_get_tod(dev_handle_t abstract, time_t * out)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

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
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	uint32_t hsticks = 2 * (time - dev->epoch);
	int rc = smart_write_ulong(dev, "\x2a", hsticks);
}
