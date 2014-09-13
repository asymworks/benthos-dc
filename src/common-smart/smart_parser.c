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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <benthos/divecomputer/unpack.h>

#include "smart_device_base.h"
#include "smart_parser.h"

#define MDL_SMART_PRO		16		// Smart Pro
#define MDL_GALILEO_SOL		17		// Galileo Sol
#define MDL_ALADIN_TEC		18		// Aladin Tec
#define MDL_ALADIN_TEC2G	19		// Aladin Tec 2G
#define MDL_SMART_COM		20		// Smart Com
#define MDL_SMART_TEC		24		// Smart Tec
#define MDL_SMART_Z			28		// Smart Z

/*
 * Smart Data Type Indicator Entry
 */
typedef struct
{
	uint8_t				type;	///< DTI Type
	uint8_t				abs;	///< Absolute or Delta
	uint8_t				idx;	///< Index
	uint8_t				ntb;	///< Number of Type Bits
	uint8_t				ignore;	///< Ignore Type Bites
	uint8_t				extra;	///< Number of Extra Bytes

} dti_entry_t;

/*
 * Smart Alarm Name Entry
 */
typedef struct
{
	uint8_t				idx;	///< Index
	uint8_t				mask;	///< Bit Mask
	const char *		name;	///< Alarm Name

} alarm_entry_t;

enum
{
	DTI_PRESSURE_DEPTH,		///< Pressure/Depth DTI
	DTI_DEPTH,				///< Depth DTI
	DTI_TEMPERATURE,		///< Temperature DTI
	DTI_PRESSURE,			///< Pressure DTI
	DTI_RBT,				///< Remaining Bottom Time DTI
	DTI_HEARTRATE,			///< Heart Rate DTI
	DTI_BEARING,			///< Bearing DTI
	DTI_ALARMS,				///< Alarms DTI
	DTI_TIME				///< Time DTI
};

struct smart_parser_
{
	struct smart_device_base_t * dev;		///< Device Handle

	uint32_t				hdr_size;		///< Header Size
	uint8_t					dti_size;		///< DTI Table Size
	const dti_entry_t	*	dti_table;		///< DTI Table Pointer

	uint8_t					alarm_size;		///< Alarm Table Size
	const alarm_entry_t *	alarm_table;	///< Alarm Table Pointer

	uint32_t				time;			///< Current Time
	uint32_t				depth;			///< Current Depth
	uint32_t				temp;			///< Current Temperature
	uint32_t				pressure;		///< Current Pressure
	uint32_t				rbt;			///< Current RBT
	uint32_t				heartrate;		///< Current Heart Rate
	uint32_t				bearing;		///< Current Bearing
	uint16_t				alarms[3];		///< Alarm Flags
	uint8_t					tank;			///< Current Tank Index

	uint32_t				dcal;			///< Depth Calibration

	uint8_t					complete;		///< Sample Complete
	uint8_t					calibrated;		///< Depth Calibrated

	uint8_t					have_depth;		///< Sample Includes Depth
	uint8_t					have_temp;		///< Sample Includes Temperature
	uint8_t					have_pressure;	///< Sample Includes Pressure
	uint8_t					have_rbt;		///< Sample Includes RBT
	uint8_t					have_heartrate;	///< Sample Includes Heart Rate
	uint8_t					have_bearing;	///< Sample Includes Bearing
	uint8_t					have_alarms;	///< Sample Includes Alarms
};

// Model Type 16 (Smart Pro)
static const dti_entry_t smart_pro_table[] =
{
	{DTI_DEPTH,				0,	0,	1,	0,	0},		// 0ddd dddd
	{DTI_TEMPERATURE,		0,	0,	2,	0,	0},		// 10dd dddd
	{DTI_TIME,				1,	0,	3,	0,	0},		// 110d dddd
	{DTI_ALARMS,			1,	0,	4,	0,	0},		// 1110 dddd
	{DTI_DEPTH,				0,	0,	5,	0,	1},		// 1111 0ddd dddd dddd
	{DTI_TEMPERATURE,		0,	0,	6,	0,	1},		// 1111 10dd dddd dddd
	{DTI_DEPTH,				1,	0,	7,	1,	2},		// 1111 110d dddd dddd dddd dddd
	{DTI_TEMPERATURE,		1,	0,	8,	0,	2},		// 1111 1110 dddd dddd dddd dddd
};

// Model Type 17 (Galileo Sol)
static const dti_entry_t smart_galileo_sol_table[] =
{
	{DTI_DEPTH,				0,	0,	1,	0,	0},		// 0ddd dddd
	{DTI_RBT,				0,	0,	3,	0,	0},		// 100d dddd
	{DTI_PRESSURE,			0,	0,	4,	0,	0},		// 1010 dddd
	{DTI_TEMPERATURE,		0,	0,	4,	0,	0},		// 1011 dddd
	{DTI_TIME,				1,	0,	4,	0,	0},		// 1100 dddd
	{DTI_HEARTRATE,			0,	0,	4,	0,	0},		// 1101 dddd
	{DTI_ALARMS,			1,	0,	4,	0,	0},		// 1110 dddd
	{DTI_ALARMS,			1,	1,	8,	0,	1},		// 1111 0000 dddd dddd
	{DTI_DEPTH,				1,	0,	8,	0,	2},		// 1111 0001 dddd dddd dddd dddd
	{DTI_RBT,				1,	0,	8,	0,	1},		// 1111 0010 dddd dddd
	{DTI_TEMPERATURE,		1,	0,	8,	0,	2},		// 1111 0011 dddd dddd dddd dddd
	{DTI_PRESSURE,			1,	0,	8,	0,	2},		// 1111 0100 dddd dddd dddd dddd
	{DTI_PRESSURE,			1,	1,	8,	0,	2},		// 1111 0101 dddd dddd dddd dddd
	{DTI_PRESSURE,			1,	2,	8,	0,	2},		// 1111 0110 dddd dddd dddd dddd
	{DTI_HEARTRATE,			1,	0,	8,	0,	1},		// 1111 0111 dddd dddd
	{DTI_BEARING,			1,	0,	8,	0,	2},		// 1111 1000 dddd dddd dddd dddd
	{DTI_ALARMS,			1,	2,	8,	0,	1},		// 1111 1001 dddd dddd
};

