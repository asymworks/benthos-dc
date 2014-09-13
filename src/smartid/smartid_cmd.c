/*
 * Copyright (C) 2014 Asymworks, LLC.  All Rights Reserved.
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>						// strncasecmp

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>

#include "smarti_codes.h"
#include "smartid_conn.h"
#include "smartid_cmd.h"
#include "smartid_logging.h"

//! Smart-I Command Structure
struct smarti_cmd_t
{
	const char *		name;				///< Command Name
	const char *		desc;				///< Command Description

	/**
	 * Command Callback Function
	 * @param[in] Smart-I Connection Structure
	 * @param[in] Smart-I Command Structure
	 * @param[in] Command Parameters
	 */
	void (* func)(smarti_conn_t, struct smarti_cmd_t *, const char *);
};

/**@{
 * @name Smart-I Command Handlers
 */
static void smartid_cmd_close(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_enum(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_help(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_model(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_open(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_serial(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_size(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_tcorr(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_ticks(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_token(smarti_conn_t, struct smarti_cmd_t *, const char *);
static void smartid_cmd_xfer(smarti_conn_t, struct smarti_cmd_t *, const char *);
/*@}*/

//! Smart-I Command Table
static struct smarti_cmd_t smarti_cmd_table[] =
{
	{"close",	"Close the current IrDA connection",					smartid_cmd_close},
	{"enum",	"Print the list of IrDA devices found on the bus",		smartid_cmd_enum},
	{"help",	"Print the list of commands and their descriptions",	smartid_cmd_help},
	{"model",	"Print the model name of the open device",				smartid_cmd_model},
	{"open",	"Open a connection to an IrDA device",					smartid_cmd_open},
	{"serial",	"Print the serial number of the open device",			smartid_cmd_serial},
	{"size",	"Print the size of the next transfer in bytes",			smartid_cmd_size},
	{"tcorr",	"Print the time correction value of the open device",	smartid_cmd_tcorr},
	{"ticks",	"Print the time tick value of the open device",			smartid_cmd_ticks},
	{"token",	"Set the token for the next transfer operation",		smartid_cmd_token},
	{"xfer",	"Transfer data from the device",						smartid_cmd_xfer},
	{0,			0,														0}
};

//! IrDA Device Info List Entry
struct irda_dev_info_t
{
	unsigned int				addr;
	unsigned int				charset;
	unsigned int				hints;
	char *						name;

	struct irda_dev_info_t *	next;
};

// IrDA Enumeration Callback
void irda_enum_cb(unsigned int addr, const char * name, unsigned int charset, unsigned int hints, void * arg)
{
	struct irda_dev_info_t * dev;
	struct irda_dev_info_t ** plist = (struct irda_dev_info_t **)arg;

	if (! arg)
		return;

	/* Fill in Device Info */
	dev = (struct irda_dev_info_t *)malloc(sizeof(struct irda_dev_info_t));
	if (! dev)
	{
		smartid_log_error("Failed to allocate struct irda_dev_info_t");
		return;
	}

	smartid_log_debug("Found device '%s' at %lu", name, addr);

	dev->addr = addr;
	dev->name = strdup(name);
	dev->charset = charset;
	dev->hints = hints;

	/* Insert as List Head */
	dev->next = (* plist);
	(* plist) = dev;
}

void smartid_cmd_process(smarti_conn_t c, const char * cmd_line, size_t cmd_len)
{
	size_t name_len;
	int i;
	struct smarti_cmd_t * cmd;
	char * p;
	char * name;
	char * params;

	/* Separate Command Name from Parameters */
	cmd_line += strspn(cmd_line, " \t");
	name_len  = strcspn(cmd_line, " \t");

	if (name_len == 0)
	{
		/* No Command Received */
		smartid_conn_send_ready(c);
		return;
	}
	else if (name_len == strlen(cmd_line))
	{
		/* No Parameters Received */
		name = strndup(cmd_line, name_len);
		params = 0;
	}
	else
	{
		/* Split Command and Parameters */
		name = strndup(cmd_line, name_len);
		params = strdup(cmd_line + name_len + 1);
	}

	/* Lower-Case the Command */
	for (p = name ; *p; ++p) *p = tolower(*p);

	/* Process Command */
	smartid_log_debug("Received command '%s'", name);

	for (i = 0; smarti_cmd_table[i].name; i++)
	{
		cmd = & smarti_cmd_table[i];

		if (strncasecmp(name, cmd->name, name_len) == 0)
		{
			smartid_log_debug("Running command '%s'", name);
			cmd->func(c, cmd, params);
			break;
		}
	}

	if (! cmd->name)
	{
		smartid_log_warning("Received unknown command '%s' from %s", name, smartid_conn_client(c));
		smartid_conn_send_responsef(c, SMARTI_ERROR_UNKNOWN_COMMAND, "Unknown Command '%s'", cmd);
	}
}

#define CHECK_CMD \
	if (! c || ! cmd) \
	{ \
		smartid_log_error("Invalid pointer passed to %s", __func__); \
		return; \
	}

#define CHECK_NO_PARAMS \
	if (params) \
	{ \
		smartid_conn_send_responsef(c, SMARTI_ERROR_INVALID_COMMAND, "Syntax error in %s", cmd->name); \
		smartid_log_warning("Syntax error in command '%s': unexpected parameters", cmd->name); \
		return; \
	}

#define CHECK_PARAMS \
	if (! params) \
	{ \
		smartid_conn_send_responsef(c, SMARTI_ERROR_INVALID_COMMAND, "Syntax error in %s", cmd->name); \
		smartid_log_warning("Syntax error in command '%s': missing parameter", cmd->name); \
		return; \
	}

#define CHECK_DEVICE \
	if (! smartid_conn_device(c)) \
	{ \
		smartid_conn_send_responsef(c, SMARTI_ERROR_NO_DEVICE, "No device is open"); \
		smartid_log_warning("Received '%s' without active device", cmd->name); \
		return; \
	}

static void smartid_cmd_close(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	CHECK_CMD
	CHECK_NO_PARAMS
	CHECK_DEVICE

	smartid_dev_dispose(smartid_conn_device(c));
	smartid_conn_set_device(c, 0);

	smartid_log_info("Disconnected from device [client: %s]", smartid_conn_client(c));

	smartid_conn_send_response(c, SMARTI_STATUS_SUCCESS, "Device Closed");
}

static void smartid_cmd_enum(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int nd;
	irda_t s;
	struct irda_dev_info_t * list = 0;
	struct irda_dev_info_t * it = 0;

	CHECK_CMD
	CHECK_NO_PARAMS

	s = smartid_conn_irda(c);
	if (! s)
	{
		smartid_conn_send_response(c, SMARTI_ERROR_IRDA, "IrDA subsystem error");
		smartid_log_warning("No IrDA socket open in smartid_cmd_enum()");
		return;
	}

	/* Enumerate Devices */
	nd = irda_socket_discover(s, irda_enum_cb, & list);
	if (nd < 0)
	{
		smartid_conn_send_response(c, SMARTI_ERROR_IRDA, "IrDA subsystem error");
		smartid_log_warning("No IrDA socket open in smartid_cmd_enum()");
		return;
	}

	/* No Devices Found */
	if (nd == 0)
	{
		smartid_conn_send_response(c, SMARTI_STATUS_NO_DEVS, "No Devices");
		smartid_log_debug("No IrDA devices found for ENUM command");
		return;
	}

	/* Generate Return Information */
	for (it = list; it; it = it->next)
	{
		smartid_conn_send_responsef(c, SMARTI_STATUS_INFO, "Device: %lu %s", it->addr, it->name);
	}

	/* Free Connection List */
	while (list)
	{
		it = list->next;
		if (list->name)
			free(list->name);
		free(list);
		list = it;
	}
}

static void smartid_cmd_help(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int i;

	CHECK_CMD
	CHECK_NO_PARAMS

	for (i = 0; smarti_cmd_table[i].name; i++)
	{
		char buf[251];
		snprintf(buf, 250, "%s\t%s", smarti_cmd_table[i].name, smarti_cmd_table[i].desc);
		smartid_conn_send_response(c, SMARTI_STATUS_INFO, buf);
	}

}

static void smartid_cmd_model(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int rv;
	uint8_t model;

	CHECK_CMD
	CHECK_NO_PARAMS
	CHECK_DEVICE

	rv = smartid_dev_model(smartid_conn_device(c), & model);
	if (rv != 0)
	{
		switch (rv)
		{
		case SMARTI_ERROR_TIMEOUT:
			smartid_conn_send_response(c, rv, "Timeout on Smart Device");
			break;

		case SMARTI_ERROR_IO:
			smartid_conn_send_response(c, rv, "Device I/O error");
			break;

		default:
			smartid_conn_send_responsef(c, SMARTI_ERROR_INTERNAL, "Internal Error (code %d)", rv);

		}

		smartid_dev_dispose(smartid_conn_device(c));
		smartid_conn_set_device(c, 0);
		return;
	}

	smartid_conn_send_responsef(c, SMARTI_STATUS_INFO, "Model Number: %u", model);
}

static void smartid_cmd_open(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int rv;
	unsigned int addr;
	unsigned int lsap = 1;
	size_t chunk_size = 8;
	smart_device_t dev = 0;

	CHECK_CMD
	CHECK_PARAMS

	if (smartid_conn_device(c))
	{
		smartid_conn_send_responsef(c, SMARTI_ERROR_EXISTS, "Device already connected", params);
		smartid_log_debug("OPEN called with an existing connection");
		return;
	}

	if (sscanf(params, "%lu %lu %lu", & addr, & lsap, & chunk_size) < 1)
	{
		smartid_conn_send_responsef(c, SMARTI_ERROR_INVALID_COMMAND, "Invalid device address '%s'", params);
		smartid_log_warning("Syntax error in command '%s': invalid parameter", cmd->name);
		return;
	}

	dev = smartid_dev_alloc(smartid_conn_irda(c));
	if (! dev)
	{
		smartid_conn_send_response(c, SMARTI_ERROR_INTERNAL, "Failed to allocate storage");
		return;
	}

	rv = smartid_dev_connect(dev, addr, lsap, chunk_size);
	if (rv != 0)
	{
		switch (rv)
		{
		case SMARTI_ERROR_CONNECT:
			smartid_conn_send_response(c, rv, "Failed to connect to Smart Device");
			break;

		case SMARTI_ERROR_TIMEOUT:
			smartid_conn_send_response(c, rv, "Timeout on Smart Device");
			break;

		case SMARTI_ERROR_HANDSHAKE:
			smartid_conn_send_response(c, rv, "Failed to handshake with Smart Device");
			break;

		case SMARTI_ERROR_IO:
			smartid_conn_send_response(c, rv, "Device I/O error");
			break;

		default:
			smartid_conn_send_responsef(c, SMARTI_ERROR_INTERNAL, "Internal Error (code %d)", rv);

		}

		smartid_dev_dispose(dev);
		return;
	}

	smartid_conn_set_device(c, dev);
	smartid_conn_send_response(c, SMARTI_STATUS_SUCCESS, "Device Opened");
	smartid_log_info("Connected to %lu [lsap: %lu, chunk_size: %lu, client: %s]", addr, lsap, chunk_size, smartid_conn_client(c));
}

static void smartid_cmd_serial(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int rv;
	uint32_t serial;

	CHECK_CMD
	CHECK_NO_PARAMS
	CHECK_DEVICE

	rv = smartid_dev_serial(smartid_conn_device(c), & serial);
	if (rv != 0)
	{
		switch (rv)
		{
		case SMARTI_ERROR_TIMEOUT:
			smartid_conn_send_response(c, rv, "Timeout on Smart Device");
			break;

		case SMARTI_ERROR_IO:
			smartid_conn_send_response(c, rv, "Device I/O error");
			break;

		default:
			smartid_conn_send_responsef(c, SMARTI_ERROR_INTERNAL, "Internal Error (code %d)", rv);

		}

		smartid_dev_dispose(smartid_conn_device(c));
		smartid_conn_set_device(c, 0);
		return;
	}

	smartid_conn_send_responsef(c, SMARTI_STATUS_INFO, "Serial Number: %lu", serial);
}

static void smartid_cmd_size(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int rv;
	uint32_t len;

	CHECK_CMD
	CHECK_NO_PARAMS
	CHECK_DEVICE

	rv = smartid_dev_xfer_size(smartid_conn_device(c), & len);
	if (rv != 0)
	{
		switch (rv)
		{
		case SMARTI_ERROR_TIMEOUT:
			smartid_conn_send_response(c, rv, "Timeout on Smart Device");
			break;

		case SMARTI_ERROR_IO:
			smartid_conn_send_response(c, rv, "Device I/O error");
			break;

		default:
			smartid_conn_send_responsef(c, SMARTI_ERROR_INTERNAL, "Internal Error (code %d)", rv);

		}

		smartid_dev_dispose(smartid_conn_device(c));
		smartid_conn_set_device(c, 0);
		return;
	}

	smartid_conn_send_responsef(c, SMARTI_STATUS_INFO, "Data Size: %lu", len);
}

static void smartid_cmd_tcorr(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int rv;
	uint32_t tcorr;

	CHECK_CMD
	CHECK_NO_PARAMS
	CHECK_DEVICE

	rv = smartid_dev_tcorr(smartid_conn_device(c), & tcorr);
	if (rv != 0)
	{
		smartid_conn_send_responsef(c, SMARTI_ERROR_INTERNAL, "Internal Error (code %d)", rv);
		smartid_dev_dispose(smartid_conn_device(c));
		smartid_conn_set_device(c, 0);
		return;
	}

	smartid_conn_send_responsef(c, SMARTI_STATUS_INFO, "Time Correction: %lu", tcorr);
}

static void smartid_cmd_ticks(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int rv;
	uint32_t ticks;

	CHECK_CMD
	CHECK_NO_PARAMS
	CHECK_DEVICE

	rv = smartid_dev_ticks(smartid_conn_device(c), & ticks);
	if (rv != 0)
	{
		smartid_conn_send_responsef(c, SMARTI_ERROR_INTERNAL, "Internal Error (code %d)", rv);
		smartid_dev_dispose(smartid_conn_device(c));
		smartid_conn_set_device(c, 0);
		return;
	}

	smartid_conn_send_responsef(c, SMARTI_STATUS_INFO, "Ticks: %lu", ticks);
}

static void smartid_cmd_token(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int rv;
	uint32_t token;

	CHECK_CMD
	CHECK_PARAMS
	CHECK_DEVICE

	if (sscanf(params, "%lu", & token) != 1)
	{
		smartid_conn_send_responsef(c, SMARTI_ERROR_INVALID_COMMAND, "Invalid token '%s'", params);
		smartid_log_warning("Syntax error in command '%s': invalid parameter", cmd->name);
		return;
	}

	rv = smartid_dev_set_token(smartid_conn_device(c), token);
	if (rv != 0)
	{
		smartid_conn_send_responsef(c, SMARTI_ERROR_INTERNAL, "Internal Error (code %d)", rv);
	}

	smartid_conn_send_response(c, SMARTI_STATUS_SUCCESS, "Token Set");
}

static void smartid_cmd_xfer(smarti_conn_t c, struct smarti_cmd_t * cmd, const char * params)
{
	int rv;
	uint8_t * buffer;
	uint32_t size;

	CHECK_CMD
	CHECK_NO_PARAMS
	CHECK_DEVICE

	rv = smartid_dev_xfer_data(smartid_conn_device(c), & buffer, & size);
	if (rv != 0)
	{
		switch (rv)
		{
		case SMARTI_STATUS_NO_DATA:
			smartid_conn_send_response(c, rv, "No Data");
			return;

		case SMARTI_ERROR_TIMEOUT:
			smartid_conn_send_response(c, rv, "Timeout on Smart Device");
			break;

		case SMARTI_ERROR_IO:
			smartid_conn_send_response(c, rv, "Device I/O error");
			break;

		case SMARTI_ERROR_DEVICE:
			smartid_conn_send_response(c, rv, "Device Error");
			return;

		default:
			smartid_conn_send_responsef(c, SMARTI_ERROR_INTERNAL, "Internal Error (code %d)", rv);

		}

		smartid_dev_dispose(smartid_conn_device(c));
		smartid_conn_set_device(c, 0);
		return;
	}

	smartid_conn_send_buffer(c, buffer, size);
	free(buffer);
}
