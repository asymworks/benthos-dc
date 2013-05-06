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

#ifndef SMART_SETTINGS_TEC2G_H_
#define SMART_SETTINGS_TEC2G_H_

/**
 * @file src/plugins/smart/smart_settings_tec2g.h
 * @brief Setting Getter/Setters for the Uwatec Aladin Tec 2G
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <benthos/divecomputer/plugin/plugin.h>
#include <benthos/divecomputer/plugin/settings.h>

#include "smart_settings.h"

extern struct setting_entry_t tec2g_settings_table[];

int smart_settings_tec2g_get_sw_version(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_get_hw_version(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_get_batt_level(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_get_pressure(dev_handle_t, value_handle_t *);

int smart_settings_tec2g_get_total_time(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_get_total_dives(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_get_max_time(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_get_max_depth(dev_handle_t, value_handle_t *);

int smart_settings_tec2g_get_utc_offset(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_set_utc_offset(dev_handle_t, const value_handle_t);
int smart_settings_tec2g_get_backlight(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_set_backlight(dev_handle_t, const value_handle_t);
int smart_settings_tec2g_get_contrast(dev_handle_t, value_handle_t *);
int smart_settings_tec2g_set_contrast(dev_handle_t, const value_handle_t);

#ifdef __cplusplus
}
#endif


#endif /* SMART_SETTINGS_TEC2G_H_ */
