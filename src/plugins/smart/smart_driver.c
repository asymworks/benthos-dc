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

#include <benthos/divecomputer/arglist.h>

#include <common-smart/smart_device_base.h>
#include <common-smart/smart_extract.h>

#include "smart_driver.h"
#include "smart_io.h"

/* Smart Device Structure Magic Number */
#define SMART_DEV_MAGIC		0x5347

/* Smart Device Structure */
struct smart_device_
{
	/* Must be First Member */
	struct smart_device_base_t	base;		///< Smart Device Base

	irda_t						s;			///< IrDA Socket

	unsigned int				epaddr;		///< IrDA Endpoint Address
	const char *				epname;		///< IrDA Endpoint Name
	const char *				devname;	///< Device Name

	int							lsap;		///< IrDA LSAP Identifier
	unsigned int				csize;		///< IrDA ChunK Size

};

/* Smart-I Device Handle */
typedef struct smart_device_ *		smart_device_t;

/* Check Device Handle and Magic Number */
#define CHECK_DEV(d) (d && (((struct smart_device_base_t *)(d))->magic == SMART_DEV_MAGIC))

void smart_driver_discover_cb(unsigned int address, const char * name, unsigned int charset, unsigned int hints, void * userdata)
{
	smart_device_t dev = (smart_device_t)(userdata);
	if (! CHECK_DEV(dev))
		return;

	if (strncmp (name, "UWATEC Galileo Sol", 18) == 0 ||
			strncmp (name, "Uwatec Smart", 12) == 0 ||
			strstr (name, "Uwatec") != 0 ||
			strstr (name, "UWATEC") != 0 ||
			strstr (name, "Aladin") != 0 ||
			strstr (name, "ALADIN") != 0 ||
			strstr (name, "Smart") != 0 ||
			strstr (name, "SMART") != 0 ||
			strstr (name, "Galileo") != 0 ||
			strstr (name, "GALILEO") != 0)
	{
		dev->epaddr = address;
		dev->epname = strdup(name);
	}
}

time_t smart_driver_epoch()
{
	struct tm t;
	memset(& t, 0, sizeof(t));
	t.tm_year = 100;
	t.tm_mday = 1;

	char * tz_orig = getenv("TZ");
	if (tz_orig)
		tz_orig = strdup(tz_orig);

	setenv("TZ", "UTC", 1);
	tzset();

	time_t epoch = mktime(& t) * 2;

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
	/* Check Magic Number */
	if (! CHECK_DEV(dev))
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	int rc;
	unsigned char cmd1[] = { 0x1b };
	unsigned char cmd2[] = { 0x1c, 0x10, 0x27, 0x00, 0x00 };
	unsigned char ans;

	rc = smart_driver_cmd(dev, cmd1, 1, & ans, 1);
	if (rc != 0)
		return rc;

	if (ans != 0x01)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_HANDSHAKE, "Handshake Phase 1 Failed", 0);
		return DRIVER_ERR_HANDSHAKE;
	}

	rc = smart_driver_cmd(dev, cmd2, 5, & ans, 1);
	if (rc != 0)
		return rc;

	if (ans != 0x01)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_HANDSHAKE, "Handshake Phase 2 Failed", 0);
		return DRIVER_ERR_HANDSHAKE;
	}

	return DRIVER_ERR_SUCCESS;
}

int smart_driver_create(dev_handle_t * abstract)
{
	smart_device_t * dev = (smart_device_t *)(abstract);
	if (! dev)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	smart_device_t sd = (smart_device_t)malloc(sizeof(struct smart_device_));
	if (! sd)
		return DRIVER_ERR_INVALID;

	/* Initialize the Base Structure */
	memset(& sd->base, 0, sizeof(struct smart_device_base_t));

	/* Set the Magic Number */
	sd->base.magic = SMART_DEV_MAGIC;

	/* Initialize the Smart Structure */
	sd->s = 0;
	sd->epaddr = 0;
	sd->epname = 0;
	sd->lsap = 1;
	sd->csize = 4;

	/* Return New Device */
	* dev = sd;

	return DRIVER_ERR_SUCCESS;
}

