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

#include <common/arglist.h>
#include <common/util.h>

#include "smart_driver.h"
#include "smart_io.h"

void smart_driver_discover_cb(unsigned int address, const char * name, unsigned int charset, unsigned int hints, void * userdata)
{
	smart_device_t dev = (smart_device_t)(userdata);
	if (dev == NULL)
		return;

	if (strncmp (name, "UWATEC Galileo Sol", 18) == 0 ||
			strncmp (name, "Uwatec Smart", 12) == 0 ||
			strstr (name, "Uwatec") != NULL ||
			strstr (name, "UWATEC") != NULL ||
			strstr (name, "Aladin") != NULL ||
			strstr (name, "ALADIN") != NULL ||
			strstr (name, "Smart") != NULL ||
			strstr (name, "SMART") != NULL ||
			strstr (name, "Galileo") != NULL ||
			strstr (name, "GALILEO") != NULL)
	{
		dev->epaddr = address;
		dev->epname = name;
	}
}

time_t smart_driver_epoch()
{
	struct tm t;
	char * tz_orig;
	time_t epoch;

	memset(& t, 0, sizeof(t));
	t.tm_year = 100;
	t.tm_mday = 1;

	tz_orig = getenv("TZ");
	if (tz_orig)
		tz_orig = strdup(tz_orig);

	setenv("TZ", "UTC", 1);
	tzset();

	epoch = mktime(& t) * 2;

	if (tz_orig)
	{
		setenv("TZ", tz_orig, 1);
		free(tz_orig);
	}
	else
		unsetenv("TZ");

	tzset();

	return epoch;
}

int smart_driver_handshake(smart_device_t dev)
{
	int rc;
	unsigned char cmd1[] = { 0x1b };
	unsigned char cmd2[] = { 0x1c, 0x10, 0x27, 0x00, 0x00 };
	unsigned char ans;

	if ((dev == NULL) || (dev->s == NULL))
	{
		BAD_POINTER()
		return -1;
	}

	rc = smart_driver_cmd(dev, cmd1, 1, & ans, 1);
	if ((rc != 0) || (ans != 0x01))
	{
		dev->errcode = DRIVER_ERR_HANDSHAKE;
		return -1;
	}

	rc = smart_driver_cmd(dev, cmd2, 5, & ans, 1);
	if ((rc != 0) || (ans != 0x01))
	{
		dev->errcode = DRIVER_ERR_HANDSHAKE;
		return -1;
	}

	return 0;
}

int smart_driver_create(dev_handle_t * abstract)
{
	smart_device_t sd;
	smart_device_t * dev = (smart_device_t *)(abstract);
	if (dev == NULL)
	{
		BAD_POINTER()
		return -1;
	}

	sd = (smart_device_t)malloc(sizeof(struct smart_device_));
	if (sd == NULL)
		return -1;

	sd->s = NULL;
	sd->errcode = 0;
	sd->errmsg = NULL;
	sd->epaddr = 0;
	sd->epname = NULL;
	sd->lsap = 1;
	sd->csize = 4;
	sd->tcorr = 0;

	*dev = sd;
	return 0;
}

