/*
 * Copyright (C) 2014 Asymworks, LLC.  All Rights Reserved.
 *
 * Developed by: Asymworks, LLC <info@asymworks.com>
 * 				 http://www.asymworks.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimers.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimers in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of Asymworks, LLC, nor the names of its contributors
 *      may be used to endorse or promote products derived from this Software
 *      without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

/**
 * @file src/transferapp/output_csv.cpp
 * @brief CSV Data Formatter
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <list>
#include <map>
#include <string>

#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "output_fmt.h"
#include "output_csv.h"

//! Tank Pressure Data
typedef struct tank_t_
{
	uint32_t		px_begin;		///< Beginning Pressure (millibar)
	uint32_t		px_end;			///< Ending Pressure (millibar)
	uint16_t		pmO2;			///< Mix Oxygen Ratio (permille)
	uint16_t		pmHe;			///< Mix Helium Ratio (permille)

	tank_t_()
		: px_begin(0), px_end(0), pmO2(209), pmHe(0)
	{
	}

} tank_t;

//! Waypoint Data
typedef struct
{
	int						valid;			///< Waypoint Valid
	uint32_t				time;			///< Waypoint Time (seconds)
	uint32_t				depth;			///< Waypoint Depth (centimeter)
	uint32_t				temp;			///< Waypoint Temp (centidegrees C)
	uint32_t				px;				///< Waypoint Pressure (millibar)

	std::list<std::string>	alarms;			///< Waypoint Alarms

	int						tank_id;		///< Waypoint Tank Id

} csv_wp_data;

//! CSV Formatter Data
typedef struct
{
	FILE *					fp;				///< Output File

	std::map<int, tank_t>	cur_tanks;		///< Current Profile Tanks
	csv_wp_data				cur_wp;			///< Current Waypoint

	int						hdr_complete;	///< Flag if Header is Complete

} csv_fmt_data;

/* Dispose of Data Formatter Structure */
void csv_dispose_formatter(output_fmt_data_t s)
{
	csv_fmt_data * fmt_data;

	if (! s || (s->magic != CSV_FMT_MAGIC))
		return;

	fmt_data = static_cast<csv_fmt_data *>(s->fmt_data);

	/* Delete Formatter Data */
	delete fmt_data;
}

/* Close the Data Formatter File */
int csv_close_formatter(output_fmt_data_t s)
{
	csv_fmt_data * fmt_data;

	if (! s || (s->magic != CSV_FMT_MAGIC))
		return EINVAL;

	fmt_data = static_cast<csv_fmt_data *>(s->fmt_data);

	/* Close the Output File */
	if (s->output_file)
		fclose(fmt_data->fp);

	/* Success */
	return 0;
}

/* Prolog Function */
int csv_prolog(output_fmt_data_t s)
{
	csv_fmt_data * fmt_data;

	if (! s || (s->magic != CSV_FMT_MAGIC))
		return EINVAL;

	fmt_data = static_cast<csv_fmt_data *>(s->fmt_data);

	/* Clear Profile Data */
	fmt_data->cur_tanks.clear();
	fmt_data->cur_wp.valid = 0;
	fmt_data->cur_wp.time = 0;
	fmt_data->cur_wp.depth = 0;
	fmt_data->cur_wp.temp = 0;
	fmt_data->cur_wp.px = 0;
	fmt_data->cur_wp.alarms.clear();
	fmt_data->cur_wp.tank_id = 0;

	/* Begin the Dive with a Header Line */
	fprintf(fmt_data->fp, "[DIVE HEADER]\n");
	fprintf(fmt_data->fp, "Name,Value\n");

	return 0;
}

/* Epilog Function */
int csv_epilog(output_fmt_data_t s)
{
	csv_fmt_data * fmt_data;

	if (! s || (s->magic != CSV_FMT_MAGIC))
		return EINVAL;

	fmt_data = static_cast<csv_fmt_data *>(s->fmt_data);

	/* End the Dive with a Blank Line */
	fprintf(fmt_data->fp, "\n");

	return 0;
}