int smart_driver_open(dev_handle_t abstract, const char * devpath, const char * args)
{
	int rc;
	arglist_t arglist;

	smart_device_t dev = (smart_device_t)(abstract);

	uint32_t chunk_size = 8;
	uint32_t lsap = 1;
	uint32_t irda_timeout = 2000;

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	int rc = arglist_parse(& arglist, args);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_INVALID, "Invalid Argument String", 0);
		return DRIVER_ERR_INVALID;
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
		smart_device_set_error(dev->base, irda_errcode(), irda_errmsg(), 1);
		return DRIVER_ERR_INTERNAL;
	}

	// Set IrDA Socket Timeout
	rc = irda_socket_set_timeout(dev->s, irda_timeout);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, irda_errcode(), irda_errmsg(), 1);
		irda_socket_close(dev->s);
		dev->s = 0;
		return DRIVER_ERR_INTERNAL;
	}

	// Find a Uwatec Smart Device
	int ndevs = irda_socket_discover(dev->s, & smart_driver_discover_cb, dev);
	if (ndevs < 0)
	{
		smart_device_set_error(dev->base, irda_errcode(), irda_errmsg(), 1);
		irda_socket_close(dev->s);
		dev->s = 0;
		return DRIVER_ERR_INTERNAL;
	}

	if (ndevs == 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_NO_DEVICE, "No Uwatec Smart devices found", 0);
		irda_socket_close(dev->s);
		dev->s = 0;
		return DRIVER_ERR_NO_DEVICE;
	}

	// Connect to the first Uwatec Smart Device
	int timeout = 0;
	rc = irda_socket_connect_lsap(dev->s, dev->epaddr, dev->lsap, & timeout);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, irda_errcode(), irda_errmsg(), 1);
		irda_socket_close(dev->s);
		dev->s = 0;
		return DRIVER_ERR_INTERNAL;
	}

	if (timeout)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_TIMEOUT, "Timed out connecting to Uwatec Smart device", 0);
		irda_socket_close(dev->s);
		dev->s = 0;
		return DRIVER_ERR_TIMEOUT;
	}

	// Handshake with the Uwatec Smart Device
	rc = smart_driver_handshake(dev);
	if (rc != 0)
	{
		irda_socket_close(dev->s);
		dev->s = 0;
		return rc;
	}

	// Read Model/Serial/Ticks
	rc = smart_read_uchar(dev, "\x10", & dev->base.model);
	if (rc != 0)
	{
		irda_socket_close(dev->s);
		dev->s = 0;
		return rc;
	}

	rc = smart_read_ulong(dev, "\x14", & dev->base.serial);
	if (rc != 0)
	{
		irda_socket_close(dev->s);
		dev->s = 0;
		return rc;
	}

	rc = smart_read_ulong(dev, "\x1a", & dev->base.ticks);
	if (rc != 0)
	{
		irda_socket_close(dev->s);
		dev->s = 0;
		return rc;
	}

	/* Calculate Time Correction */
	time_t hstime = time(NULL) * 2;
	dev->base.epoch = smarti_driver_epoch();
	dev->base.tcorr = hstime - dev->base.ticks;

	/* Success */
	return DRIVER_ERR_SUCCESS;
}

void smart_driver_close(dev_handle_t abstract)
{
	smart_device_t dev = (smart_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
		return;

	/* Close IrDA Socket */
	if (dev->s != NULL)
		irda_socket_close(dev->s);

	/* Free Endpoint Name String */
	if (dev->epname)
	{
		free(dev->epname);
		dev->epname = 0;
	}
}

void smart_driver_shutdown(dev_handle_t abstract)
{
	smart_device_t dev = (smart_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
		return;

	/* Shutdown IrDA Socket */
	if (dev->s != NULL)
		irda_socket_shutdown(dev->s);

	dev->s = NULL;

	/* Free Error Message */
	if (dev->base.errdyn)
		free((char *)dev->base.errmsg);

	/* Free Handle */
	free(dev);
}

const char * smart_driver_name(dev_handle_t abstract)
{
	return "smart";
}

const char * smart_driver_errmsg(dev_handle_t abstract)
{
	smart_device_t dev = (smart_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
	{
		errno = EINVAL;
		return 0;
	}

	/* Return Error Message */
	return dev->base.errmsg;
}

int smart_driver_get_model(dev_handle_t abstract, uint8_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev) || ! outval)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	* outval = dev->base.model;

	return DRIVER_ERR_SUCCESS;
}

int smart_driver_get_serial(dev_handle_t abstract, uint32_t * outval)
{
	smart_device_t dev = (smart_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev) || ! outval)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	* outval = dev->base.serial;

	return DRIVER_ERR_SUCCESS;
}

