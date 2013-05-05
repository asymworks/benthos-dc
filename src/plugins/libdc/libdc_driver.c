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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <common/arglist.h>
#include <common/base64.h>

#include <libdivecomputer/common.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>

#include "libdc_devices.h"
#include "libdc_driver.h"

//! Dive Data Singly-Linked List Structure
typedef struct dive_list_t_ {
	size_t					data_length;
	uint8_t *				data;
	size_t					token_length;
	uint8_t *				token;

	struct dive_list_t_ *	next;

} dive_list_t;

void * join_dive_list(dive_list_t * head, uint32_t * size)
{
	size_t ndives = 0;
	dive_list_t * cur = head;
	uint8_t * data = NULL;

	/*
	 * Get the total size of the structure.  Dives are stored as follows:
	 *
	 * size_t data_length
	 * uint8_t data[data_length]
	 * size_t token_length
	 * uint8_t token[token_length]
	 */

	* size = 0;

	while (cur != NULL)
	{
		ndives++;
		* size += 2 * sizeof(size_t);
		* size += cur->data_length;
		* size += cur->token_length;

		cur = cur->next;
	}

	size_t pos = 0;
	data = malloc(* size);

	cur = head;
	while (cur != NULL)
	{
		memcpy(data + pos, & cur->data_length, sizeof(size_t));
		pos += sizeof(size_t);
		memcpy(data + pos, & cur->data, cur->data_length);
		pos += cur->data_length;

		memcpy(data + pos, & cur->token_length, sizeof(size_t));
		pos += sizeof(size_t);
		memcpy(data + pos, & cur->token, cur->token_length);
		pos += cur->token_length;

		cur = cur->next;
	}

	return data;
}

static int libdc_cancel_cb(void * userdata)
{
	libdc_device_t dev = (libdc_device_t)(userdata);
	if (dev == NULL)
		return 0;

	return dev->cancel;
}

static void libdc_event_cb(dc_device_t * device, dc_event_type_t event, const void * data, void * userdata)
{
	libdc_device_t dev = (libdc_device_t)(userdata);
	if (dev == NULL)
		return;

	const dc_event_progress_t * progress = (dc_event_progress_t *)data;
	const dc_event_devinfo_t * devinfo = (dc_event_devinfo_t *)data;
	const dc_event_clock_t * clock = (dc_event_clock_t *)data;

	switch (event)
	{
	case DC_EVENT_PROGRESS:
		if (dev->pcb)
			dev->pcb(dev->cb_data, progress->current, progress->maximum, & dev->cancel);
		break;

	case DC_EVENT_DEVINFO:
		if (dev->dcb)
		{
			// Get the Token
			char * token = NULL;
			int free_token = 0;
			dev->dcb(dev->cb_data, devinfo->model, devinfo->serial, dev->devtime, & token, & free_token);

			if (token != NULL)
			{
				// Base64 Decode the Token
				unsigned char * token_data = NULL;
				size_t token_size = 0;
				token_data = base64_decode(token, strlen(token), & token_size);

				// Set the Token
				if (token_data != NULL)
					dc_device_set_fingerprint(dev->device, token_data, token_size);

				// Free Token Data
				if (free_token)
					free(token);
			}
		}
		break;

	case DC_EVENT_CLOCK:
		dev->devtime = clock->devtime;
		dev->systime = clock->systime;

	default:
		break;
	}
}

static int libdc_dive_cb(const unsigned char * data, unsigned int size, const unsigned char * token, unsigned int tsize, void * userdata)
{
	libdc_device_t dev = (libdc_device_t)(userdata);
	if (dev == NULL)
		return 0;

	dive_list_t * le = malloc(sizeof(dive_list_t));
	le->data_length = size;
	le->data = malloc(size);
	le->token_length = tsize;
	le->token = malloc(tsize);
	le->next = NULL;

	memcpy(le->data, data, size);
	memcpy(le->token, token, tsize);

	if (dev->dives == NULL)
	{
		dev->dives = le;
	}
	else
	{
		le->next = dev->dives;
		dev->dives = le;
	}

	return 1;
}