/* Initialize Data Formatter Structure */
int csv_init_formatter(output_fmt_data_t s)
{
	csv_fmt_data * fmt_data;

	if (! s || s->magic)
		return EINVAL;

	/* Set Magic Number to identify as CSV Data */
	s->magic = CSV_FMT_MAGIC;

	/* Setup CSV Parser Callbacks */
	s->header_cb = csv_header_cb;
	s->profile_cb = csv_waypoint_cb;

	s->close_fn = csv_close_formatter;
	s->dispose_fn = csv_dispose_formatter;
	s->prolog_fn = csv_prolog;
	s->epilog_fn = csv_epilog;

	/* Create the CSV Formatter Data */
	fmt_data = new csv_fmt_data;
	if (! fmt_data)
		return ENOMEM;

	/* Open the Output File */
	if (! s->output_file)
		fmt_data->fp = stdout;
	else
		fmt_data->fp = fopen(s->output_file, "w");

	if (! fmt_data->fp)
	{
		delete fmt_data;
		return errno;
	}

	/* Store Formatter Data */
	s->fmt_data = fmt_data;

	/* Success */
	return 0;
}

/* Parser Callback for Header Data */
void csv_header_cb(void * arg, uint8_t token, int32_t value, uint8_t index, const char * name)
{
	output_fmt_data_t cb_data = static_cast<output_fmt_data_t>(arg);
	csv_fmt_data * fmt_data;

	time_t st;
	char buf[256];

	if (! cb_data || (cb_data->magic != CSV_FMT_MAGIC))
		return;

	fmt_data = static_cast<csv_fmt_data *>(cb_data->fmt_data);

	/* Check if Processing Header */
	if (! cb_data->output_header)
		return;

	/* Parse Token */
	switch (token)
	{
	case DIVE_HEADER_START_TIME:
	{
		st = (time_t)value;
		struct tm * tmp;
		tmp = gmtime(& st);
		strftime(buf, 255, "%Y-%m-%dT%H:%M:%S", tmp);
		fprintf(fmt_data->fp, "datetime,%s\n", buf);
		break;
	}

	case DIVE_HEADER_UTC_OFFSET:
	{
		fprintf(fmt_data->fp, "utc_offset,%d\n", value);
		break;
	}

	case DIVE_HEADER_DURATION:
	{
		fprintf(fmt_data->fp, "dive_duration,%d\n", value);
		break;
	}

	case DIVE_HEADER_INTERVAL:
	{
		fprintf(fmt_data->fp, "surface_interval,%d\n", value);
		break;
	}

	case DIVE_HEADER_REPETITION:
	{
		fprintf(fmt_data->fp, "repetition,%d\n", value);
		break;
	}

	case DIVE_HEADER_DESAT_BEFORE:
	{
		fprintf(fmt_data->fp, "desat_before,%d\n", value);
		break;
	}

	case DIVE_HEADER_DESAT_AFTER:
	{
		fprintf(fmt_data->fp, "desat_after,%d\n", value);
		break;
	}

	case DIVE_HEADER_NOFLY_BEFORE:
	{
		fprintf(fmt_data->fp, "nofly_before,%d\n", value);
		break;
	}

	case DIVE_HEADER_NOFLY_AFTER:
	{
		fprintf(fmt_data->fp, "nofly_after,%d\n", value);
		break;
	}

	case DIVE_HEADER_MAX_DEPTH:
	{
		fprintf(fmt_data->fp, "max_depth,%.2f\n", value / 100.0);
		break;
	}

	case DIVE_HEADER_AVG_DEPTH:
	{
		fprintf(fmt_data->fp, "avg_depth,%.2f\n", value / 100.0);
		break;
	}

	case DIVE_HEADER_AIR_TEMP:
	{
		fprintf(fmt_data->fp, "air_temp,%.2f\n", value / 100.0);
		break;
	}

	case DIVE_HEADER_MAX_TEMP:
	{
		fprintf(fmt_data->fp, "max_temp,%.2f\n", value / 100.0);
		break;
	}

	case DIVE_HEADER_MIN_TEMP:
	{
		fprintf(fmt_data->fp, "min_temp,%.2f\n", value / 100.0);
		break;
	}

	case DIVE_HEADER_PX_START:
	{
		if (fmt_data->cur_tanks.find(index) == fmt_data->cur_tanks.end())
			fmt_data->cur_tanks.insert(std::pair<int, tank_t>(index, tank_t()));

		fmt_data->cur_tanks[index].px_begin = value;
		break;
	}

	case DIVE_HEADER_PX_END:
	{
		if (fmt_data->cur_tanks.find(index) == fmt_data->cur_tanks.end())
			fmt_data->cur_tanks.insert(std::pair<int, tank_t>(index, tank_t()));

		fmt_data->cur_tanks[index].px_end = value;
		break;
	}

	case DIVE_HEADER_PMO2:
	{
		if (! value)
			break;

		if (fmt_data->cur_tanks.find(index) == fmt_data->cur_tanks.end())
			fmt_data->cur_tanks.insert(std::pair<int, tank_t>(index, tank_t()));

		fmt_data->cur_tanks[index].pmO2 = value;
		break;
	}

	case DIVE_HEADER_PMHe:
	{
		if (! value)
			break;

		if (fmt_data->cur_tanks.find(index) == fmt_data->cur_tanks.end())
			fmt_data->cur_tanks.insert(std::pair<int, tank_t>(index, tank_t()));

		fmt_data->cur_tanks[index].pmHe = value;
		break;
	}

	case DIVE_HEADER_VENDOR:
	{
		fprintf(fmt_data->fp, "vendor_%s%u,%u\n", name, index, value);
		break;
	}
	}
}