int smart_driver_transfer(dev_handle_t abstract, void ** buffer, uint32_t * size, device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata)
{
	smart_device_t dev = (smart_device_t)(abstract);

	char * stoken = 0;
	int free_token = 0;
	uint32_t token = 0;
	int rc;

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
	{
		errno = EINVAL;
		return 0;
	}

	// Send the Device Callback and retrieve the Token
	if (dcb)
	{
		rc = dcb(userdata, dev->base.model, dev->base.serial, dev->base.ticks, & stoken, & free_token);
		if (rc != DRIVER_ERR_SUCCESS)
		{
			smart_device_set_error(dev->base, rc, "Error in Device Callback", 0);
			return rc;
		}

		if (stoken != NULL)
		{
			// Unpack the Token String
			rc = sscanf(stoken, "%u", & token);
			if (rc != 1)
			{
				smart_device_set_error(dev->base, rc, "Invalid Token String", 0);
				return DRIVER_ERR_INVALID;
			}

			// Free the Token Memory
			if (free_token)
				free(stoken);
		}
	}

	// Read the Transfer Length
	unsigned char cmd1[] = { 0xc6, 0, 0, 0, 0, 0x10, 0x27, 0, 0 };
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
		smart_device_set_error(dev->base, ENOMEM, "Unable to allocate transfer data buffer", 0);
		return DRIVER_ERR_INTERNAL;
	}

	unsigned char cmd2[] = { 0xc4, 0, 0, 0, 0, 0x10, 0x27, 0, 0 };
	int timeout = 0;
	int cancel = 0;
	uint32_t len = (* size);
	uint32_t nb;
	uint32_t pos;

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

		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, "Data length mismatch", 0);
		return DRIVER_ERR_INTERNAL;
	}

	// Start Transfer
	if (pcb != NULL)
		pcb(userdata, 0, (* size), & cancel);

	if (cancel)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_CANCELLED, "Operation was cancelled by the user", 0);
		return DRIVER_ERR_CANCELLED;
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
			smart_device_set_error(dev->base, DRIVER_ERR_READ, "Failed to read bytes from the Uwatec Smart device", 0);
			return DRIVER_ERR_READ;
		}

		if (timeout)
		{
			smart_device_set_error(dev->base, DRIVER_ERR_TIMEOUT, "Timed out reading data from the Uwatec Smart device", 0);
			return DRIVER_ERR_TIMEOUT;
		}

		len -= nt;
		pos += nt;

		if (pcb != NULL)
			pcb(userdata, pos, (* size), & cancel);

		if (cancel)
		{
			smart_device_set_error(dev->base, DRIVER_ERR_CANCELLED, "Operation was cancelled by the user", 0);
			return DRIVER_ERR_CANCELLED;
		}
	}

	// Transfer Succeeded
	return 0;
}

int smart_driver_extract(dev_handle_t abstract, void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata)
{
	int rc;
	smart_device_t dev = (smart_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev) || ! cb)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	/* Extract Dives */
	rc = smart_extract_dives(buffer, size, cb, userdata);
	if (rc != EXTRACT_SUCCESS)
	{
		switch (rc)
		{
		case EXTRACT_INVALID:
			errno = EINVAL;
			return DRIVER_ERR_INVALID;

		case EXTRACT_CORRUPT:
			smart_device_set_error(dev->base, DRIVER_ERR_READ, "Invalid or Corrupt Data", 0);
			return DRIVER_ERR_READ;

		case EXTRACT_TOO_SHORT:
			smart_device_set_error(dev->base, DRIVER_ERR_READ, "Dive extends past end of Buffer", 0);
			return DRIVER_ERR_READ;

		case EXTRACT_EXTRA_DATA:
			smart_device_set_error(dev->base, DRIVER_ERR_READ, "Extra data at end of Buffer", 0);
			return DRIVER_ERR_READ;

		}

		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, "Internal Error", 0);
		return DRIVER_ERR_INTERNAL;
	}

	/* Success */
	return DRIVER_ERR_SUCCESS;
}