// Model Type 18 (Aladin)
static const dti_entry_t smart_aladin_table[] =
{
	{DTI_DEPTH,				0,	0,	1,	0,	0},		// 0ddd dddd
	{DTI_TEMPERATURE,		0,	0,	2,	0,	0},		// 10dd dddd
	{DTI_TIME,				1,	0,	3,	0,	0},		// 110d dddd
	{DTI_ALARMS,			1,	0,	4,	0,	0},		// 1110 dddd
	{DTI_DEPTH,				0,	0,	5,	0,	1},		// 1111 0ddd dddd dddd
	{DTI_TEMPERATURE,		0,	0,	6,	0,	1},		// 1111 10dd dddd dddd
	{DTI_DEPTH,				1,	0,	7,	1,	2},		// 1111 110d dddd dddd dddd dddd
	{DTI_TEMPERATURE,		1,	0,	8,	0,	2},		// 1111 1110 dddd dddd dddd dddd
	{DTI_ALARMS,			1,	1,	9,	0,	0},		// 1111 1111 0ddd dddd
};

// Model Type 19 (Aladin Tec 2G)
static const dti_entry_t smart_aladin_tec2g_table[] =
{
	{DTI_DEPTH,				0,	0,	1,	0,	0},		// 0ddd dddd
	{DTI_TEMPERATURE,		0,	0,	2,	0,	0},		// 10dd dddd
	{DTI_TIME,				1,	0,	3,	0,	0},		// 110d dddd
	{DTI_ALARMS,			1,	0,	4,	0,	0},		// 1110 dddd
	{DTI_DEPTH,				0,	0,	5,	0,	1},		// 1111 0ddd dddd dddd
	{DTI_TEMPERATURE,		0,	0,	6,	0,	1},		// 1111 10dd dddd dddd
	{DTI_DEPTH,				1,	0,	7,	1,	2},		// 1111 110d dddd dddd dddd dddd
	{DTI_TEMPERATURE,		1,	0,	8,	0,	2},		// 1111 1110 dddd dddd dddd dddd
	{DTI_ALARMS,			1,	1,	9,	0,	0},		// 1111 1111 0ddd dddd
};

static const alarm_entry_t smart_aladin_tec2g_alarms[] =
{
	{ 0,	(1 << 1),		"ascent" },
	{ 0,	(1 << 2),		"bookmark" },
};

// Model Type 20 (Smart Com)
static const dti_entry_t smart_com_table[] =
{
	{DTI_PRESSURE_DEPTH,	0,	0,	1,	0,	1},		// 0ddd dddd dddd dddd
	{DTI_RBT,				0,	0,	2,	0,	0},		// 10dd dddd
	{DTI_TEMPERATURE,		0,	0,	3,	0,	0},		// 110d dddd
	{DTI_PRESSURE,			0,	0,	4,	0,	1},		// 1110 dddd dddd dddd
	{DTI_DEPTH,				0,	0,	5,	0,	1},		// 1111 0ddd dddd dddd
	{DTI_TEMPERATURE,		0,	0,	6,	0,	1},		// 1111 10dd dddd dddd
	{DTI_ALARMS,			1,	0,	7,	1,	1},		// 1111 110d dddd dddd
	{DTI_TIME,				1,	0,	8,	0,	1},		// 1111 1110 dddd dddd
	{DTI_DEPTH,				1,	0,	9,	1,	2},		// 1111 1111 0ddd dddd dddd dddd dddd dddd
	{DTI_TEMPERATURE,		1,	0,	10,	1,	2},		// 1111 1111 10dd dddd dddd dddd dddd dddd
	{DTI_PRESSURE,			1,	0,	11,	1,	2},		// 1111 1111 110d dddd dddd dddd dddd dddd
	{DTI_RBT,				1,	0,	12,	1,	1},		// 1111 1111 1111 10dd dddd dddd
};

// Model Type 24 (Smart Tec)
// Model Type 28 (Smart Z)
static const dti_entry_t smart_tec_table[] =
{
	{DTI_PRESSURE_DEPTH,	0,	0,	1,	0,	1},		// 0ddd dddd dddd dddd
	{DTI_RBT,				0,	0,	2,	0,	0},		// 10dd dddd
	{DTI_TEMPERATURE,		0,	0,	3,	0,	0},		// 110d dddd
	{DTI_PRESSURE,			0,	0,	4,	0,	1},		// 1110 dddd dddd dddd
	{DTI_DEPTH,				0,	0,	5,	0,	1},		// 1111 0ddd dddd dddd
	{DTI_TEMPERATURE,		0,	0,	6,	0,	1},		// 1111 10dd dddd dddd
	{DTI_ALARMS,			1,	0,	7,	1,	1},		// 1111 110d dddd dddd
	{DTI_TIME,				1,	0,	8,	0,	1},		// 1111 1110 dddd dddd
	{DTI_DEPTH,				1,	0,	9,	1,	2},		// 1111 1111 0ddd dddd dddd dddd dddd dddd
	{DTI_TEMPERATURE,		1,	0,	10,	1,	2},		// 1111 1111 10dd dddd dddd dddd dddd dddd
	{DTI_PRESSURE,			1,	0,	11,	1,	2},		// 1111 1111 110d dddd dddd dddd dddd dddd
	{DTI_PRESSURE,			1,	1,	12,	1,	2},		// 1111 1111 1110 dddd dddd dddd dddd dddd
	{DTI_PRESSURE,			1,	2,	13,	1,	2},		// 1111 1111 1111 0ddd dddd dddd dddd dddd
	{DTI_RBT,				1,	0,	14,	1,	1},		// 1111 1111 1111 10dd dddd dddd
};

