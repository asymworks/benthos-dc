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

#include <benthos/divecomputer/plugin/plugin.h>
#include <common/util.h>

#include "smart_driver.h"
#include "smart_io.h"

int smart_driver_cmd(smart_device_t dev, unsigned char * cmd, ssize_t cmdlen, unsigned char * ans, ssize_t anslen)
{
	int rc;
	int timeout = 0;
	ssize_t _cmdlen = cmdlen;
	ssize_t _anslen = anslen;

	if ((dev == NULL) || (dev->s == NULL))
	{
		BAD_POINTER()
		return -1;
	}

	rc = irda_socket_write(dev->s, cmd, & _cmdlen, & timeout);
	if (rc != 0)
	{
		dev->errcode = DRIVER_ERR_WRITE;
		dev->errmsg = "Failed to write bytes to the Uwatec Smart device";
		return -1;
	}

	if (timeout)
	{
		dev->errcode = DRIVER_ERR_TIMEOUT;
		dev->errmsg = "Timed out writing data to the Uwatec Smart device";
		return -1;
	}

	rc = irda_socket_read(dev->s, ans, & _anslen, & timeout);
	if (rc != 0)
	{
		dev->errcode = DRIVER_ERR_READ;
		dev->errmsg = "Failed to read bytes to the Uwatec Smart device";
		return -1;
	}

	if (timeout)
	{
		dev->errcode = DRIVER_ERR_TIMEOUT;
		dev->errmsg = "Timed out reading data from the Uwatec Smart device";
		return -1;
	}

	return 0;
}

int smart_read_uchar(smart_device_t dev, const char * cmd, uint8_t * ans)
{
	return smart_driver_cmd(dev, (unsigned char *)cmd, strlen(cmd), ans, 1);
}

int smart_read_schar(smart_device_t dev, const char * cmd, int8_t * ans)
{
	return smart_driver_cmd(dev, (unsigned char *)cmd, strlen(cmd), (unsigned char *)ans, 1);
}

int smart_read_ushort(smart_device_t dev, const char * cmd, uint16_t * ans)
{
	return smart_driver_cmd(dev, (unsigned char *)cmd, strlen(cmd), (unsigned char *)ans, 2);
}

int smart_read_sshort(smart_device_t dev, const char * cmd, int16_t * ans)
{
	return smart_driver_cmd(dev, (unsigned char *)cmd, strlen(cmd), (unsigned char *)ans, 2);
}

int smart_read_ulong(smart_device_t dev, const char * cmd, uint32_t * ans)
{
	return smart_driver_cmd(dev, (unsigned char *)cmd, strlen(cmd), (unsigned char *)ans, 4);
}

int smart_read_slong(smart_device_t dev, const char * cmd, int32_t * ans)
{
	return smart_driver_cmd(dev, (unsigned char *)cmd, strlen(cmd), (unsigned char *)ans, 4);
}
