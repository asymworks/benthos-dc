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

#ifndef SETTING_VALUE_H_
#define SETTING_VALUE_H_

/**
 * @file src/common/setting_value.h
 * @brief Common functions to manipulate setting data
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>

#include <benthos/divecomputer/plugin/settings.h>

int value_type(value_handle_t);
int value_size(value_handle_t);
void value_free(value_handle_t);

value_handle_t value_create_int(int32_t);
value_handle_t value_create_uint(uint32_t);
value_handle_t value_create_bool(int);
value_handle_t value_create_time(time_t);
value_handle_t value_create_str(const char *);
value_handle_t value_create_tuple(int, value_handle_t *);
value_handle_t value_clone(value_handle_t);

int value_get_int(value_handle_t, int32_t *);
int value_get_uint(value_handle_t, uint32_t *);
int value_get_bool(value_handle_t, int *);
int value_get_time(value_handle_t, time_t *);
int value_get_str(value_handle_t, char **);
int value_get_tuple_item(value_handle_t, int, value_handle_t *);

#ifdef __cplusplus
}
#endif

#endif /* SETTING_VALUE_H_ */
