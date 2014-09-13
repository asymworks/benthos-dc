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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <benthos/divecomputer/arglist.h>

#include <benthos/smarti/smarti_codes.h>

#include <common-smart/smart_device_base.h>
#include <common-smart/smart_extract.h>

#include "smarti_client.h"
#include "smarti_driver.h"

/* Smart-I Device Structure Magic Number */
#define SMARTI_DEV_MAGIC		0x1224

/* Smart-I Device Structure */
struct smarti_device_
{
	/* Must be First Member */
	struct smart_device_base_t	base;		///< Smart Device Base

	smarti_client_t				client;		///< Smart-I Client Handle

	unsigned int				epaddr;		///< Device Address
	char *						epname;		///< Device Name

	int							lsap;		///< Device LSAP Identifier
	unsigned int				csize;		///< Device Chunk Size
};

/* Smart-I Device Handle */
typedef struct smarti_device_ *		smarti_device_t;

/* Check Device Handle and Magic Number */
#define CHECK_DEV(d) (d && (((struct smart_device_base_t *)(d))->magic == SMARTI_DEV_MAGIC))

/* Enumeration Callback */
int smarti_enum_cb(void * userdata, uint32_t address, const char * name)
{
	smarti_device_t dev = (smarti_device_t)(userdata);
	if (! CHECK_DEV(dev))
		return EINVAL;

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
		dev->epname = strdup(name);
	}

	return 0;
}

time_t smarti_driver_epoch()
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

int smarti_driver_create(dev_handle_t * abstract)
{
	int rv;
	smarti_device_t * dev = (smarti_device_t *)(abstract);
	if (dev == NULL)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	smarti_device_t sd = (smarti_device_t)malloc(sizeof(struct smarti_device_));
	if (sd == NULL)
		return DRIVER_ERR_INVALID;

	/* Initialize the Base Structure */
	memset(& sd->base, 0, sizeof(struct smart_device_base_t));

	/* Set the Magic Number */
	sd->base.magic = SMARTI_DEV_MAGIC;

	/* Initialize the Smart-I Structure */
	sd->client = 0;
	sd->epaddr = 0;
	sd->epname = NULL;
	sd->lsap = 1;
	sd->csize = 4;

	/* Create the Smart-I Client Handle */
	rv = smarti_client_alloc(& sd->client);
	if (rv != 0)
	{
		free(sd);
		return DRIVER_ERR_INVALID;
	}

	/* Return New Device */
	* dev = sd;

	return 0;
}

int smarti_driver_open(dev_handle_t abstract, const char * devpath, const char * args)
{
	int rc;
	arglist_t arglist;

	smarti_device_t dev = (smarti_device_t)(abstract);

	uint32_t port = 6740;
	uint32_t lsap = 1;
	uint32_t chunk_size = 8;

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	/* Parse Argument List */
	rc = arglist_parse(& arglist, args);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_INVALID, "Invalid Argument String", 0);
		return DRIVER_ERR_INVALID;
	}

	rc = arglist_read_uint(arglist, "port", & port);
	if (rc != 0)
		port = 6740;

	rc = arglist_read_uint(arglist, "chunk_size", & chunk_size);
	if (rc != 0)
		chunk_size = 8;

	rc = arglist_read_uint(arglist, "lsap", & lsap);
	if (rc != 0)
		lsap = 1;

	arglist_close(arglist);

	/* Connect to the Server in devpath */
	rc = smarti_client_connect(dev->client, devpath, port);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_BAD_DEVPATH, "Failed to Connect to Server", 0);
		return DRIVER_ERR_BAD_DEVPATH;
	}

	/* Enumerate Devices */
	rc = smarti_client_enumerate(dev->client, smarti_enum_cb, dev);
	if (rc != 0)
	{
		if (smarti_client_errcode(dev->client) == SMARTI_STATUS_NO_DEVS)
		{
			smart_device_set_error(dev->base, DRIVER_ERR_NO_DEVICE, "No Devices Found", 0);
			smarti_client_disconnect(dev->client);
			return DRIVER_ERR_NO_DEVICE;
		}
		else
		{
			smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, smarti_client_errmsg(dev->client), 1);
			smarti_client_disconnect(dev->client);
			return DRIVER_ERR_INTERNAL;
		}
	}

	/* Check for a Smart Device */
	if (! dev->epname || ! dev->epaddr)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_NO_DEVICE, "No Smart Devices Found", 0);
		smarti_client_disconnect(dev->client);
		return DRIVER_ERR_NO_DEVICE;
	}

	/* Connect to the Device */
	rc = smarti_client_open(dev->client, dev->epaddr, lsap, chunk_size);
	if (rc != 0)
	{
		switch (smarti_client_errcode(dev->client))
		{
		case ETIMEDOUT:
			smart_device_set_error(dev->base, DRIVER_ERR_TIMEOUT, "Timed Out", 0);
			smarti_client_disconnect(dev->client);
			return DRIVER_ERR_TIMEOUT;

		case SMARTI_ERROR_CONNECT:
		case SMARTI_ERROR_HANDSHAKE:
			smart_device_set_error(dev->base, DRIVER_ERR_HANDSHAKE, "Failed to Connect to Device", 0);
			smarti_client_disconnect(dev->client);
			return DRIVER_ERR_HANDSHAKE;

		case SMARTI_ERROR_IO:
			smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, "I/O Error on Device", 0);
			smarti_client_disconnect(dev->client);
			return DRIVER_ERR_INTERNAL;

		}

		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, smarti_client_errmsg(dev->client), 1);
		smarti_client_disconnect(dev->client);
		return DRIVER_ERR_INTERNAL;
	}

	/* Read Model Number */
	rc = smarti_client_model(dev->client, & dev->base.model);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, smarti_client_errmsg(dev->client), 1);
		smarti_client_disconnect(dev->client);
		return DRIVER_ERR_INTERNAL;
	}

	/* Read Serial Number */
	rc = smarti_client_serial(dev->client, & dev->base.serial);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, smarti_client_errmsg(dev->client), 1);
		smarti_client_disconnect(dev->client);
		return DRIVER_ERR_INTERNAL;
	}

	/* Read Tick Count */
	rc = smarti_client_ticks(dev->client, & dev->base.ticks);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, smarti_client_errmsg(dev->client), 1);
		smarti_client_disconnect(dev->client);
		return DRIVER_ERR_INTERNAL;
	}

	/* Calculate Time Correction */
	time_t hstime = time(NULL) * 2;
	dev->base.epoch = smarti_driver_epoch();
	dev->base.tcorr = hstime - dev->base.ticks;

	/* Success */
	return DRIVER_ERR_SUCCESS;
}

