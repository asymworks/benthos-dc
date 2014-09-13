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
 * @file src/transferapp/output_uddf.cpp
 * @brief UDDF Data Formatter
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <cerrno>
#include <map>
#include <string>
#include <utility>

#include <libxml/tree.h>

#include <benthos/divecomputer/config.h>

#include "output_fmt.h"
#include "output_uddf.h"

typedef std::pair<uint16_t, uint16_t>	mix_t;		///< Gas Mix Data
typedef std::map<std::string, mix_t>	mix_list_t;	///< Gas Mix List

//! UDDF Formatter Data
typedef struct
{
	xmlDoc *						doc;			///< XML Document
	xmlNode *						root;			///< XML Root Node
	xmlNode *						profiles;		///< UDDF Profiles Node
	xmlNode *						gasdef;			///< UDDF Gas Definitions Node
	xmlNode *						repgrp;			///< UDDF Repetition Group Node

	mix_list_t						mixes;			///< List of Gas Mixes
	std::map<uint8_t, xmlNode *>	tanks;			///< List of Tank Nodes

	mix_list_t						cur_mixes;		///< Current Profile Mixes
	xmlNode *						cur_profile;	///< Current Profile Node
	xmlNode *						cur_waypoint;	///< Current Waypoint Node
	xmlNode *						appdata;		///< Application Data Node
	xmlNode *						samples;		///< Samples Node

	xmlNode *						before;			///< Information Before Dive Node
	xmlNode *						after;			///< Information After Dive Node

	std::string						ts;				///< Date/Time Stamp
	int								rgid;			///< Repetition Group Id
	int								did;			///< Dive Id

} uddf_fmt_data;

/* Dispose of Data Formatter Structure */
void uddf_dispose_formatter(output_fmt_data_t s)
{
	uddf_fmt_data * fmt_data;

	if (! s || (s->magic != UDDF_FMT_MAGIC))
		return;

	fmt_data = static_cast<uddf_fmt_data *>(s->fmt_data);

	/* Cleanup XML Data */
	xmlFreeDoc(fmt_data->doc);
	xmlCleanupParser();

	/* Delete Formatter Data */
	delete fmt_data;
}

/* Close the Data Formatter File */
int uddf_close_formatter(output_fmt_data_t s)
{
	uddf_fmt_data * fmt_data;
	mix_list_t::const_iterator it;
	std::string outfile(s->output_file);

	if (! s || (s->magic != UDDF_FMT_MAGIC))
		return EINVAL;

	fmt_data = static_cast<uddf_fmt_data *>(s->fmt_data);

	/* Process Mixes */
	for (it = fmt_data->mixes.begin(); it != fmt_data->mixes.end(); it++)
	{
		xmlNode * mixnode = xmlNewChild(fmt_data->gasdef, 0, BAD_CAST("mix"), 0);
		xmlNewProp(mixnode, BAD_CAST("id"), BAD_CAST(it->first.c_str()));
		uint16_t o2 = it->second.first;
		uint16_t he = it->second.second;
		uint16_t n2 = 1000 - (o2 + he);

		char _o2[10];	snprintf(_o2, 8, "0.%03u", o2);
		char _he[10];	snprintf(_he, 8, "0.%03u", he);
		char _n2[10];	snprintf(_n2, 8, "0.%03u", n2);

		xmlNewChild(mixnode, NULL, BAD_CAST "o2", BAD_CAST(_o2));
		xmlNewChild(mixnode, NULL, BAD_CAST "n2", BAD_CAST(_n2));
		xmlNewChild(mixnode, NULL, BAD_CAST "he", BAD_CAST(_he));
		xmlNewChild(mixnode, NULL, BAD_CAST "ar", BAD_CAST("0.000"));
		xmlNewChild(mixnode, NULL, BAD_CAST "h2", BAD_CAST("0.000"));
	}

	/* Close the XML File */
	xmlSaveFormatFileEnc(outfile.empty() ? "-" : outfile.c_str(), fmt_data->doc, "UTF-8", 1);

	/* Success */
	return 0;
}

