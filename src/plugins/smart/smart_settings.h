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

/*
 * Portions of the Uwatec Smart parser were adapted from libdivecomputer,
 * Copyright (C) 2008 Jef Driesen.  libdivecomputer can be found online at
 * http://www.divesoftware.org/libdc/
 *
 * Also, significant credit is due to Simon Naunton, whose work decoding
 * the Uwatec Smart protocol has been extremely helpful.  His work can be
 * found online at http://diversity.sourceforge.net/uwatec_smart_format.html
 */

#ifndef SMART_SETTINGS_H_
#define SMART_SETTINGS_H_

/**
 * @file src/plugins/smart/smart_io.h
 * @brief Uwatec Smart Device Settings Functions
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <benthos/divecomputer/plugin/plugin.h>
#include <benthos/divecomputer/plugin/settings.h>

int smart_settings_get_tod(dev_handle_t, time_t *);
int smart_settings_set_tod(dev_handle_t, time_t);

int smart_settings_has(dev_handle_t, const char *);
int smart_settings_get(dev_handle_t, const char *, value_handle_t *);
int smart_settings_set(dev_handle_t, const char *, const value_handle_t);

struct setting_entry_t * smart_settings_table(uint8_t);

typedef int (* smart_settings_getter_fn_t)(dev_handle_t, value_handle_t *);
typedef int (* smart_settings_setter_fn_t)(dev_handle_t, const value_handle_t);

struct setting_entry_t {
	const char *				name;
	smart_settings_getter_fn_t	getter;
	smart_settings_setter_fn_t	setter;

};

#ifdef __cplusplus
}
#endif

#endif /* SMART_SETTINGS_H_ */