int smart_driver_open(dev_handle_t abstract, const char * devpath, const char * args)
{
	uint32_t chunk_size = 8;
	uint32_t lsap = 1;
	uint32_t irda_timeout = 2000;

	int ndevs = 0;
	int timeout = 0;
	time_t hstime;

	arglist_t arglist;
	int rc;

	smart_device_t dev = (smart_device_t)(abstract);

	if (dev == NULL)
	{
		BAD_POINTER()
		return -1;
	}

	rc = arglist_parse(& arglist, args);
	if (rc != 0)
	{
		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Invalid Argument String";
		return -1;
	}

	rc = arglist_read_int(arglist, "timeout", & irda_timeout);
	if (rc != 0)
		irda_timeout = 2000;

	rc = arglist_read_uint(arglist, "chunk_size", & chunk_size);
	if (rc != 0)
		chunk_size = 8;

	rc = arglist_read_uint(arglist, "lsap", & lsap);
	if (rc != 0)
		lsap = 1;

	arglist_close(arglist);

	if (chunk_size < 2)
		chunk_size = 2;
	if (chunk_size > 32)
		chunk_size = 32;

	dev->lsap = lsap;
	dev->csize = chunk_size;

	// Open IrDA Socket
	rc = irda_socket_open(& dev->s);
	if (rc != 0)
	{
		dev->errcode = irda_errcode();
		dev->errmsg = irda_errmsg();
		return -1;
	}

	// Set IrDA Socket Timeout
	rc = irda_socket_set_timeout(dev->s, irda_timeout);
	if (rc != 0)
	{
		dev->errcode = irda_errcode();
		dev->errmsg = irda_errmsg();
		irda_socket_close(dev->s);
		dev->s = NULL;
		return -1;
	}

	// Find a Uwatec Smart Device
	ndevs = irda_socket_discover(dev->s, & smart_driver_discover_cb, dev);
	if (ndevs < 0)
	{
		dev->errcode = irda_errcode();
		dev->errmsg = irda_errmsg();
		irda_socket_close(dev->s);
		dev->s = NULL;
		return -1;
	}

	if (ndevs == 0)
	{
		dev->errcode = DRIVER_ERR_NO_DEVICE;
		dev->errmsg = "No Uwatec Smart devices found";
		irda_socket_close(dev->s);
		dev->s = NULL;
		return -1;
	}

	// Connect to the first Uwatec Smart Device
	rc = irda_socket_connect_lsap(dev->s, dev->epaddr, dev->lsap, & timeout);
	if (rc != 0)
	{
		dev->errcode = irda_errcode();
		dev->errmsg = irda_errmsg();
		irda_socket_close(dev->s);
		dev->s = NULL;
		return -1;
	}

	if (timeout)
	{
		dev->errcode = DRIVER_ERR_TIMEOUT;
		dev->errmsg = "Timed out connecting to Uwatec Smart device";
		irda_socket_close(dev->s);
		dev->s = NULL;
		return -1;
	}

	// Handshake with the Uwatec Smart Device
	rc = smart_driver_handshake(dev);
	if (rc != 0)
		return -1;

	// Read Model/Serial/Ticks
	rc = smart_read_uchar(dev, "\x10", & dev->model);
	if (rc != 0)
		return -1;

	rc = smart_read_ulong(dev, "\x14", & dev->serial);
	if (rc != 0)
		return -1;

	rc = smart_read_ulong(dev, "\x1a", & dev->ticks);
	if (rc != 0)
		return -1;

	hstime = time(NULL) * 2;
	dev->epoch = smart_driver_epoch();
	dev->tcorr = hstime - dev->ticks;

	// Device Opened Successfully
	return 0;
}

void smart_driver_close(dev_handle_t abstract)
{
	smart_device_t dev = (smart_device_t)(abstract);

	if (dev == NULL)
		return;

	if (dev->s != NULL)
		irda_socket_close(dev->s);

	free(dev);
}

void smart_driver_shutdown(dev_handle_t abstract)
{
	smart_device_t dev = (smart_device_t)(abstract);

	if (dev == NULL)
		return;

	if (dev->s != NULL)
		irda_socket_shutdown(dev->s);

	dev->s = NULL;
}

const char * smart_driver_name(dev_handle_t abstract)
{
	return "smart";
}

const char * smart_driver_errmsg(dev_handle_t abstract)
{
	smart_device_t dev = (smart_device_t)(abstract);

	if (dev == NULL)
		return NULL;

	return dev->errmsg;
}

int smart_driver_get_model(dev_handle_t abstract, uint8_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	* outval = dev->model;
	return DRIVER_ERR_SUCCESS;
}

int smart_driver_get_serial(dev_handle_t abstract, uint32_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
		return DRIVER_ERR_INVALID;

	* outval = dev->serial;
	return DRIVER_ERR_SUCCESS;
}

