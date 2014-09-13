/*
 * Copyright (C) 2014 Asymworks, LLC.  All Rights Reserved.
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
#include <string.h>

#include "smart_extract.h"

int smart_extract_dives(void * buffer, uint32_t size, divedata_callback_fn_t cb, void * userdata)
{
	char token[20];
	uint8_t hdr[4] = { 0xa5, 0xa5, 0x5a, 0x5a };
	uint32_t dlen = 0;
	uint32_t tok = 0;
	uint32_t pos = 0;

	if (! buffer)
		return EXTRACT_INVALID;

	while (pos < size)
	{
		if (strncmp((char *)(& buffer[pos]), (char *)hdr, 4) != 0)
			return EXTRACT_CORRUPT;

		dlen = * (uint32_t *)(& buffer[pos + 4]);
		if (pos + dlen > size)
			return EXTRACT_TOO_SHORT;

		tok = * (uint32_t *)(& buffer[pos + 8]);
		sprintf(token, "%u", tok);

		if (cb)
			cb(userdata, & buffer[pos], dlen, token);

		pos += dlen;
	}

	if (pos != size)
		return EXTRACT_EXTRA_DATA;

	return EXTRACT_SUCCESS;
}