int libdc_driver_create(dev_handle_t * abstract)
{
	libdc_device_t * dev = (libdc_device_t *)(abstract);
	if (dev == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	dc_context_t * ctx = NULL;
	dc_status_t rc = dc_context_new(& ctx);
	if (rc != DC_STATUS_SUCCESS)
	{
		errno = EINVAL;
		return -1;
	}

	libdc_device_t d = (libdc_device_t)malloc(sizeof(struct libdc_device_));
	if (d == NULL)
		return -1;

	d->modelid = 0;
	d->context = ctx;
	d->family = 0;
	d->model = 0;
	d->descriptor = NULL;
	d->device = NULL;
	d->devtime = 0;
	d->systime = 0;
	d->errcode = 0;
	d->errmsg = NULL;
	d->dcb = NULL;
	d->pcb = NULL;
	d->cb_data = NULL;
	d->cancel = 0;
	d->dives = NULL;

	dc_context_set_loglevel(d->context, DC_LOGLEVEL_NONE);
	dc_context_set_logfunc(d->context, NULL, 0);

	* dev = d;
	return 0;
}

int libdc_driver_open(dev_handle_t abstract, const char * devpath, const char * args)
{
	libdc_device_t dev = (libdc_device_t)(abstract);
	if (dev == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	// Parse the Argument List
	arglist_t arglist;
	int rc = arglist_parse(& arglist, args);
	if (rc != 0)
	{
		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Invalid Argument String";
		return -1;
	}

	// Load a Device Descriptor based on the Model argument
	rc = arglist_read_uint(arglist, "model", & dev->modelid);
	if (rc != 0)
	{
		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Missing 'model' Argument";
		return -1;
	}

	int i, found = 0;
	for (i = 0; g_libdc_plugin_devtable[i].id != -1; ++i)
	{
		if (g_libdc_plugin_devtable[i].id == dev->modelid)
		{
			found = 1;
			dev->family = g_libdc_plugin_devtable[i].family;
			dev->model = g_libdc_plugin_devtable[i].model;
		}
	}

	if (! found)
	{
		dev->errcode = DRIVER_ERR_UNSUPPORTED;
		dev->errmsg = "Unsupported 'model' Argument";
		return -1;
	}

	dc_iterator_t * iterator = NULL;
	dc_descriptor_t * descriptor = NULL;
	dc_descriptor_iterator(& iterator);

	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS)
	{
		if ((dc_descriptor_get_type(descriptor) == dev->family) &&
			(dc_descriptor_get_model(descriptor) == dev->model))
		{
			dev->descriptor = descriptor;
		}
	}

	dc_iterator_free(iterator);

	if (dev->descriptor == NULL)
	{
		dev->errcode = DRIVER_ERR_UNSUPPORTED;
		dev->errmsg = "Could not load descriptor for model";
		return -1;
	}

	// Open the Device
	dc_status_t ret = dc_device_open(& dev->device, dev->context, dev->descriptor, devpath);
	if (ret != DC_STATUS_SUCCESS)
	{
		dev->errcode = DRIVER_ERR_NO_DEVICE;
		dev->errmsg = "Failed to connect to device";
		return -1;
	}

	// Register the Event and Cancel Handlers
	int events = DC_EVENT_PROGRESS | DC_EVENT_DEVINFO | DC_EVENT_CLOCK;

	dc_device_set_events (dev->device, events, libdc_event_cb, dev);
	dc_device_set_cancel (dev->device, libdc_cancel_cb, dev);

	return DRIVER_ERR_SUCCESS;
}

void libdc_driver_shutdown(dev_handle_t abstract)
{
	libdc_device_t dev = (libdc_device_t)(abstract);
	if (dev == NULL)
		return;

	if (dev->device != NULL)
		dc_device_close(dev->device);

	dev->device = NULL;
}

void libdc_driver_close(dev_handle_t abstract)
{
	libdc_device_t dev = (libdc_device_t)(abstract);
	if (dev == NULL)
		return;

	if (dev->device != NULL)
		dc_device_close(dev->device);

	if (dev->context != NULL)
		dc_context_free(dev->context);

	free(dev);
}

const char * libdc_driver_name(dev_handle_t abstract)
{
	return "libdc";
}

const char * libdc_driver_errmsg(dev_handle_t abstract)
{
	libdc_device_t dev = (libdc_device_t)(abstract);

	if (dev == NULL)
		return NULL;

	return dev->errmsg;
}

int libdc_driver_transfer(dev_handle_t abstract, void ** buffer, uint32_t * size, device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata)
{
	libdc_device_t dev = (libdc_device_t)(abstract);
	if (dev == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	// Set the Callback Data
	dev->cb_data = userdata;

	/*
	 * NB: libdc_dive_cb reverses the returned order of dives so that the last dive returned
	 *     holds the next transfer token.  This is per the universal.c program included with
	 *     libdivecomputer which takes the _first_ returned token as the next token.
	 */

	// Transfer and Store the Dives (calls device_callback and transfer_callback internally)
	dc_status_t rc = dc_device_foreach (dev->device, libdc_dive_cb, dev);
	if (rc != DC_STATUS_SUCCESS)
	{
		dev->errcode = DRIVER_ERR_READ;
		dev->errmsg = "Failed to read dive data from device";

		dive_list_t * c = dev->dives;
		while (c != NULL)
		{
			free(c->data);
			free(c->token);
			dive_list_t * n = c->next;
			free(c);
			c = n;
		}

		return -1;
	}

	// Flatten the Dive List (supports higher-level dump function)
	* buffer = join_dive_list(dev->dives, size);

	// Free Dive List Memory
	dive_list_t * c = dev->dives;
	while (c != NULL)
	{
		free(c->data);
		free(c->token);
		dive_list_t * n = c->next;
		free(c);
		c = n;
	}

	return DRIVER_ERR_SUCCESS;
}

int libdc_driver_extract(dev_handle_t abstract, void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata)
{
	libdc_device_t dev = (libdc_device_t)(abstract);
	if (dev == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	unsigned char * dive_data;
	unsigned char * token_data;
	uint32_t pos = 0;
	size_t ndives;
	size_t dlen;
	size_t tlen;

	if (size < sizeof(size_t))
	{
		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Invalid or corrupt data in smart_transfer";
		return -1;
	}

	while (pos < size)
	{
		// Extract the Dive Data from the Buffer
		if (size - pos < sizeof(size_t))
		{
			dev->errcode = DRIVER_ERR_INVALID;
			dev->errmsg = "Length of dive extends past end of received data in libdc_driver_extract";
			return -1;
		}

		memcpy(& dlen, buffer + pos, sizeof(size_t));
		pos += sizeof(size_t);

		if (size - pos < dlen)
		{
			dev->errcode = DRIVER_ERR_INVALID;
			dev->errmsg = "Length of dive extends past end of received data in libdc_driver_extract";
			return -1;
		}

		dive_data = buffer + pos;
		pos += dlen;

		if (size - pos < sizeof(size_t))
		{
			dev->errcode = DRIVER_ERR_INVALID;
			dev->errmsg = "Length of dive extends past end of received data in libdc_driver_extract";
			return -1;
		}

		memcpy(& tlen, buffer + pos, sizeof(size_t));
		pos += sizeof(size_t);

		if (size - pos < tlen)
		{
			dev->errcode = DRIVER_ERR_INVALID;
			dev->errmsg = "Length of dive extends past end of received data in libdc_driver_extract";
			return -1;
		}

		token_data = buffer + pos;
		pos += tlen;

		// Base64-encode the Token
		size_t b64len = 0;
		char * b64token = base64_encode(token_data, tlen, & b64len);

		// Run the Callback
		if (cb != NULL)
			cb(userdata, dive_data, dlen, b64token);
	}

	if (pos != size)
	{
		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Found additional bytes at end of received data in libdc_driver_extract";
		return -1;
	}

	return DRIVER_ERR_UNSUPPORTED;
}
