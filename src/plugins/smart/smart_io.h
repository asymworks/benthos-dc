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

#ifndef SMART_IO_H_
#define SMART_IO_H_

/**
 * @file src/plugins/smart/smart_io.h
 * @brief Uwatec Smart Device I/O Functions
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

//! Uwatec Smart Device Handle
typedef struct smart_device_ *	smart_device_t;

/**
 * @brief Send a Command to the Smart Device
 * @param[in] Device Handle
 * @param[in] Command Data
 * @param[in] Command Size
 * @param[out] Answer Data
 * @param[in] Answer Size
 * @return Error value or 0 on success
 */
int smart_driver_cmd(smart_device_t dev, unsigned char * cmd, ssize_t cmdlen, unsigned char * ans, ssize_t anslen);

/**
 * @brief Read an Unsigned Character from the Smart Device
 * @param[in] Device Handle
 * @param[in] Command Data
 * @param[out] Answer Data
 * @return Error value or 0 on success
 */
int smart_read_uchar(smart_device_t dev, const char * cmd, uint8_t * ans);

/**
 * @brief Read a Signed Character from the Smart Device
 * @param[in] Device Handle
 * @param[in] Command Data
 * @param[out] Answer Data
 * @return Error value or 0 on success
 */
int smart_read_schar(smart_device_t dev, const char * cmd, int8_t * ans);

/**
 * @brief Read an Unsigned Integer from the Smart Device
 * @param[in] Device Handle
 * @param[in] Command Data
 * @param[out] Answer Data
 * @return Error value or 0 on success
 */
int smart_read_ushort(smart_device_t dev, const char * cmd, uint16_t * ans);

/**
 * @brief Read a Signed Integer from the Smart Device
 * @param[in] Device Handle
 * @param[in] Command Data
 * @param[out] Answer Data
 * @return Error value or 0 on success
 */
int smart_read_sshort(smart_device_t dev, const char * cmd, int16_t * ans);

/**
 * @brief Read an Unsigned Long Integer from the Smart Device
 * @param[in] Device Handle
 * @param[in] Command Data
 * @param[out] Answer Data
 * @return Error value or 0 on success
 */
int smart_read_ulong(smart_device_t dev, const char * cmd, uint32_t * ans);

/**
 * @brief Read a Signed Long Integer from the Smart Device
 * @param[in] Device Handle
 * @param[in] Command Data
 * @param[out] Answer Data
 * @return Error value or 0 on success
 */
int smart_read_slong(smart_device_t dev, const char * cmd, int32_t * ans);

#ifdef __cplusplus
}
#endif


#endif /* SMART_IO_H_ */