void smarti_driver_close(dev_handle_t abstract)
{
	smarti_device_t dev = (smarti_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
		return;

	/* Close Smart-I Client */
	smarti_client_close(dev->client);
	smarti_client_disconnect(dev->client);

	/* Free Endpoint Name String */
	if (dev->epname)
	{
		free(dev->epname);
		dev->epname = 0;
	}
}

void smarti_driver_shutdown(dev_handle_t abstract)
{
	smarti_device_t dev = (smarti_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
		return;

	/* Cleanup Smart-I Client */
	smarti_client_dispose(dev->client);

	/* Free Error Message */
	if (dev->base.errdyn)
		free((char *)dev->base.errmsg);

	/* Free Handle */
	free(dev);
}

const char * smarti_driver_name(dev_handle_t abstract)
{
	return "smarti";
}

const char * smarti_driver_errmsg(dev_handle_t abstract)
{
	smarti_device_t dev = (smarti_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev))
	{
		errno = EINVAL;
		return 0;
	}

	/* Return Error Message */
	return dev->base.errmsg;
}

int smarti_driver_get_model(dev_handle_t abstract, uint8_t * outval)
{
	smarti_device_t dev = (smarti_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev) || ! outval)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	* outval = dev->base.model;

	return DRIVER_ERR_SUCCESS;
}

int smarti_driver_get_serial(dev_handle_t abstract, uint32_t * outval)
{
	smarti_device_t dev = (smarti_device_t)(abstract);

	/* Check Magic Number */
	if (! CHECK_DEV(dev) || ! outval)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	* outval = dev->base.serial;

	return DRIVER_ERR_SUCCESS;
}

int smarti_driver_transfer(dev_handle_t abstract, void ** buffer, uint32_t * size, device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata)
{
	int rc;

	smarti_device_t dev = (smarti_device_t)(abstract);

	char * stoken = 0;
	int free_token = 0;
	uint32_t token = 0;
	size_t bsize;

	/* Check Magic Number */
	if (! CHECK_DEV(dev) || ! buffer || ! size)
	{
		errno = EINVAL;
		return DRIVER_ERR_INVALID;
	}

	/* Obtain the Transfer Token */
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

	/* Set the Transfer Token */
	rc = smarti_client_set_token(dev->client, token);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, smarti_client_errmsg(dev->client), 1);
		return DRIVER_ERR_INTERNAL;
	}

	/* Get the Transfer Size */
	rc = smarti_client_size(dev->client, size);
	if (rc != 0)
	{
		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, smarti_client_errmsg(dev->client), 1);
		return DRIVER_ERR_INTERNAL;
	}

	/* Check for No Data */
	if ((* size) == 0)
	{
		* buffer = 0;
		return DRIVER_ERR_SUCCESS;
	}

	/* Transfer Data */
	if (pcb != NULL)
		pcb(userdata, 0, (* size), 0);

	rc = smarti_client_xfer(dev->client, buffer, & bsize);
	if (rc != 0)
	{
		if (smarti_client_errcode(dev->client) == SMARTI_ERROR_TIMEOUT)
		{
			smart_device_set_error(dev->base, DRIVER_ERR_TIMEOUT, "Timed Out", 0);
			return DRIVER_ERR_TIMEOUT;
		}
		else
		{
			smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, smarti_client_errmsg(dev->client), 1);
			return DRIVER_ERR_INTERNAL;
		}
	}

	/* Check Data Size */
	if (bsize != (* size))
	{
		smart_device_set_error(dev->base, DRIVER_ERR_INTERNAL, "Data length mismatch", 1);
		return DRIVER_ERR_INTERNAL;
	}

	/* Data Transfer Complete */
	if (pcb != NULL)
		pcb(userdata, bsize, (* size), 0);

	return DRIVER_ERR_SUCCESS;
}

int smarti_driver_extract(dev_handle_t abstract, void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata)
{
	int rc;
	smarti_device_t dev = (smarti_device_t)(abstract);

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
