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

#ifndef LIBDC_PARSER_H_
#define LIBDC_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <benthos/divecomputer/plugin/parser.h>
#include <benthos/divecomputer/plugin/plugin.h>

typedef struct libdc_parser_ *	libdc_parser_t;

int libdc_parser_create(parser_handle_t * parser, dev_handle_t dev);
void libdc_parser_close(parser_handle_t parser);
int libdc_parser_reset(parser_handle_t parser);

int libdc_parser_parse_header(parser_handle_t parser, const void * buffer, uint32_t size, header_callback_fn_t cb, void * userdata);
int libdc_parser_parse_profile(parser_handle_t parser, const void * buffer, uint32_t size, waypoint_callback_fn_t cb, void * userdata);

#ifdef __cplusplus
}
#endif

#endif /* LIBDC_PARSER_H_ */