int smart_parser_create(parser_handle_t * abstract, dev_handle_t abstract_dev)
{
	smart_parser_t * parser = (smart_parser_t *)(abstract);
	struct smart_device_base_t * dev = (struct smart_device_base_t *)(abstract_dev);

	if ((parser == NULL) || (dev == NULL))
	{
		errno = EINVAL;
		return -1;
	}

	smart_parser_t p = (smart_parser_t)malloc(sizeof(struct smart_parser_));
	if (p == NULL)
		return -1;

	p->dev = dev;

	switch (p->dev->model)
	{
	case MDL_SMART_PRO:
		p->hdr_size = 92;
		p->dti_size = sizeof(smart_pro_table) / sizeof(dti_entry_t);
		p->dti_table = smart_pro_table;
		p->alarm_size = 0;
		p->alarm_table = 0;
		break;

	case MDL_GALILEO_SOL:
		p->hdr_size = 152;
		p->dti_size = sizeof(smart_galileo_sol_table) / sizeof(dti_entry_t);
		p->dti_table = smart_galileo_sol_table;
		p->alarm_size = 0;
		p->alarm_table = 0;
		break;

	case MDL_ALADIN_TEC:
		p->hdr_size = 108;
		p->dti_size = sizeof(smart_aladin_table) / sizeof(dti_entry_t);
		p->dti_table = smart_aladin_table;
		p->alarm_size = 0;
		p->alarm_table = 0;
		break;

	case MDL_ALADIN_TEC2G:
		p->hdr_size = 116;
		p->dti_size = sizeof(smart_aladin_tec2g_table) / sizeof(dti_entry_t);
		p->dti_table = smart_aladin_tec2g_table;
		p->alarm_size = sizeof(smart_aladin_tec2g_alarms) / sizeof(alarm_entry_t);
		p->alarm_table = smart_aladin_tec2g_alarms;
		break;

	case MDL_SMART_COM:
		p->hdr_size = 100;
		p->dti_size = sizeof(smart_com_table) / sizeof(dti_entry_t);
		p->dti_table = smart_com_table;
		p->alarm_size = 0;
		p->alarm_table = 0;
		break;

	case MDL_SMART_TEC:
	case MDL_SMART_Z:
		p->hdr_size = 132;
		p->dti_size = sizeof(smart_tec_table) / sizeof(dti_entry_t);
		p->dti_table = smart_tec_table;
		p->alarm_size = 0;
		p->alarm_table = 0;
		break;

	default:
		p->dev->errcode = DRIVER_ERR_UNSUPPORTED;
		p->dev->errmsg = "Unsupported Model Number";
		free(p);
		return -1;

	}

	smart_parser_reset((parser_handle_t)p);

	*abstract = (parser_handle_t)p;
	return 0;
}

void smart_parser_close(parser_handle_t abstract)
{
	smart_parser_t parser = (smart_parser_t)(abstract);
	if (parser == NULL)
		return;

	free(parser);
}

int smart_parser_reset(parser_handle_t abstract)
{
	smart_parser_t parser = (smart_parser_t)(abstract);

	parser->time = 0;
	parser->depth = 0;
	parser->temp = 0;
	parser->pressure = 0;
	parser->rbt = 99;
	parser->heartrate = 0;
	parser->bearing = 0;
	parser->alarms[0] = 0;
	parser->alarms[1] = 0;
	parser->alarms[2] = 0;
	parser->tank = 0;
	parser->dcal = 0;

	parser->complete = 0;
	parser->calibrated = 0;

	parser->have_depth = 0;
	parser->have_temp = 0;
	parser->have_pressure = 0;
	parser->have_rbt = 0;
	parser->have_heartrate = 0;
	parser->have_bearing = 0;
	parser->have_alarms = 0;

	return 0;
}

void smart_parse_smart_pro_header(const unsigned char * buffer, int32_t tcorr, header_callback_fn_t cb, void * userdata)
{
	if (! cb)
		return;

	time_t dtime = (uint32_le(buffer, 8) + tcorr) / 2;

	cb(userdata,	DIVE_HEADER_START_TIME,		dtime,							0,	0);						// Dive Date/Time
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 16),			0,	"alarms");				// Alarms During Dive
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[17],						0,	"microbubble_level");	// Microbubble Level
	cb(userdata,	DIVE_HEADER_MAX_DEPTH,		uint16_le(buffer, 18),			0,	0);						// Maximum Depth
	cb(userdata,	DIVE_HEADER_DURATION,		uint16_le(buffer, 20),			0,	0);						// Duration
	cb(userdata,	DIVE_HEADER_MIN_TEMP,		uint16_le(buffer, 22) * 10,		0,	0);						// Minimum Temperature
	cb(userdata,	DIVE_HEADER_PMO2,			buffer[24] * 10,				0,	0);						// pmO2, Tank 1
	cb(userdata,	DIVE_HEADER_INTERVAL,		uint16_le(buffer, 26),			0,	0);						// Surface Interval
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 28),			0,	"cnsO2");				// CNS O2 Saturation
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 30),			0,	"altitude");			// Altitude Level
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 32),			0,	"ppO2_max");			// Max ppO2 Tank 1 (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 34),			0,	"depth_max");			// Depth Alarm Limit
	cb(userdata,	DIVE_HEADER_DESAT_BEFORE,	uint16_le(buffer, 38),			0,	0);						// Desaturation Time Before Dive
	cb(userdata, 	DIVE_HEADER_VENDOR,			uint16_le(buffer, 48),			0,	"mode");				// Mode Settings

	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 60),			0,	"cmp");					// Compartment 0
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 64),			1,	"cmp");					// Compartment 1
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 68),			2,	"cmp");					// Compartment 2
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 72),			3,	"cmp");					// Compartment 3
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 76),			4,	"cmp");					// Compartment 4
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 80),			5,	"cmp");					// Compartment 5
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 84),			6,	"cmp");					// Compartment 6
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 88),			7,	"cmp");					// Compartment 7
}

