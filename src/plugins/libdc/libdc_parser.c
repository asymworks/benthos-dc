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

#include <errno.h>
#include <math.h>
#include <stdlib.h>

#include <libdivecomputer/parser.h>

#include "libdc_driver.h"
#include "libdc_parser.h"

struct libdc_parser_
{
	libdc_device_t			dev;			///< Device Handle
	dc_parser_t *			parser;			///< libdivecomputer Parser Handle

	waypoint_callback_fn_t	wcb;			///< Waypoint Callback Function
	void *					wcb_data;		///< Waypoint Callback Data
};

void libdc_sample_cb(dc_sample_type_t type, dc_sample_value_t value, void * userdata)
{
	static const char *events[] = {
			"none", "deco", "rbt", "ascent", "ceiling", "workload", "transmitter",
			"violation", "bookmark", "surface", "safety stop", "gaschange",
			"safety stop (voluntary)", "safety stop (mandatory)", "deepstop",
			"ceiling (safety stop)", "unknown", "divetime", "maxdepth",
			"OLF", "PO2", "airtime", "rgbm", "heading", "tissue level warning",
			"gaschange2"};

	libdc_parser_t parser = (libdc_parser_t)(userdata);
	if (parser == NULL)
		return;

	if (! parser->wcb)
		return;

	uint32_t intval;
	switch (type)
	{
	case DC_SAMPLE_TIME:
		parser->wcb(parser->wcb_data, DIVE_WAYPOINT_TIME, value.time, 0, 0);
		break;

	case DC_SAMPLE_DEPTH:
		// Centimeters
		intval = (uint32_t)round(value.depth * 100.0);
		parser->wcb(parser->wcb_data, DIVE_WAYPOINT_DEPTH, intval, 0, 0);
		break;

	case DC_SAMPLE_TEMPERATURE:
		// Centidegrees Celsius
		intval = (uint32_t)round(value.temperature * 100.0);
		parser->wcb(parser->wcb_data, DIVE_WAYPOINT_TEMP, intval, 0, 0);

	case DC_SAMPLE_PRESSURE:
		// Millibar
		intval = (uint32_t)round(value.pressure.value);
		parser->wcb(parser->wcb_data, DIVE_WAYPOINT_PX, intval, value.pressure.tank, 0);
		break;

	case DC_SAMPLE_RBT:
		parser->wcb(parser->wcb_data, DIVE_WAYPOINT_RBT, value.rbt, 0, 0);
		break;

	case DC_SAMPLE_HEARTBEAT:
		parser->wcb(parser->wcb_data, DIVE_WAYPOINT_HEARTRATE, value.heartbeat, 0, 0);
		break;

	case DC_SAMPLE_BEARING:
		parser->wcb(parser->wcb_data, DIVE_WAYPOINT_BEARING, value.bearing, 0, 0);
		break;

	case DC_SAMPLE_EVENT:
		parser->wcb(parser->wcb_data, DIVE_WAYPOINT_ALARM, value.event.type, 0, events[value.event.type]);
		break;

	default:
		break;
	}
}

int libdc_parser_create(parser_handle_t * abstract, dev_handle_t abstract_dev)
{
	libdc_parser_t * parser = (libdc_parser_t *)(abstract);
	libdc_device_t dev = (libdc_device_t)(abstract_dev);

	if ((parser == NULL) || (dev == NULL))
	{
		errno = EINVAL;
		return -1;
	}

	libdc_parser_t p = (libdc_parser_t)malloc(sizeof(struct libdc_parser_));
	if (p == NULL)
		return -1;

	p->dev = dev;
	p->parser = NULL;

	dc_status_t rc = dc_parser_new(& p->parser, p->dev->device);
	if (rc != DC_STATUS_SUCCESS)
	{
		dev->errcode = DRIVER_ERR_INVALID;
		dev->errmsg = "Failed to create a libdivecomputer Parser";
		free(p);
		return -1;
	}

	libdc_parser_reset((parser_handle_t)p);

	*abstract = (parser_handle_t)p;
	return 0;
}

void libdc_parser_close(parser_handle_t abstract)
{
	libdc_parser_t parser = (libdc_parser_t)(abstract);
	if (parser == NULL)
		return;

	if (parser->parser != NULL)
		dc_parser_destroy(parser->parser);

	parser->parser = NULL;

	free(parser);
}

int libdc_parser_reset(parser_handle_t abstract)
{
	libdc_parser_t parser = (libdc_parser_t)(abstract);
	if (parser == NULL)
		return -1;

	return 0;
}

