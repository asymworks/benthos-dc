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

#include "unpack.h"

uint32_t uint32_le(const unsigned char * buffer, uint32_t offset)
{
	return buffer[offset+0] + (buffer[offset+1] << 8) + (buffer[offset+2] << 16) + (buffer[offset+3] << 24);
}

uint32_t uint32_be(const unsigned char * buffer, uint32_t offset)
{
	return (buffer[offset+0] << 24) + (buffer[offset+1] << 16) + (buffer[offset+2] << 8) + buffer[offset+3];
}

int32_t int32_le(const unsigned char * buffer, uint32_t offset)
{
	return (int32_t)(buffer[offset+0] + (buffer[offset+1] << 8) + (buffer[offset+2] << 16) + (buffer[offset+3] << 24));
}

int32_t int32_be(const unsigned char * buffer, uint32_t offset)
{
	return (int32_t)((buffer[offset+0] << 24) + (buffer[offset+1] << 16) + (buffer[offset+2] << 8) + buffer[offset+3]);
}

uint32_t uint24_le(const unsigned char * buffer, uint32_t offset)
{
	return buffer[offset+0] + (buffer[offset+1] << 8) + (buffer[offset+2] << 16);
}

uint32_t uint24_be(const unsigned char * buffer, uint32_t offset)
{
	return (buffer[offset+0] << 16) + (buffer[offset+1] << 8) + buffer[offset+2];
}

int32_t int24_le(const unsigned char * buffer, uint32_t offset)
{
	return (int32_t)(buffer[offset+0] + (buffer[offset+1] << 8) + (buffer[offset+2] << 16));
}

int32_t int24_be(const unsigned char * buffer, uint32_t offset)
{
	return (int32_t)((buffer[offset+0] << 16) + (buffer[offset+1] << 8) + buffer[offset+2]);
}

uint16_t uint16_le(const unsigned char * buffer, uint32_t offset)
{
	return buffer[offset+0] + (buffer[offset+1] << 8);
}

uint16_t uint16_be(const unsigned char * buffer, uint32_t offset)
{
	return (buffer[offset+0] << 8) + buffer[offset+1];
}

int16_t int16_le(const unsigned char * buffer, uint32_t offset)
{
	return (int16_t)(buffer[offset+0] + (buffer[offset+1] << 8));
}

int16_t int16_be(const unsigned char * buffer, uint32_t offset)
{
	return (int16_t)((buffer[offset+0] << 8) + buffer[offset+1]);
}