void smart_parse_galileo_sol_header(const unsigned char * buffer, int32_t tcorr, header_callback_fn_t cb, void * userdata)
{
	if (! cb)
		return;

	int16_t utcoff = ((char *)buffer)[16];
	time_t dtime = (uint32_le(buffer, 8) + tcorr + utcoff * 1800) / 2;

	cb(userdata,	DIVE_HEADER_START_TIME,		dtime,							0,	0);						// Dive Date/Time
	cb(userdata,	DIVE_HEADER_UTC_OFFSET,		utcoff * 15,					0,	0);						// UTC Offset
	cb(userdata,	DIVE_HEADER_REPETITION,		buffer[17],						0,	0);						// Repetition Number
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[18],						0,	"microbubble_level");	// Microbubble Level
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[19],						0, 	"battery");				// Battery Level
	cb(userdata,	DIVE_HEADER_MAX_DEPTH,		uint16_le(buffer, 22),			0,	0);						// Maximum Depth
	cb(userdata,	DIVE_HEADER_AVG_DEPTH,		uint16_le(buffer, 24),			0,	0);						// Average Depth
	cb(userdata,	DIVE_HEADER_DURATION,		uint16_le(buffer, 26),			0,	0);						// Duration
	cb(userdata,	DIVE_HEADER_MAX_TEMP,		uint16_le(buffer, 28) * 10,		0,	0);						// Maximum Temperature
	cb(userdata,	DIVE_HEADER_MIN_TEMP,		uint16_le(buffer, 30) * 10,		0,	0);						// Minimum Temperature
	cb(userdata,	DIVE_HEADER_AIR_TEMP,		uint16_le(buffer, 32) * 10,		0,	0);						// Air Temperature
	cb(userdata,	DIVE_HEADER_INTERVAL,		uint16_le(buffer, 38),			0,	0);						// Surface Interval
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 40),			0,	"cnsO2");				// CNS O2 Saturation
	cb(userdata,	DIVE_HEADER_PMO2,			uint16_le(buffer, 44) * 10,		0,	0);						// pmO2, Tank 1
	cb(userdata, 	DIVE_HEADER_PMO2,			uint16_le(buffer, 46) * 10,		1,	0);						// pmO2, Tank 2
	cb(userdata,	DIVE_HEADER_PMO2,			uint16_le(buffer, 48) * 10,		2,	0);						// pmO2, Tank d
	cb(userdata,	DIVE_HEADER_PX_END,			uint16_le(buffer, 50) * 125/16,	0,	0);						// End Pressure, Tank 1
	cb(userdata,	DIVE_HEADER_PX_END,			uint16_le(buffer, 52) * 125/16,	1,	0);						// End Pressure, Tank 2
	cb(userdata,	DIVE_HEADER_PX_END,			uint16_le(buffer, 54) * 125/16,	2,	0);						// End Pressure, Tank d
	cb(userdata,	DIVE_HEADER_PX_START,		uint16_le(buffer, 56) * 125/16,	0,	0);						// Start Pressure, Tank 1
	cb(userdata,	DIVE_HEADER_PX_START,		uint16_le(buffer, 58) * 125/16,	1,	0);						// Start Pressure, Tank 2
	cb(userdata,	DIVE_HEADER_PX_START,		uint16_le(buffer, 60) * 125/16,	2,	0);						// Start Pressure, Tank d
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 66),			0,	"ppO2_max");			// Max ppO2 Tank 1 (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 68),			1,	"ppO2_max");			// Max ppO2 Tank 2 (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 70),			2,	"ppO2_max");			// Max ppO2 Tank d (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 74),			0,	"altitude");			// Altitude Level
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 78),			0,	"depth_max");			// Depth Alarm Limit
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 80),			0,	"time_max");			// Bottom Time Alarm Limit
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 82),			0,	"pressure_min");		// Tank Pressure Alarm Limit
	cb(userdata,	DIVE_HEADER_DESAT_BEFORE,	uint16_le(buffer, 88),			0,	0);						// Desaturation Time Before Dive

	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 120),			0,	"cmp");					// Compartment 0
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 124),			1,	"cmp");					// Compartment 1
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 128),			2,	"cmp");					// Compartment 2
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 132),			3,	"cmp");					// Compartment 3
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 136),			4,	"cmp");					// Compartment 4
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 140),			5,	"cmp");					// Compartment 5
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 144),			6,	"cmp");					// Compartment 6
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 148),			7,	"cmp");					// Compartment 7
}

void smart_parse_aladin_tec_header(const unsigned char * buffer, int32_t tcorr, header_callback_fn_t cb, void * userdata)
{
	if (! cb)
		return;

	int16_t utcoff = ((char *)buffer)[16];
	time_t dtime = (uint32_le(buffer, 8) + tcorr + utcoff * 1800) / 2;

	cb(userdata,	DIVE_HEADER_START_TIME,		dtime,							0,	0);						// Dive Date/Time
	cb(userdata,	DIVE_HEADER_UTC_OFFSET,		utcoff * 15,					0,	0);						// UTC Offset
	cb(userdata,	DIVE_HEADER_REPETITION,		buffer[17],						0,	0);						// Repetition Number
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[18],						0,	"microbubble_level");	// Microbubble Level
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 20),			0,	"alarms");				// Alarms During Dive
	cb(userdata,	DIVE_HEADER_MAX_DEPTH,		uint16_le(buffer, 22),			0,	0);						// Maximum Depth
	cb(userdata,	DIVE_HEADER_DURATION,		uint16_le(buffer, 24),			0,	0);						// Duration
	cb(userdata,	DIVE_HEADER_MIN_TEMP,		uint16_le(buffer, 26) * 10,		0,	0);						// Maximum Temperature
	cb(userdata,	DIVE_HEADER_MAX_TEMP,		uint16_le(buffer, 28) * 10,		0,	0);						// Minimum Temperature
	cb(userdata,	DIVE_HEADER_PMO2,			uint16_le(buffer, 30) * 10,		0,	0);						// pmO2, Tank 1
	cb(userdata,	DIVE_HEADER_AIR_TEMP,		uint16_le(buffer, 32) * 10,		0,	0);						// Air Temperature
	cb(userdata,	DIVE_HEADER_INTERVAL,		uint16_le(buffer, 34),			0,	0);						// Surface Interval
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 36),			0,	"cnsO2");				// CNS O2 Saturation
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 38),			0,	"altitude");			// Altitude Level
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 42),			0,	"ppO2_max");			// Max ppO2 Tank 1 (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 44),			0,	"depth_max");			// Depth Alarm Limit
	cb(userdata,	DIVE_HEADER_DESAT_BEFORE,	uint16_le(buffer, 48),			0,	0);						// Desaturation Time Before Dive

	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 76),			0,	"cmp");					// Compartment 0
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 80),			1,	"cmp");					// Compartment 1
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 84),			2,	"cmp");					// Compartment 2
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 88),			3,	"cmp");					// Compartment 3
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 92),			4,	"cmp");					// Compartment 4
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 96),			5,	"cmp");					// Compartment 5
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 100),			6,	"cmp");					// Compartment 6
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 104),			7,	"cmp");					// Compartment 7
}