int libdc_parser_parse_header(parser_handle_t abstract, const void * buffer, uint32_t size, header_callback_fn_t cb, void * userdata)
{
	libdc_parser_t parser = (libdc_parser_t)(abstract);
	if (parser == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	dc_status_t rc = dc_parser_set_data(parser->parser, buffer, size);
	if (rc != DC_STATUS_SUCCESS)
	{
		parser->dev->errcode = DRIVER_ERR_PARSER;
		parser->dev->errmsg = "Failed to set parser data";
		return -1;
	}

	// Parse Dive Date/Time
	dc_datetime_t dt = {0};
	rc = dc_parser_get_datetime(parser->parser, & dt);
	if ((rc != DC_STATUS_SUCCESS) && (rc != DC_STATUS_UNSUPPORTED))
	{
		parser->dev->errcode = DRIVER_ERR_PARSER;
		parser->dev->errmsg = "Failed to retrieve dive date/time";
		return -1;
	}

	if ((rc == DC_STATUS_SUCCESS) && (cb != NULL))
	{
		dc_ticks_t ts = dc_datetime_mktime(& dt);
		cb(userdata, DIVE_HEADER_START_TIME, (time_t)ts, 0, 0);
	}

	// Parse Dive Duration
	unsigned int duration = 0;
	rc = dc_parser_get_field (parser->parser, DC_FIELD_DIVETIME, 0, & duration);
	if ((rc != DC_STATUS_SUCCESS) && (rc != DC_STATUS_UNSUPPORTED))
	{
		parser->dev->errcode = DRIVER_ERR_PARSER;
		parser->dev->errmsg = "Failed to retrieve dive duration";
		return -1;
	}

	if ((rc == DC_STATUS_SUCCESS) && (cb != NULL))
		cb(userdata, DIVE_HEADER_DURATION, duration / 60, 0, 0);

	// Parse Maximum Depth
	double maxdepth = 0.0;
	rc = dc_parser_get_field (parser->parser, DC_FIELD_MAXDEPTH, 0, & maxdepth);
	if ((rc != DC_STATUS_SUCCESS) && (rc != DC_STATUS_UNSUPPORTED))
	{
		parser->dev->errcode = DRIVER_ERR_PARSER;
		parser->dev->errmsg = "Failed to retrieve maximum depth";
		return -1;
	}

	if ((rc == DC_STATUS_SUCCESS) && (cb != NULL))
		cb(userdata, DIVE_HEADER_MAX_DEPTH, (uint32_t)round(maxdepth * 100), 0, 0);

	// Parse Gas Mixes
	unsigned int ngasses = 0;
	rc = dc_parser_get_field(parser->parser, DC_FIELD_GASMIX_COUNT, 0, & ngasses);
	if ((rc != DC_STATUS_SUCCESS) && (rc != DC_STATUS_UNSUPPORTED))
	{
		parser->dev->errcode = DRIVER_ERR_PARSER;
		parser->dev->errmsg = "Failed to retrieve gas mix count";
		return -1;
	}

	unsigned int i;
	for (i = 0; i < ngasses; ++i)
	{
		dc_gasmix_t gasmix = {0};
		rc = dc_parser_get_field(parser->parser, DC_FIELD_GASMIX, i, & gasmix);
		if ((rc != DC_STATUS_SUCCESS) && (rc != DC_STATUS_UNSUPPORTED))
		{
			parser->dev->errcode = DRIVER_ERR_PARSER;
			parser->dev->errmsg = "Failed to retrieve gas mix data";
			return -1;
		}

		if ((rc == DC_STATUS_SUCCESS) && (cb != NULL))
		{
			uint32_t pmHe = (uint32_t)round(gasmix.helium * 1000.0);
			uint32_t pmO2 = (uint32_t)round(gasmix.oxygen * 1000.0);

			cb(userdata, DIVE_HEADER_PMO2, pmO2, i, 0);
			cb(userdata, DIVE_HEADER_PMHe, pmHe, i, 0);
		}
	}

	return 0;
}

int libdc_parser_parse_profile(parser_handle_t abstract, const void * buffer, uint32_t size, waypoint_callback_fn_t cb, void * userdata)
{
	libdc_parser_t parser = (libdc_parser_t)(abstract);
	if (parser == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	dc_status_t rc = dc_parser_set_data(parser->parser, buffer, size);
	if (rc != DC_STATUS_SUCCESS)
	{
		parser->dev->errcode = DRIVER_ERR_PARSER;
		parser->dev->errmsg = "Failed to set parser data";
		return -1;
	}

	parser->wcb = cb;
	parser->wcb_data = userdata;
	rc = dc_parser_samples_foreach(parser->parser, libdc_sample_cb, parser);
	if (rc != DC_STATUS_SUCCESS)
	{
		parser->dev->errcode = DRIVER_ERR_PARSER;
		parser->dev->errmsg = "Failed to parse profile data";
		return -1;
	}

	return 0;
}
