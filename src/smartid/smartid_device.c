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

/**
 * @file smartid_device.c
 * @brief Smart Device Driver
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "irda.h"
#include "smarti_codes.h"
#include "smartid_device.h"
#include "smartid_logging.h"

struct smart_device_t_
{
	irda_t			s;			///< IrDA Socket
	unsigned int	epaddr;		///< IrDA Endpoint Address

	int				lsap;		///< IrDA LSAP Identifier
	size_t			csize;		///< IrDA Chunk Size

	time_t			epoch;		///< Epoch in Half-Seconds
	int32_t			tcorr;		///< Time Correction Value

	uint8_t			model;		///< Model Number
	uint32_t		serial;		///< Serial Number
	uint32_t		ticks;		///< Tick Count

	uint32_t		token;		///< Device Token
};

int smart_driver_cmd(smart_device_t dev, unsigned char * cmd, ssize_t cmdlen, unsigned char * ans, ssize_t anslen)
{
	if ((dev == NULL) || (dev->s == NULL))
	{
		smartid_log_error("Null pointer passed to smart_driver_cmd()");
		return -1;
	}

	int rc;
	int timeout = 0;
	ssize_t _cmdlen = cmdlen;
	ssize_t _anslen = anslen;

	rc = irda_socket_write(dev->s, cmd, & _cmdlen, & timeout);
	if (rc != 0)
	{
		smartid_log_error("Failed to write to the Uwatec Smart Device");
		return SMARTI_ERROR_IO;
	}

	if (timeout)
	{
		smartid_log_error("Timed out writing to the Uwatec Smart Device");
		return SMARTI_ERROR_TIMEOUT;
	}

	rc = irda_socket_read(dev->s, ans, & _anslen, & timeout);
	if (rc != 0)
	{
		smartid_log_error("Failed to read from the Uwatec Smart Device");
		return SMARTI_ERROR_IO;
	}

	if (timeout)
	{
		smartid_log_error("Timed out reading from the Uwatec Smart Device");
		return SMARTI_ERROR_TIMEOUT;
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
	if ((dev == NULL) || (dev->s == NULL))
	{
		smartid_log_error("Null pointer passed to smart_driver_handshake()");
		return -1;
	}

	int rc;
	unsigned char cmd1[] = { 0x1b };
	unsigned char cmd2[] = { 0x1c, 0x10, 0x27, 0x00, 0x00 };
	unsigned char ans;

	rc = smart_driver_cmd(dev, cmd1, 1, & ans, 1);
	if ((rc != 0) || (ans != 0x01))
	{
		smartid_log_debug("Handshake Phase 1 failed");
		return -1;
	}

	rc = smart_driver_cmd(dev, cmd2, 5, & ans, 1);
	if ((rc != 0) || (ans != 0x01))
	{
		smartid_log_debug("Handshake Phase 2 failed");
		return -1;
	}

	return 0;
}

smart_device_t smartid_dev_alloc(irda_t s)
{
	smart_device_t ret;

	if (! s)
	{
		smartid_log_error("Null IrDA socket passed to smartid_dev_alloc()");
		return 0;
	}

	ret = (smart_device_t)malloc(sizeof(struct smart_device_t_));
	if (! ret)
	{
		smartid_log_error("Failed to allocate memory for struct smart_device_t");
		return 0;
	}

	memset(ret, 0, sizeof(struct smart_device_t_));
	ret->s = s;

	return ret;
}

void smartid_dev_dispose(smart_device_t dev)
{
	if (! dev)
	{
		smartid_log_error("Null pointer passed to smartid_dev_dispose()");
		return;
	}

	free(dev);
}

int smartid_dev_connect(smart_device_t dev, unsigned int addr, int lsap, size_t chunk_size)
{
	int rv;
	int timeout = 0;

	if (! dev)
	{
		smartid_log_error("Null pointer passed to smartid_dev_connect()");
		return SMARTI_ERROR_INTERNAL;
	}

	/* Setup IrDA Information */
	if (chunk_size < 2)
		chunk_size = 2;
	if (chunk_size > 32)
		chunk_size = 32;

	dev->epaddr = addr;
	dev->lsap = lsap;
	dev->csize = chunk_size;

	/* Connect to the Device */
	rv = irda_socket_connect_lsap(dev->s, dev->epaddr, dev->lsap, & timeout);
	if (rv != 0)
	{
		smartid_log_error("Failed to connect to device (code %d: %s)", irda_errcode(), irda_errmsg());
		return SMARTI_ERROR_CONNECT;
	}

	if (timeout)
	{
		smartid_log_error("Timed out connecting to device");
		return SMARTI_ERROR_TIMEOUT;
	}

	/* Handshake with the Device */
	rv = smart_driver_handshake(dev);
	if (rv != 0)
	{
		smartid_log_error("Failed to handshake with the device");
		return SMARTI_ERROR_HANDSHAKE;
	}

	/* Calculate Time Offset */
	rv = smart_read_ulong(dev, "\x1a", & dev->ticks);
	if (rv != 0)
	{
		smartid_log_error("Failed to read ticks from the device");
		return rv;
	}

	time_t hstime = time(NULL) * 2;
	dev->epoch = smart_driver_epoch();
	dev->tcorr = hstime - dev->ticks;

	/* Success */
	return 0;
}