void smart_parse_aladin_tec2g_header(const unsigned char * buffer, int32_t tcorr, header_callback_fn_t cb, void * userdata)
{
	if (! cb)
		return;

	int16_t utcoff = ((char *)buffer)[16];
	time_t dtime = (uint32_le(buffer, 8) + tcorr + utcoff * 1800) / 2;

	cb(userdata,	DIVE_HEADER_START_TIME,		dtime,							0,	0);						// Dive Date/Time
	cb(userdata,	DIVE_HEADER_UTC_OFFSET,		utcoff * 15,					0,	0);						// UTC Offset
	cb(userdata,	DIVE_HEADER_REPETITION,		buffer[17],						0,	0);						// Repetition Number
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[18],						0,	"microbubble_level");	// Microbubble Level
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 20),			0,	"alarms");				// Alarms During Dive
	cb(userdata,	DIVE_HEADER_MAX_DEPTH,		uint16_le(buffer, 22),			0,	0);						// Maximum Depth
	cb(userdata,	DIVE_HEADER_AVG_DEPTH,		uint16_le(buffer, 24),			0,	0);						// Average Depth
	cb(userdata,	DIVE_HEADER_DURATION,		uint16_le(buffer, 26),			0,	0);						// Duration
	cb(userdata,	DIVE_HEADER_MAX_TEMP,		uint16_le(buffer, 28) * 10,		0,	0);						// Maximum Temperature
	cb(userdata,	DIVE_HEADER_MIN_TEMP,		uint16_le(buffer, 30) * 10,		0,	0);						// Minimum Temperature
	cb(userdata,	DIVE_HEADER_AIR_TEMP,		uint16_le(buffer, 32) * 10,		0,	0);						// Air Temperature
	cb(userdata,	DIVE_HEADER_PMO2,			buffer[34] * 10,				0,	0);						// pmO2, Tank 1
	cb(userdata, 	DIVE_HEADER_PMO2,			buffer[35] * 10,				1,	0);						// pmO2, Tank 2
	cb(userdata,	DIVE_HEADER_PMO2,			buffer[36] * 10,				2,	0);						// pmO2, Tank d
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[37],						0, 	"battery");				// Battery Level
	cb(userdata,	DIVE_HEADER_INTERVAL,		uint16_le(buffer, 40),			0,	0);						// Surface Interval
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 42),			0,	"cnsO2");				// CNS O2 Saturation
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 46),			0,	"ppO2_max");			// Max ppO2 Tank 1 (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 48),			1,	"ppO2_max");			// Max ppO2 Tank 2 (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 50),			2,	"ppO2_max");			// Max ppO2 Tank d (ata)
	cb(userdata,	DIVE_HEADER_DESAT_BEFORE,	uint16_le(buffer, 56),			0,	0);						// Desaturation Time Before Dive
	cb(userdata,	DIVE_HEADER_NOFLY_BEFORE,	uint16_le(buffer, 58),			0,	0);						// No-Fly Time Before Dive
	cb(userdata, 	DIVE_HEADER_VENDOR,			uint32_le(buffer, 60),			0,	"mode");				// Mode Settings

	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 84),			0,	"cmp");					// Compartment 0
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 88),			1,	"cmp");					// Compartment 1
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 92),			2,	"cmp");					// Compartment 2
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 96),			3,	"cmp");					// Compartment 3
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 100),			4,	"cmp");					// Compartment 4
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 104),			5,	"cmp");					// Compartment 5
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 108),			6,	"cmp");					// Compartment 6
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 112),			7,	"cmp");					// Compartment 7
}

void smart_parse_smart_com_header(const unsigned char * buffer, int32_t tcorr, header_callback_fn_t cb, void * userdata)
{
	if (! cb)
		return;

	time_t dtime = (uint32_le(buffer, 8) + tcorr) / 2;

	cb(userdata,	DIVE_HEADER_START_TIME,		dtime,							0,	0);						// Dive Date/Time
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 16),			0,	"alarms");				// Alarms During Dive
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[17],						0,	"microbubble_level");	// Microbubble Level
	cb(userdata,	DIVE_HEADER_MAX_DEPTH,		uint16_le(buffer, 18),			0,	0);						// Maximum Depth
	cb(userdata,	DIVE_HEADER_DURATION,		uint16_le(buffer, 20),			0,	0);						// Duration
	cb(userdata,	DIVE_HEADER_MIN_TEMP,		uint16_le(buffer, 22) * 10,		0,	0);						// Minimum Temperature
	cb(userdata,	DIVE_HEADER_PMO2,			buffer[24] * 10,				0,	0);						// pmO2, Tank 1
	cb(userdata,	DIVE_HEADER_INTERVAL,		uint16_le(buffer, 26),			0,	0);						// Surface Interval
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 28),			0,	"cnsO2");				// CNS O2 Saturation
	cb(userdata,	DIVE_HEADER_PX_START,		uint16_le(buffer, 30) * 125/16,	0,	0);						// Start Pressure, Tank 1
	cb(userdata,	DIVE_HEADER_PX_END,			uint16_le(buffer, 32) * 125/16,	0,	0);						// End Pressure, Tank 1
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 34),			0,	"depth_max");			// Depth Alarm Limit
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 36),			0,	"pressure_min");		// Tank Pressure Alarm Limit
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 42),			0,	"altitude");			// Altitude Level
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 44),			0,	"ppO2_max");			// Max ppO2 Tank 1 (ata)
	cb(userdata, 	DIVE_HEADER_VENDOR,			uint32_le(buffer, 58),			0,	"mode");				// Mode Settings
}