/* Parser Callback for Waypoint Data */
void csv_waypoint_cb(void * arg, uint8_t token, int32_t value, uint8_t index, const char * name)
{
	output_fmt_data_t cb_data = static_cast<output_fmt_data_t>(arg);
	csv_fmt_data *	fmt_data;

	if (! cb_data || (cb_data->magic != CSV_FMT_MAGIC))
		return;

	fmt_data = static_cast<csv_fmt_data *>(cb_data->fmt_data);

	/* Check if the Header is Complete */
	if (! fmt_data->hdr_complete)
	{
		if (cb_data->output_header)
		{
			std::map<int, tank_t>::const_iterator it;

			fprintf(fmt_data->fp, "[TANKS]\n");
			fprintf(fmt_data->fp, "Index,Start Pressure,End Pressure,%%O2,%%He\n");

			for (it = fmt_data->cur_tanks.begin(); it != fmt_data->cur_tanks.end(); it++)
			{
				fprintf(fmt_data->fp, "%d,%u,%u,%.1f,%.1f\n",
						it->first,
						it->second.px_begin,
						it->second.px_end,
						it->second.pmO2 / 10.0,
						it->second.pmHe / 10.0
				);
			}
		}

		if (cb_data->output_profile)
		{
			fprintf(fmt_data->fp, "[PROFILE]\n");
			fprintf(fmt_data->fp, "Time,Depth,Temp,Tank,Pressure,Alarms\n");
		}

		fmt_data->hdr_complete = 1;
	}

	/* Check if Processing Profile */
	if (! cb_data->output_profile)
		return;

	/* Parse Token */
	switch (token)
	{
	case DIVE_WAYPOINT_TIME:
	{
		if (fmt_data->cur_wp.valid)
		{
			std::string alarms;
			std::list<std::string>::const_iterator it;

			for (it = fmt_data->cur_wp.alarms.begin(); it != fmt_data->cur_wp.alarms.end(); it++)
			{
				if (it == fmt_data->cur_wp.alarms.begin())
					alarms = alarms + (* it);
				else
					alarms = alarms + "," + (* it);
			}

			fprintf(fmt_data->fp, "%u,%.1f,%.1f,%s\n",
					fmt_data->cur_wp.time,
					(int32_t)fmt_data->cur_wp.depth / 100.0,
					(int32_t)fmt_data->cur_wp.temp / 100.0,
					alarms.c_str()
			);

			fmt_data->cur_wp.alarms.clear();
		}

		fmt_data->cur_wp.time = value;
		fmt_data->cur_wp.valid = 1;
		break;
	}

	case DIVE_WAYPOINT_DEPTH:
	{
		fmt_data->cur_wp.depth = value;
		break;
	}

	case DIVE_WAYPOINT_TEMP:
	{
		fmt_data->cur_wp.temp = value;
		break;
	}

	case DIVE_WAYPOINT_ALARM:
	{
		fmt_data->cur_wp.alarms.push_back(name);
		break;
	}
	}
}
