/*
 * Copyright (C) 2013 Asymworks, LLC.  All Rights Reserved.
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

#ifndef BASE64_H_
#define BASE64_H_

/**
 * @file src/common/base64.h
 * @brief Base64 String Functions
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Encode a string in Base 64
 * @param[in] Input Data to Encode
 * @param[in] Input Data Length
 * @param[out] Output Data Length
 * @return Pointer to Base 64 string
 *
 * Encodes the given string in Base 64.  The given length parameter must be set
 * with the data length as NUL characters are encoded as data.  If the string
 * cannot be encoded, NULL is returned.
 *
 * @note The caller is responsible for free()'ing the returned string
 */
char * base64_encode(const unsigned char * data, size_t in_length, size_t * out_length);

/**
 * @brief Decode a string from Base 64
 * @param[in] Input Data to Decode
 * @param[in] Input Data Length
 * @param[out] Output Data Length
 * @return Pointer to Decoded Data
 *
 * Decodes the given string from Base 64.  If the string cannot be encoded,
 * NULL is returned.
 *
 * @note The caller is responsible for free()'ing the returned data buffer.
 */
unsigned char * base64_decode(const char * data, size_t in_length, size_t * out_length);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_H_ */