void smart_parse_smart_tec_header(const unsigned char * buffer, int32_t tcorr, header_callback_fn_t cb, void * userdata)
{
	if (! cb)
		return;

	time_t dtime = (uint32_le(buffer, 8) + tcorr) / 2;

	cb(userdata,	DIVE_HEADER_START_TIME,		dtime,							0,	0);						// Dive Date/Time
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 16),			0,	"alarms");				// Alarms During Dive
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[17],						0,	"microbubble_level");	// Microbubble Level
	cb(userdata,	DIVE_HEADER_MAX_DEPTH,		uint16_le(buffer, 18),			0,	0);						// Maximum Depth
	cb(userdata,	DIVE_HEADER_DURATION,		uint16_le(buffer, 20),			0,	0);						// Duration
	cb(userdata,	DIVE_HEADER_MIN_TEMP,		uint16_le(buffer, 22) * 10,		0,	0);						// Minimum Temperature
	cb(userdata,	DIVE_HEADER_INTERVAL,		uint16_le(buffer, 24),			0,	0);						// Surface Interval
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 26),			0,	"cnsO2");				// CNS O2 Saturation
	cb(userdata,	DIVE_HEADER_PMO2,			uint16_le(buffer, 28) * 10,		0,	0);						// pmO2, Tank 1
	cb(userdata, 	DIVE_HEADER_PMO2,			uint16_le(buffer, 30) * 10,		1,	0);						// pmO2, Tank 2
	cb(userdata,	DIVE_HEADER_PMO2,			uint16_le(buffer, 32) * 10,		2,	0);						// pmO2, Tank d
	cb(userdata,	DIVE_HEADER_PX_START,		uint16_le(buffer, 34) * 125/16,	0,	0);						// Start Pressure, Tank 1
	cb(userdata,	DIVE_HEADER_PX_END,			uint16_le(buffer, 36) * 125/16,	0,	0);						// End Pressure, Tank 1
	cb(userdata,	DIVE_HEADER_PX_START,		uint16_le(buffer, 38) * 125/16,	1,	0);						// Start Pressure, Tank 2
	cb(userdata,	DIVE_HEADER_PX_END,			uint16_le(buffer, 40) * 125/16,	1,	0);						// End Pressure, Tank 2
	cb(userdata,	DIVE_HEADER_PX_START,		uint16_le(buffer, 42) * 125/16,	2,	0);						// Start Pressure, Tank d
	cb(userdata,	DIVE_HEADER_PX_END,			uint16_le(buffer, 44) * 125/16,	2,	0);						// End Pressure, Tank d
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 48),			0,	"depth_max");			// Depth Alarm Limit
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 50),			0,	"pressure_min");		// Tank Pressure Alarm Limit
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 56),			0,	"altitude");			// Altitude Level
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 58),			0,	"ppO2_max");			// Max ppO2 Tank 1 (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 60),			1,	"ppO2_max");			// Max ppO2 Tank 2 (ata)
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 62),			2,	"ppO2_max");			// Max ppO2 Tank d (ata)
	cb(userdata,	DIVE_HEADER_DESAT_BEFORE,	uint16_le(buffer, 80),			0,	0);						// Desaturation Time Before Dive
	cb(userdata, 	DIVE_HEADER_VENDOR,			uint32_le(buffer, 92),			0,	"mode");				// Mode Settings

	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 100),			0,	"cmp");					// Compartment 0
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 104),			1,	"cmp");					// Compartment 1
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 108),			2,	"cmp");					// Compartment 2
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 112),			3,	"cmp");					// Compartment 3
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 116),			4,	"cmp");					// Compartment 4
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 120),			5,	"cmp");					// Compartment 5
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 124),			6,	"cmp");					// Compartment 6
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 128),			7,	"cmp");					// Compartment 7
}

void smart_parse_smart_z_header(const unsigned char * buffer, int32_t tcorr, header_callback_fn_t cb, void * userdata)
{
	if (! cb)
		return;

	time_t dtime = (uint32_le(buffer, 8) + tcorr) / 2;

	cb(userdata,	DIVE_HEADER_START_TIME,		dtime,							0,	0);						// Dive Date/Time
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 16),			0,	"alarms");				// Alarms During Dive
	cb(userdata,	DIVE_HEADER_VENDOR,			buffer[17],						0,	"microbubble_level");	// Microbubble Level
	cb(userdata,	DIVE_HEADER_MAX_DEPTH,		uint16_le(buffer, 18),			0,	0);						// Maximum Depth
	cb(userdata,	DIVE_HEADER_DURATION,		uint16_le(buffer, 20),			0,	0);						// Duration
	cb(userdata,	DIVE_HEADER_MIN_TEMP,		uint16_le(buffer, 22) * 10,		0,	0);						// Minimum Temperature
	cb(userdata,	DIVE_HEADER_INTERVAL,		uint16_le(buffer, 24),			0,	0);						// Surface Interval
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 26),			0,	"cnsO2");				// CNS O2 Saturation
	cb(userdata,	DIVE_HEADER_PMO2,			uint16_le(buffer, 28) * 10,		0,	0);						// pmO2, Tank 1
	cb(userdata,	DIVE_HEADER_PX_START,		uint16_le(buffer, 34) * 125/16,	0,	0);						// Start Pressure, Tank 1
	cb(userdata,	DIVE_HEADER_PX_END,			uint16_le(buffer, 36) * 125/16,	0,	0);						// End Pressure, Tank 1
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 48),			0,	"depth_max");			// Depth Alarm Limit
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 50),			0,	"pressure_min");		// Tank Pressure Alarm Limit
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 56),			0,	"altitude");			// Altitude Level
	cb(userdata,	DIVE_HEADER_VENDOR,			uint16_le(buffer, 58),			0,	"ppO2_max");			// Max ppO2 Tank 1 (ata)
	cb(userdata,	DIVE_HEADER_DESAT_BEFORE,	uint16_le(buffer, 80),			0,	0);						// Desaturation Time Before Dive
	cb(userdata, 	DIVE_HEADER_VENDOR,			uint32_le(buffer, 92),			0,	"mode");				// Mode Settings

	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 100),			0,	"cmp");					// Compartment 0
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 104),			1,	"cmp");					// Compartment 1
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 108),			2,	"cmp");					// Compartment 2
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 112),			3,	"cmp");					// Compartment 3
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 116),			4,	"cmp");					// Compartment 4
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 120),			5,	"cmp");					// Compartment 5
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 124),			6,	"cmp");					// Compartment 6
	cb(userdata,	DIVE_HEADER_VENDOR,			uint32_le(buffer, 128),			7,	"cmp");					// Compartment 7
}