/* Prolog Function */
int uddf_prolog(output_fmt_data_t s)
{
	output_fmt_data_t cb_data = static_cast<output_fmt_data_t>(s);
	uddf_fmt_data *	fmt_data;

	time_t st;
	char buf[256];

	if (! cb_data || (cb_data->magic != UDDF_FMT_MAGIC))
		return EINVAL;

	fmt_data = static_cast<uddf_fmt_data *>(cb_data->fmt_data);

	/* Clear Previous Dive Data */
	fmt_data->cur_profile = 0;
	fmt_data->cur_waypoint = 0;
	fmt_data->cur_mixes.clear();

	fmt_data->tanks.clear();

	/* Create new Profile Entry */
	snprintf(buf, 255, "benthos_dive_%s_%03d", fmt_data->ts.c_str(), fmt_data->did++);
	fmt_data->cur_profile = xmlNewNode(0, BAD_CAST("dive"));
	xmlNewProp(fmt_data->cur_profile, BAD_CAST("id"), BAD_CAST(buf));

	xmlNode * appdata = xmlNewChild(fmt_data->cur_profile, 0, BAD_CAST("applicationdata"), 0);
	fmt_data->appdata = xmlNewChild(appdata, 0, BAD_CAST("benthos"), NULL);

	fmt_data->before = xmlNewChild(fmt_data->cur_profile, 0, BAD_CAST("informationbeforedive"), 0);
	fmt_data->after = xmlNewChild(fmt_data->cur_profile, 0, BAD_CAST("informationafterdive"), 0);

	fmt_data->samples = xmlNewChild(fmt_data->cur_profile, 0, BAD_CAST("samples"), 0);

	return 0;
}

/* Epilog Function */
int uddf_epilog(output_fmt_data_t s)
{
	output_fmt_data_t cb_data = static_cast<output_fmt_data_t>(s);
	uddf_fmt_data *	fmt_data;

	std::map<uint8_t, xmlNode *>::iterator idx;

	if (! cb_data || (cb_data->magic != UDDF_FMT_MAGIC))
		return EINVAL;

	fmt_data = static_cast<uddf_fmt_data *>(cb_data->fmt_data);

	/* Link the Profile to the Repetition Group */
	xmlAddChild(fmt_data->repgrp, fmt_data->cur_profile);

	/* Process Tanks and Mixes */
	for (idx = fmt_data->tanks.begin(); idx != fmt_data->tanks.end(); idx++)
	{
		char buf[256];
		snprintf(buf, 255, "mix-%u", idx->first);
		mix_list_t::iterator dive_mix = fmt_data->cur_mixes.find(buf);
		if (dive_mix == fmt_data->cur_mixes.end())
		{
			// Assume Air
			xmlNode * mn = xmlNewChild(idx->second, NULL, BAD_CAST "link", NULL);
			xmlNewProp(mn, BAD_CAST "ref", BAD_CAST "air");
		}
		else
		{
			uint16_t o2 = dive_mix->second.first;
			uint16_t he = dive_mix->second.second;

			// See if the mix already exists
			mix_list_t::iterator curmix;
			for (curmix = fmt_data->mixes.begin(); curmix != fmt_data->mixes.end(); curmix++)
				if ((curmix->second.first == o2) && (curmix->second.second == he))
					break;

			if (curmix == fmt_data->mixes.end())
			{
				// Add a new mix
				snprintf(buf, 255, "mix-%lu", fmt_data->mixes.size());
				fmt_data->mixes.insert(std::pair<std::string, mix_t>(buf, mix_t(o2, he)));
				xmlNode * mn = xmlNewChild(idx->second, NULL, BAD_CAST "link", NULL);
				xmlNewProp(mn, BAD_CAST "ref", BAD_CAST buf);
			}
			else
			{
				// Link to the existing mix
				xmlNode * mn = xmlNewChild(idx->second, NULL, BAD_CAST "link", NULL);
				xmlNewProp(mn, BAD_CAST "ref", BAD_CAST curmix->first.c_str());
			}
		}
	}

	return 0;
}

/* Initialize Data Formatter Structure */
int uddf_init_formatter(output_fmt_data_t s)
{
	uddf_fmt_data * fmt_data;
	xmlNode * generator;
	xmlNode * gasdef;
	xmlNode * profiles;

	char buf[255];
	time_t t = time(NULL);
	struct tm * tm;

	if (! s || s->magic)
		return EINVAL;

	/* Set Magic Number to identify as UDDF Data */
	s->magic = UDDF_FMT_MAGIC;

	/* Setup UDDF Parser Callbacks */
	s->header_cb = uddf_header_cb;
	s->profile_cb = uddf_waypoint_cb;

	s->close_fn = uddf_close_formatter;
	s->dispose_fn = uddf_dispose_formatter;
	s->prolog_fn = uddf_prolog;
	s->epilog_fn = uddf_epilog;

	/* Create the UDDF Formatter Data */
	fmt_data = new uddf_fmt_data;
	if (! fmt_data)
		return ENOMEM;

	/* Initialize UDDF Document */
	fmt_data->doc = xmlNewDoc(BAD_CAST("1.0"));
	fmt_data->root = xmlNewNode(0, BAD_CAST("uddf"));
	xmlDocSetRootElement(fmt_data->doc, fmt_data->root);
	xmlNewProp(fmt_data->root, BAD_CAST("version"), BAD_CAST("3.0.0"));

	/* Initialize Generator Data */
	xmlNewChild(generator, 0, BAD_CAST("name"), BAD_CAST("benthos-dc"));
	xmlNewChild(generator, 0, BAD_CAST("version"), BAD_CAST(BENTHOS_DC_VERSION_STRING));

	tm = localtime(& t);
	strftime(buf, 250, "%Y-%m-%d", tm);
	xmlNewChild(generator, 0, BAD_CAST("datetime"), BAD_CAST(buf));

	/* Store Timestamp for Dive IDs */
	strftime(buf, 250, "%Y%m%dT%H%m%S", tm);
	fmt_data->ts = std::string(buf);

	/* Setup Gas and Profile Nodes */
	fmt_data->gasdef = xmlNewChild(fmt_data->root, 0, BAD_CAST("gasdefinitions"), 0);
	fmt_data->profiles = xmlNewChild(fmt_data->root, 0, BAD_CAST("profiledata"), 0);

	/* Initialize Standard Mixes */
	fmt_data->mixes.insert(std::pair<std::string, mix_t>("air", mix_t(210, 0)));
	fmt_data->mixes.insert(std::pair<std::string, mix_t>("ean32", mix_t(320, 0)));
	fmt_data->mixes.insert(std::pair<std::string, mix_t>("ean36", mix_t(360, 0)));

	/* Initialize Remaining Elements */
	fmt_data->repgrp = 0;
	fmt_data->rgid = 0;
	fmt_data->did = 0;

	/* Store Formatter Data */
	s->fmt_data = fmt_data;

	/* Success */
	return 0;
}

