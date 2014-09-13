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

#ifndef UNPACK_H_
#define UNPACK_H_

/**
 * @file src/common/unpack.h
 * @brief Common functions to unpack data
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t uint32_le(const unsigned char * buffer, uint32_t offset);
uint32_t uint32_be(const unsigned char * buffer, uint32_t offset);
int32_t int32_le(const unsigned char * buffer, uint32_t offset);
int32_t int32_be(const unsigned char * buffer, uint32_t offset);

uint32_t uint24_le(const unsigned char * buffer, uint32_t offset);
uint32_t uint24_be(const unsigned char * buffer, uint32_t offset);
int32_t int24_le(const unsigned char * buffer, uint32_t offset);
int32_t int24_be(const unsigned char * buffer, uint32_t offset);

uint16_t uint16_le(const unsigned char * buffer, uint32_t offset);
uint16_t uint16_be(const unsigned char * buffer, uint32_t offset);
int16_t int16_le(const unsigned char * buffer, uint32_t offset);
int16_t int16_be(const unsigned char * buffer, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif /* UNPACK_H_ */
