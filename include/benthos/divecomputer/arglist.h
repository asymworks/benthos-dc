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

#ifndef ARGLIST_H_
#define ARGLIST_H_

/**
 * @file src/common/arglist.h
 * @brief Driver Argument List Parsing
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>

//! Argument List Opaque Pointer
typedef struct arglist_ * arglist_t;

/**
 * @brief Parse an Argument List
 * @param[out] Argument List
 * @param[in] Argument String
 * @return Error value or 0 on success
 *
 * Parses the given string into individual arguments.  Arguments are each
 * pairs of the form name=value and are separated by the colon (:).  To embed
 * a literal equal sign or colon into the string they must be doubled, e.g.
 * to pass a Windows absolute path, the argument would appear as:
 * path=C::\path\to\file.
 */
int arglist_parse(arglist_t * arglist, const char * argstring);

/**
 * @brief Free resources associated with the Argument List
 * @param[in] Argument List
 */
void arglist_close(arglist_t args);

/**
 * @brief Return the number of Arguments in the List
 * @param[in] Argument List
 * @return Number of Arguments
 */
ssize_t arglist_count(arglist_t args);

/**
 * @brief Check if an Argument is defined in the List
 * @param[in] Argument List
 * @param[in] Argument Name
 * @return 1 if the argument is present, otherwise 0
 *
 * The string comparison is case-sensitive.
 */
int arglist_has(arglist_t args, const char * name);

/**
 * @brief Check if an Argument has a Value
 * @param[in] Argument List
 * @param[in] Argument Name
 * @return 1 if the value is defined, otherwise 0
 */
int arglist_hasvalue(arglist_t args, const char * name);

/**
 * @brief Retrieve the Argument Value as a String
 * @param[in] Argument List
 * @param[in] Argument Name
 * @param[out] Argument Value
 * @return 0 on success, 1 if the value is NULL, 2 if the argument is not found
 */
int arglist_read_string(arglist_t args, const char * name, const char ** value);

/**
 * @brief Retrieve the Argument Value as a Signed Integer
 * @param[in] Argument List
 * @param[in] Argument Name
 * @param[out] Argument Value
 * @return 0 on success, 1 if the value is NULL, 2 if the argument is not found
 */
int arglist_read_int(arglist_t args, const char * name, int32_t * value);

/**
 * @brief Retrieve the Argument Value as a Unsigned Integer
 * @param[in] Argument List
 * @param[in] Argument Name
 * @param[out] Argument Value
 * @return 0 on success, 1 if the value is NULL, 2 if the argument is not found
 */
int arglist_read_uint(arglist_t args, const char * name, uint32_t * value);

/**
 * @brief Retrieve the Argument Value as a Floating Point
 * @param[in] Argument List
 * @param[in] Argument Name
 * @param[out] Argument Value
 * @return 0 on success, 1 if the value is NULL, 2 if the argument is not found
 */
int arglist_read_float(arglist_t args, const char * name, double * value);

#ifdef __cplusplus
}
#endif

#endif /* ARGLIST_H_ */