/* Parser Callback for Header Data */
void uddf_header_cb(void * arg, uint8_t token, int32_t value, uint8_t index, const char * name)
{
	output_fmt_data_t cb_data = static_cast<output_fmt_data_t>(arg);
	uddf_fmt_data *	fmt_data;

	time_t st;
	char buf[256];

	if (! cb_data || (cb_data->magic != UDDF_FMT_MAGIC))
		return;

	fmt_data = static_cast<uddf_fmt_data *>(cb_data->fmt_data);

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
		xmlNewChild(fmt_data->before, 0, BAD_CAST "datetime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_UTC_OFFSET:
	{
		snprintf(buf, 255, "%d", value);
		xmlNewChild(fmt_data->appdata, NULL, BAD_CAST "utc_offset", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_DURATION:
	{
		snprintf(buf, 255, "%u", value * 60);
		xmlNewChild(fmt_data->after, NULL, BAD_CAST "diveduration", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_INTERVAL:
	{
		xmlNode * si = xmlNewChild(fmt_data->before, NULL, BAD_CAST "surfaceintervalbeforedive", NULL);
		if (value == 0)
		{
			xmlNewChild(si, NULL, BAD_CAST "infinity", NULL);
		}
		else
		{
			snprintf(buf, 255, "%d", value * 60);
			xmlNewChild(si, NULL, BAD_CAST "passedtime", BAD_CAST buf);
		}
		break;
	}

	case DIVE_HEADER_REPETITION:
	{
		if (value == 1)
		{
			snprintf(buf, 255, "benthos_repgroup_%s_%03d", fmt_data->ts.c_str(), fmt_data->rgid++);
			fmt_data->repgrp = xmlNewChild(fmt_data->profiles, NULL, BAD_CAST "repetitiongroup", NULL);
			xmlNewProp(fmt_data->repgrp, BAD_CAST "id", BAD_CAST buf);
		}
		break;
	}

	case DIVE_HEADER_DESAT_BEFORE:
	{
		snprintf(buf, 255, "%u", value * 60);
		xmlNewChild(fmt_data->before, NULL, BAD_CAST "desaturationtime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_DESAT_AFTER:
	{
		snprintf(buf, 255, "%u", value * 60);
		xmlNewChild(fmt_data->after, NULL, BAD_CAST "desaturationtime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_NOFLY_BEFORE:
	{
		snprintf(buf, 255, "%u", value * 60);
		xmlNewChild(fmt_data->before, NULL, BAD_CAST "noflighttime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_NOFLY_AFTER:
	{
		snprintf(buf, 255, "%u", value * 60);
		xmlNewChild(fmt_data->after, NULL, BAD_CAST "noflighttime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_MAX_DEPTH:
	{
		snprintf(buf, 255, "%.2f", value / 100.0);
		xmlNewChild(fmt_data->after, NULL, BAD_CAST "greatestdepth", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_AVG_DEPTH:
	{
		snprintf(buf, 255, "%.2f", value / 100.0);
		xmlNewChild(fmt_data->appdata, NULL, BAD_CAST "avg_depth", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_AIR_TEMP:
	{
		snprintf(buf, 255, "%.2f", value / 100.0 + 273.15);
		xmlNewChild(fmt_data->before, NULL, BAD_CAST "airtemperature", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_MAX_TEMP:
	{
		snprintf(buf, 255, "%.2f", value / 100.0);
		xmlNewChild(fmt_data->appdata, NULL, BAD_CAST "max_temp", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_MIN_TEMP:
	{
		snprintf(buf, 255, "%.2f", value / 100.0 + 273.15);
		xmlNewChild(fmt_data->after, NULL, BAD_CAST "lowesttemperature", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_PX_START:
	{
		if (fmt_data->tanks.find(index) == fmt_data->tanks.end())
			fmt_data->tanks.insert(std::pair<uint8_t, xmlNode *>(index, xmlNewChild(fmt_data->cur_profile, NULL, BAD_CAST "tankdata", NULL)));

		snprintf(buf, 255, "%u", value);
		xmlNewChild(fmt_data->tanks[index], NULL, BAD_CAST "tankpressurebegin", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_PX_END:
	{
		if (fmt_data->tanks.find(index) == fmt_data->tanks.end())
			fmt_data->tanks.insert(std::pair<uint8_t, xmlNode *>(index, xmlNewChild(fmt_data->cur_profile, NULL, BAD_CAST "tankdata", NULL)));

		snprintf(buf, 255, "%u", value);
		xmlNewChild(fmt_data->tanks[index], NULL, BAD_CAST "tankpressureend", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_PMO2:
	{
		if (! value)
			break;

		if (fmt_data->tanks.find(index) == fmt_data->tanks.end())
			fmt_data->tanks.insert(std::pair<uint8_t, xmlNode *>(index, xmlNewChild(fmt_data->cur_profile, NULL, BAD_CAST "tankdata", NULL)));

		snprintf(buf, 255, "mix-%d", index);
		std::string mix(buf);
		mix_list_t::iterator it = fmt_data->cur_mixes.find(mix);
		if (it == fmt_data->cur_mixes.end())
		{
			fmt_data->cur_mixes.insert(std::pair<std::string, mix_t>(mix, mix_t(value, 0)));
		}
		else
		{
			it->second.first = value;
		}
		break;
	}

	case DIVE_HEADER_PMHe:
	{
		if (! value)
			break;

		if (fmt_data->tanks.find(index) == fmt_data->tanks.end())
			fmt_data->tanks.insert(std::pair<uint8_t, xmlNode *>(index, xmlNewChild(fmt_data->cur_profile, NULL, BAD_CAST "tankdata", NULL)));

		snprintf(buf, 255, "mix-%d", index);
		std::string mix(buf);
		mix_list_t::iterator it = fmt_data->cur_mixes.find(mix);
		if (it == fmt_data->cur_mixes.end())
		{
			fmt_data->cur_mixes.insert(std::pair<std::string, mix_t>(mix, mix_t(0, value)));
		}
		else
		{
			it->second.second = value;
		}
		break;
	}

	case DIVE_HEADER_VENDOR:
	{
		char n[256];
		snprintf(n, 255, "%s%u", name, index);
		snprintf(buf, 255, "%u", value);
		xmlNewChild(fmt_data->appdata, NULL, BAD_CAST n, BAD_CAST buf);
		break;
	}
	}
}

/* Parser Callback for Waypoint Data */
void uddf_waypoint_cb(void * arg, uint8_t token, int32_t value, uint8_t index, const char * name)
{
	output_fmt_data_t cb_data = static_cast<output_fmt_data_t>(arg);
	uddf_fmt_data *	fmt_data;
	char buf[256];

	if (! cb_data || (cb_data->magic != UDDF_FMT_MAGIC))
		return;

	fmt_data = static_cast<uddf_fmt_data *>(cb_data->fmt_data);

	/* Check if Processing Profile */
	if (! cb_data->output_profile)
		return;

	/* Parse Token */
	switch (token)
	{
	case DIVE_WAYPOINT_TIME:
	{
		snprintf(buf, 255, "%d", value);
		fmt_data->cur_waypoint = xmlNewChild(fmt_data->samples, NULL, BAD_CAST "waypoint", NULL);
		xmlNewChild(fmt_data->cur_waypoint, NULL, BAD_CAST "divetime", BAD_CAST buf);
		break;
	}

	case DIVE_WAYPOINT_DEPTH:
	{
		snprintf(buf, 255, "%.2f", value / 100.0);
		xmlNewChild(fmt_data->cur_waypoint, NULL, BAD_CAST "depth", BAD_CAST buf);
		break;
	}

	case DIVE_WAYPOINT_TEMP:
	{
		snprintf(buf, 255, "%.2f", value / 100.0 + 273.15);
		xmlNewChild(fmt_data->cur_waypoint, NULL, BAD_CAST "temperature", BAD_CAST buf);
		break;
	}

	case DIVE_WAYPOINT_ALARM:
	{
		xmlNewChild(fmt_data->cur_waypoint, NULL, BAD_CAST "alarm", BAD_CAST name);
		break;
	}
	}
}