int smart_driver_transfer(dev_handle_t abstract, void ** buffer, uint32_t * size, device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata)
{
	static unsigned char cmd1[] = { 0xc6, 0, 0, 0, 0, 0x10, 0x27, 0, 0 };
	static unsigned char cmd2[] = { 0xc4, 0, 0, 0, 0, 0x10, 0x27, 0, 0 };

	char * stoken = 0;
	int free_token = 0;
	uint32_t token = 0;
	int rc;

	int timeout = 0;
	int cancel = 0;
	uint32_t len = (* size);
	uint32_t nb;
	uint32_t pos;

	smart_device_t dev = (smart_device_t)(abstract);
	if (dev == NULL)
	{
		BAD_POINTER();
		return -1;
	}

	// Send the Device Callback and retrieve the Token
	if (dcb != NULL)
	{
		rc = dcb(userdata, dev->model, dev->serial, dev->ticks, & stoken, & free_token);
		if (rc != DRIVER_ERR_SUCCESS)
		{
			dev->errcode = rc;
			dev->errmsg = "Error in Driver Callback";
			return -1;
		}

		if (stoken != NULL)
		{
			// Unpack the Token String
			rc = sscanf(stoken, "%u", & token);
			if (rc != 1)
			{
				dev->errcode = DRIVER_ERR_INVALID;
				dev->errmsg = "Failed to parse Token String";
				return -1;
			}

			// Free the Token Memory
			if (free_token)
				free(stoken);
		}
	}

	// Read the Transfer Length
	* (uint32_t *)(& cmd1[1]) = token;
	rc = smart_driver_cmd(dev, cmd1, 9, (unsigned char *)(size), 4);
	if (rc != 0)
		return -1;

	if (* size == 0)
		return 0;

	// Allocate the Data Buffer
	(* buffer) = malloc(* size);
	if (* buffer == NULL)
	{
		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Unable to allocate transfer data buffer";
		return -1;
	}

	* (uint32_t *)(& cmd2[1]) = token;

	// Begin the Data Transfer
	rc = smart_driver_cmd(dev, cmd2, 9, (unsigned char *)(& nb), 4);
	if (rc != 0)
		return -1;

	if (nb != (* size) + 4)
	{
		free(* buffer);
		(* buffer) = 0;
		(* size) = 0;

		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Data Length Mismatch in smart_driver_transfer";
		return -1;
	}

	// Start Transfer
	if (pcb != NULL)
		pcb(userdata, 0, (* size), & cancel);

	if (cancel)
	{
		dev->errcode = DRIVER_ERR_CANCELLED;
		dev->errmsg = "Operation was cancelled by the user";
		return -1;
	}

	pos = 0;
	while (len > 0)
	{
		ssize_t nt;

		if (len > dev->csize)
			nt = dev->csize;
		else
			nt = len;

		rc = irda_socket_read(dev->s, & ((unsigned char *)(* buffer))[pos], & nt, & timeout);
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

		len -= nt;
		pos += nt;

		if (pcb != NULL)
			pcb(userdata, pos, (* size), & cancel);

		if (cancel)
		{
			dev->errcode = DRIVER_ERR_CANCELLED;
			dev->errmsg = "Operation was cancelled by the user";
			return -1;
		}
	}

	// Transfer Succeeded
	return 0;
}

int smart_driver_extract(dev_handle_t abstract, void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata)
{
	char token[20];
	uint8_t hdr[4] = { 0xa5, 0xa5, 0x5a, 0x5a };
	uint32_t dlen = 0;
	uint32_t tok = 0;
	uint32_t pos = 0;

	unsigned char * data = (unsigned char *)buffer;
	smart_device_t dev = (smart_device_t)(abstract);

	if ((dev == NULL) || (buffer == NULL))
	{
		BAD_POINTER();
		return -1;
	}

	while (pos < size)
	{
		if (strncmp((char *)(& data[pos]), (char *)hdr, 4) != 0)
		{
			dev->errcode = DRIVER_ERR_INVALID;
			dev->errmsg = "Invalid or corrupt data in smart_transfer";
			return -1;
		}

		dlen = * (uint32_t *)(& data[pos + 4]);
		if (pos + dlen > size)
		{
			dev->errcode = DRIVER_ERR_INVALID;
			dev->errmsg = "Length of dive extends past end of received data in smart_transfer";
			return -1;
		}

		tok = * (uint32_t *)(& data[pos + 8]);
		sprintf(token, "%u", tok);

		if (cb != NULL)
			cb(userdata, & data[pos], dlen, token);

		pos += dlen;
	}

	if (pos != size)
	{
		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Found additional bytes at end of received data in smart_transfer";
		return -1;
	}

	return 0;
}