int smartid_dev_model(smart_device_t dev, uint8_t * model)
{
	if (! dev)
	{
		smartid_log_error("Null pointer passed to smartid_dev_model()");
		return SMARTI_ERROR_INTERNAL;
	}

	return smart_read_uchar(dev, "\x10", model);
}

int smartid_dev_serial(smart_device_t dev, uint32_t * serial)
{
	if (! dev)
	{
		smartid_log_error("Null pointer passed to smartid_dev_serial()");
		return SMARTI_ERROR_INTERNAL;
	}

	return smart_read_ulong(dev, "\x14", serial);
}

int smartid_dev_tcorr(smart_device_t dev, uint32_t * tcorr)
{
	if (! dev)
	{
		smartid_log_error("Null pointer passed to smartid_dev_tcorr()");
		return SMARTI_ERROR_INTERNAL;
	}

	* tcorr = dev->tcorr;
	return 0;
}

int smartid_dev_ticks(smart_device_t dev, uint32_t * ticks)
{
	if (! dev)
	{
		smartid_log_error("Null pointer passed to smartid_dev_ticks()");
		return SMARTI_ERROR_INTERNAL;
	}

	* ticks = dev->ticks;
	return 0;
}

int smartid_dev_set_token(smart_device_t dev, uint32_t token)
{
	if (! dev)
	{
		smartid_log_error("Null pointer passed to smartid_dev_set_token()");
		return SMARTI_ERROR_INTERNAL;
	}

	dev->token = token;
	return 0;
}

int smartid_dev_xfer_size(smart_device_t dev, uint32_t * size)
{
	unsigned char cmd[] = { 0xc6, 0, 0, 0, 0, 0x10, 0x27, 0, 0 };

	if (! dev)
	{
		smartid_log_error("Null pointer passed to smartid_dev_xfer_size()");
		return SMARTI_ERROR_INTERNAL;
	}

	* (uint32_t *)(& cmd[1]) = dev->token;

	return smart_driver_cmd(dev, cmd, 9, (unsigned char *)(size), 4);
}

int smartid_dev_xfer_data(smart_device_t dev, uint8_t ** buf, uint32_t * size)
{
	int rv;
	uint32_t nb;
	uint32_t pos;
	uint32_t len;
	unsigned char cmd[] = { 0xc4, 0, 0, 0, 0, 0x10, 0x27, 0, 0 };

	if (! dev || ! buf)
	{
		smartid_log_error("Null pointer passed to smartid_dev_xfer_data()");
		return SMARTI_ERROR_INTERNAL;
	}

	* (uint32_t *)(& cmd[1]) = dev->token;

	rv = smart_driver_cmd(dev, cmd, 9, (unsigned char *)(& nb), 4);
	if (rv != 0)
		return rv;

	if (nb == 4)
	{
		smartid_log_debug("No bytes to transfer from device");
		* buf = 0;
		* size = 0;
		return SMARTI_STATUS_NO_DATA;
	}

	if (nb < 4)
	{
		smartid_log_warning("Data length too short (%lu bytes)", nb);
		return SMARTI_ERROR_DEVICE;
	}

	(* size) = nb - 4;
	(* buf) = malloc(* size);

	if (! (* buf))
	{
		smartid_log_warning("Failed to allocate %lu bytes for transfer buffer", * size);
		return SMARTI_ERROR_INTERNAL;
	}

	smartid_log_debug("Allocated transfer buffer for %lu bytes", * size);

	pos = 0;
	len = (* size);
	while (len > 0)
	{
		int timeout = 0;
		size_t nc;

		if (len > dev->csize)
		{
			nc = dev->csize;
		}
		else
		{
			nc = len;
		}

		rv = irda_socket_read(dev->s, & (* buf)[pos], & nc, & timeout);
		if (rv != 0)
		{
			smartid_log_error("Failed to read from the Uwatec Smart Device (code %d: %s)", errno, strerror(errno));
			free(buf);
			* buf = 0;
			* size = 0;
			return SMARTI_ERROR_IO;
		}

		if (timeout)
		{
			smartid_log_error("Timed out reading from the Uwatec Smart Device");
			free(buf);
			* buf = 0;
			* size = 0;
			return SMARTI_ERROR_TIMEOUT;
		}

		len -= nc;
		pos += nc;

		/* smartid_log_debug("Transferred chunk: %lu, cum: %lu, tot: %lu, rem: %lu", nc, pos, * size, len); */
	}

	smartid_log_debug("Transferred %lu bytes", * size);

	return 0;
}