int smart_parser_parse_header(parser_handle_t abstract, const void * buffer, uint32_t size, header_callback_fn_t cb, void * userdata)
{
	smart_parser_t parser = (smart_parser_t)(abstract);
	if (parser == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if (size < parser->hdr_size)
	{
		parser->dev->errcode = DRIVER_ERR_INVALID;
		parser->dev->errmsg = "Data buffer is too short";
		return -1;
	}

	uint32_t psize =  uint32_le((const unsigned char *)buffer, 4);
	if (psize != size)
	{
		parser->dev->errcode = DRIVER_ERR_INVALID;
		parser->dev->errmsg = "Data buffer size does not match header-specified data size";
		return -1;
	}

	switch (parser->dev->model)
	{
	case MDL_SMART_PRO:
		smart_parse_smart_pro_header((const unsigned char *)buffer, parser->dev->tcorr, cb, userdata);
		break;

	case MDL_GALILEO_SOL:
		smart_parse_galileo_sol_header((const unsigned char *)buffer, parser->dev->tcorr, cb, userdata);
		break;

	case MDL_ALADIN_TEC:
		smart_parse_aladin_tec_header((const unsigned char *)buffer, parser->dev->tcorr, cb, userdata);
		break;

	case MDL_ALADIN_TEC2G:
		smart_parse_aladin_tec2g_header((const unsigned char *)buffer, parser->dev->tcorr, cb, userdata);
		break;

	case MDL_SMART_COM:
		smart_parse_smart_com_header((const unsigned char *)buffer, parser->dev->tcorr, cb, userdata);
		break;

	case MDL_SMART_TEC:
		smart_parse_smart_tec_header((const unsigned char *)buffer, parser->dev->tcorr, cb, userdata);
		break;

	case MDL_SMART_Z:
		smart_parse_smart_z_header((const unsigned char *)buffer, parser->dev->tcorr, cb, userdata);
		break;

	default:
		parser->dev->errcode = DRIVER_ERR_UNSUPPORTED;
		parser->dev->errmsg = "Unsupported Model Number";
		return -1;
	}

	return 0;
}

#define NBYTES	8
#define NBITS	8

static uint8_t smart_identify(const unsigned char * data, uint32_t size)
{
	uint8_t count = 0;
	uint8_t i;
	uint8_t j;

	for (i = 0; i < ((size > NBYTES) ? NBYTES : size); ++i)
	{
		uint8_t value = data[i];
		for (j = 0; j < NBITS; ++j)
		{
			uint8_t mask = (1 << (NBITS - 1 - j));
			if ((value & mask) == 0)
				return count;

			count++;
		}
	}

	return (uint8_t)(-1);
}

static uint8_t galileo_identify(const unsigned char value)
{
	if ((value & 0x80) == 0)
		return 0;

	if ((value & 0xE0) == 0x80)
		return 1;

	if ((value & 0xF0) != 0xF0)
		return ((value & 0x70) >> 4);

	return (value & 0x0F) + 7;
}

static uint32_t smart_fixsignbit(uint32_t x, uint32_t n)
{
	if ((n == 0) || (n > 32))
		return 0;

	uint32_t signbit = (1 << (n - 1));
	uint32_t mask = (0xFFFFFFFF << n);

	if ((x & signbit) == signbit)
		return x | mask;
	return x & ~mask;
}

int smart_process_dti(smart_parser_t parser, const unsigned char * data, uint32_t size,
		uint32_t * offset, const dti_entry_t * dti, uint32_t * value, int32_t * svalue)
{
	// Skip the Processed Type Bits
	*offset += dti->ntb / NBITS;

	// Process Remaining Bits
	uint8_t nbits = 0;
	uint8_t n = dti->ntb % NBITS;
	if (n > 0)
	{
		nbits = NBITS - n;
		*value = data[*offset] & (0xFF >> n);
		if (dti->ignore)
		{
			nbits = 0;
			*value = 0;
		}

		(*offset)++;
	}

	// Check for Overflow
	if (*offset + dti->extra > size)
	{
		parser->dev->errcode = DRIVER_ERR_INVALID;
		parser->dev->errmsg = "DTI extends past end of data buffer";
		return -1;
	}

	// Process Extra Bits
	for (uint8_t i = 0; i < dti->extra; ++i)
	{
		nbits += NBITS;
		(*value) <<= NBITS;
		(*value) += data[*offset];
		(*offset)++;
	}

	// Fix Sign Bit
	*svalue = (int32_t)smart_fixsignbit(*value, nbits);

	// Done with DTI Processing
	return 0;
}

int smart_parse_dti(smart_parser_t parser, const dti_entry_t * dti, uint32_t value, int32_t svalue)
{
	switch (dti->type)
	{
	case DTI_PRESSURE_DEPTH:
		parser->pressure += ((signed char)((svalue >> NBITS) & 0xFF)) * 250;
		parser->depth += ((signed char)(svalue & 0xFF)) * 2;
		parser->complete = 1;
		break;

	case DTI_RBT:
		if (dti->abs)
		{
			parser->rbt = value;
			parser->have_rbt = 1;
		}
		else
		{
			parser->rbt += svalue;
		}

		break;

	case DTI_TEMPERATURE:
		if (dti->abs)
		{
			parser->temp = value * 40;
			parser->have_temp = 1;
		}
		else
		{
			parser->temp += svalue * 40;
		}

		break;

	case DTI_PRESSURE:
		if (dti->abs)
		{
			parser->pressure = value * 250;
			parser->tank = dti->idx;
			parser->have_pressure = 1;
		}
		else
		{
			parser->pressure += svalue * 250;
		}

		break;

	case DTI_DEPTH:
		if (dti->abs)
		{
			parser->depth = value * 2;
			if (! parser->calibrated)
			{
				parser->calibrated = 1;
				parser->dcal = parser->depth;
			}

			parser->have_depth = 1;
		}
		else
		{
			parser->depth += svalue * 2;
		}

		parser->complete = 1;
		break;

	case DTI_HEARTRATE:
		if (dti->abs)
		{
			parser->heartrate = value;
			parser->have_heartrate = 1;
		}
		else
		{
			parser->heartrate += svalue;
		}

		break;

	case DTI_BEARING:
		parser->bearing = value;
		parser->have_bearing = 1;
		break;

	case DTI_ALARMS:
		parser->alarms[dti->idx] = value;
		parser->have_alarms = 1;
		break;

	case DTI_TIME:
		parser->complete = value;
		break;

	default:
		//WARNING("Unknown Sample Type");
		return -1;
	}

	return 0;
}

const char * alarm_name(smart_parser_t parser, uint8_t idx, uint8_t mask)
{
	if ((parser == NULL) || ! parser->alarm_size)
		return 0;

	int i;
	for (i = 0; i < parser->alarm_size; ++i)
	{
		if ((parser->alarm_table[i].idx == idx) && (parser->alarm_table[i].mask == mask))
			return parser->alarm_table[i].name;
	}

	return 0;
}

int smart_parser_parse_profile(parser_handle_t abstract, const void * buffer, uint32_t size, waypoint_callback_fn_t cb, void * userdata)
{
	smart_parser_t parser = (smart_parser_t)(abstract);
	if (parser == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if (size < parser->hdr_size)
	{
		parser->dev->errcode = DRIVER_ERR_INVALID;
		parser->dev->errmsg = "Data buffer is too short";
		return -1;
	}

	uint32_t psize =  uint32_le((const unsigned char *)buffer, 4);
	if (psize != size)
	{
		parser->dev->errcode = DRIVER_ERR_INVALID;
		parser->dev->errmsg = "Data buffer size does not match header-specified data size";
		return -1;
	}

	const unsigned char * data = (const unsigned char *)buffer;
	uint32_t offset = parser->hdr_size;
	while (offset < size)
	{
		// Find the DTI Entry
		uint8_t id = 0;
		if (parser->dev->model == MDL_GALILEO_SOL)
			id = galileo_identify(data[offset]);
		else
			id = smart_identify(data + offset, size - offset);

		if (id > parser->dti_size)
		{
			parser->dev->errcode = DRIVER_ERR_INVALID;
			parser->dev->errmsg = "Invalid DTI Code";
			return -1;
		}

		// Process the DTI
		uint32_t value = 0;
		int32_t svalue = 0;

		if (smart_process_dti(parser, data, size, & offset, & parser->dti_table[id], & value, & svalue) != 0)
			return -1;

		// Parse the DTI
		if (smart_parse_dti(parser, & parser->dti_table[id], value, svalue) != 0)
			//WARNING("Unknown DTI Type");
		{ }

		// Process the Sample
		while (parser->complete)
		{
			// Send the Waypoint Time
			if (cb) cb(userdata, DIVE_WAYPOINT_TIME, parser->time, 0, 0);

			// Send Depth Data
			if (parser->have_depth)
				if (cb) cb(userdata, DIVE_WAYPOINT_DEPTH, parser->depth - parser->dcal, 0, 0);

			// Send Temperature Data
			if (parser->have_temp)
				if (cb) cb(userdata, DIVE_WAYPOINT_TEMP, parser->temp, 0, 0);

			// Send Pressure Data
			if (parser->have_pressure)
				if (cb) cb(userdata, DIVE_WAYPOINT_PX, parser->pressure, parser->tank, 0);

			// Send RBT Data
			if (parser->have_rbt)
				if (cb) cb(userdata, DIVE_WAYPOINT_RBT, parser->rbt, 0, 0);

			// Send Heart Rate Data
			if (parser->have_heartrate)
				if (cb) cb(userdata, DIVE_WAYPOINT_HEARTRATE, parser->heartrate, 0, 0);

			// Send Bearing Data
			if (parser->have_bearing)
			{
				if (cb) cb(userdata, DIVE_WAYPOINT_BEARING, parser->bearing, 0, 0);
				parser->have_bearing = 0;
			}

			// Send Alarm Data
			if (parser->have_alarms)
			{
				for (uint8_t i = 0; i < 3; i++)
				{
					for (uint8_t j = 0; j < 9; j++)
					{
						uint16_t mask = (1 << j);
						if (((parser->alarms[i] & mask) == mask) && cb)
						{
							const char * aname = alarm_name(parser, i, mask);
							if (! aname)
							{
								char buf[20];
								sprintf(buf, "alarm%u-%u", i, j);
								cb(userdata, DIVE_WAYPOINT_ALARM, j, i, buf);
							}
							else
							{
								cb(userdata, DIVE_WAYPOINT_ALARM, j, i, aname);
							}
						}
					}
				}

				parser->have_alarms = 0;
			}

			// Done with Sample
			parser->time += 4;
			parser->complete--;
		}
	}

	return 0;
}
