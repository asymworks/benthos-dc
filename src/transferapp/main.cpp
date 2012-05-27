/*
 * Copyright (C) 2011 Asymworks, LLC.  All Rights Reserved.
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
 * @file src/transferapp/main.cpp
 * @brief Example Data Transfer Application
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 *
 * This program demonstrates how to transfer data from a dive computer, and
 * outputs transferred data in UDDF.
 */

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <list>
#include <map>
#include <vector>
#include <utility>

#include <benthos/divecomputer/config.hpp>
#include <benthos/divecomputer/driver.hpp>
#include <benthos/divecomputer/driverclass.hpp>
#include <benthos/divecomputer/plugin.hpp>
#include <benthos/divecomputer/registry.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <libxml/parser.h>
#include <libxml/tree.h>

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace benthos;

typedef std::vector<uint8_t>		dive_buffer_t;
typedef std::list<dive_buffer_t>	dive_data_t;

void listDrivers()
{
	std::cout << "Registered Device Drivers" << std::endl;
	std::cout << std::endl;

	PluginRegistry::Ptr reg = PluginRegistry::Instance();
	std::list<plugin_manifest_t>::const_iterator it;
	for (it = reg->plugins().begin(); it != reg->plugins().end(); it++)
	{
		std::list<driver_manifest_t>::const_iterator it2;
		for (it2 = it->plugin_drivers.begin(); it2 != it->plugin_drivers.end(); it2++)
			std::cout << "  " << std::setiosflags(std::ios_base::left) << std::setw(20) << it2->driver_name << it2->driver_desc << std::endl;
	}
}

std::string tokenFilePath(const std::string & driver, uint32_t serial, const std::string & path)
{
	/*
	 * Get the file path in which the transfer token is stored.  If the path
	 * argument is specified, the file will be stored there; otherwise, the
	 * data is stored in the home directory under the .benthos/tokens directory.
	 *
	 * The file name is the driver name, a dash and the device serial number.
	 */
	std::stringstream ss;
	ss << driver << "-" << serial;
	std::string filename = ss.str();

	fs::path tokenpath;

	// Get the Directory
	if (path.empty())
	{
#ifdef _WIN32
		char * homedir_ = getenv("APPDATA")
#else
		char * homedir_ = getenv("HOME");
#endif

		tokenpath = homedir_;
		tokenpath /= ".benthos-dc";
		tokenpath /= "tokens";
	}
	else
	{
		tokenpath = path;
	}

	// Append the File Name
	tokenpath /= filename;

	return tokenpath.native();
}

#define PB_WIDTH	60

void draw_progress_bar(double pct)
{
	int c = pct * PB_WIDTH;

	std::string bar;
	for (int i = 0; i < PB_WIDTH; i++)
	{
		if (c > i)
			bar += '=';
		else
			bar += ' ';
	}

	std::cout << "\x1B[2K";		// Erase Current Line
	std::cout << "\x1B[0E";		// Carriage Return
	std::cout << "[" << bar << "] " << (static_cast<int>(100 * pct)) << "%";

	std::flush(std::cout);
}

void progress_bar(void * userdata, uint32_t transferred, uint32_t total)
{
	static int filled = 0;
	double pct = transferred / (double)total;

	if (transferred == 0)
	{
		// Initialize the Progress Bar
		filled = 0;
		draw_progress_bar(0);
		return;
	}

	if (transferred == total)
	{
		// Finalize the Progress Bar
		draw_progress_bar(1);
		std::cout << std::endl << "Transfer Finished" << std::endl;
		return;
	}

	int next_filled = pct * PB_WIDTH;
	if (next_filled <= filled)
		return;

	draw_progress_bar(pct);
	filled = next_filled;
}

typedef std::pair<uint16_t, uint16_t>	mix_t;
typedef std::map<std::string, mix_t>	mix_list_t;

typedef struct
{
	xmlDoc *						doc;			///< XML Document
	xmlNode *						root;			///< XML Root Node
	xmlNode *						profiles;		///< UDDF Profiles Node
	xmlNode *						repgrp;			///< UDDF Repetition Group Node

	mix_list_t						mixes;			///< List of Gas Mixes
	std::map<uint8_t, xmlNode *>	tanks;			///< List of Tank Nodes

	xmlNode *						cur_profile;	///< Current Profile Node
	xmlNode *						cur_waypoint;	///< Current Waypoint Node
	xmlNode *						appdata;		///< Application Data Node
	xmlNode *						samples;		///< Samples Node

	xmlNode *						before;			///< Information Before Dive Node
	xmlNode *						after;			///< Information After Dive Node

	std::string						ts;				///< Date/Time Stamp
	int								rgid;			///< Repetition Group Id
	int								did;			///< Dive Id

} parser_data;

void parse_header(void * userdata, uint8_t token, int32_t value, uint8_t index, const char * name)
{
	time_t st;
	char buf[255];

	parser_data * _data = (parser_data *)userdata;

	if (_data->cur_profile == 0)
	{
		sprintf(buf, "benthos_dive_%s_%03d", _data->ts.c_str(), _data->did++);
		_data->cur_profile = xmlNewNode(NULL, BAD_CAST "dive");
		xmlNewProp(_data->cur_profile, BAD_CAST "id", BAD_CAST buf);

		xmlNode * appdata = xmlNewChild(_data->cur_profile, NULL, BAD_CAST "applicationdata", NULL);
		_data->appdata = xmlNewChild(appdata, NULL, BAD_CAST "benthos", NULL);

		_data->before = xmlNewChild(_data->cur_profile, NULL, BAD_CAST "informationbeforedive", NULL);
		_data->after = xmlNewChild(_data->cur_profile, NULL, BAD_CAST "informationafterdive", NULL);

		_data->samples = xmlNewChild(_data->cur_profile, NULL, BAD_CAST "samples", NULL);
	}

	switch (token)
	{
	case DIVE_HEADER_START_TIME:
	{
		st = (time_t)value;
		struct tm * tmp;
		tmp = gmtime(& st);
		strftime(buf, 255, "%Y-%m-%dT%H:%M:%S", tmp);
		xmlNewChild(_data->cur_profile, NULL, BAD_CAST "datetime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_UTC_OFFSET:
	{
		sprintf(buf, "%d", value);
		xmlNewChild(_data->appdata, NULL, BAD_CAST "utc_offset", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_DURATION:
	{
		sprintf(buf, "%u", value * 60);
		xmlNewChild(_data->after, NULL, BAD_CAST "diveduration", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_INTERVAL:
	{
		xmlNode * si = xmlNewChild(_data->before, NULL, BAD_CAST "surfaceintervalbeforedive", NULL);
		if (value == 0)
		{
			xmlNewChild(si, NULL, BAD_CAST "infinity", NULL);
		}
		else
		{
			sprintf(buf, "%d", value * 60);
			xmlNewChild(si, NULL, BAD_CAST "passedtime", BAD_CAST buf);
		}
		break;
	}

	case DIVE_HEADER_REPETITION:
	{
		if (value == 1)
		{
			sprintf(buf, "benthos_repgroup_%s_%03d", _data->ts.c_str(), _data->rgid++);
			_data->repgrp = xmlNewChild(_data->profiles, NULL, BAD_CAST "repetitiongroup", NULL);
			xmlNewProp(_data->repgrp, BAD_CAST "id", BAD_CAST buf);
		}
		break;
	}

	case DIVE_HEADER_DESAT_BEFORE:
	{
		sprintf(buf, "%u", value * 60);
		xmlNewChild(_data->before, NULL, BAD_CAST "desaturationtime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_DESAT_AFTER:
	{
		sprintf(buf, "%u", value * 60);
		xmlNewChild(_data->after, NULL, BAD_CAST "desaturationtime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_NOFLY_BEFORE:
	{
		sprintf(buf, "%u", value * 60);
		xmlNewChild(_data->before, NULL, BAD_CAST "noflighttime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_NOFLY_AFTER:
	{
		sprintf(buf, "%u", value * 60);
		xmlNewChild(_data->after, NULL, BAD_CAST "noflighttime", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_MAX_DEPTH:
	{
		sprintf(buf, "%.2f", value / 100.0);
		xmlNewChild(_data->after, NULL, BAD_CAST "greatestdepth", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_AVG_DEPTH:
	{
		sprintf(buf, "%.2f", value / 100.0);
		xmlNewChild(_data->appdata, NULL, BAD_CAST "avg_depth", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_AIR_TEMP:
	{
		sprintf(buf, "%.2f", value / 100.0 + 273.15);
		xmlNewChild(_data->before, NULL, BAD_CAST "airtemperature", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_MAX_TEMP:
	{
		sprintf(buf, "%.2f", value / 100.0);
		xmlNewChild(_data->appdata, NULL, BAD_CAST "max_temp", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_MIN_TEMP:
	{
		sprintf(buf, "%.2f", value / 100.0 + 273.15);
		xmlNewChild(_data->after, NULL, BAD_CAST "lowesttemperature", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_PX_START:
	{
		if (_data->tanks.find(index) == _data->tanks.end())
			_data->tanks.insert(std::pair<uint8_t, xmlNode *>(index, xmlNewChild(_data->cur_profile, NULL, BAD_CAST "tankdata", NULL)));

		sprintf(buf, "%u", value);
		xmlNewChild(_data->tanks[index], NULL, BAD_CAST "tankpressurebegin", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_PX_END:
	{
		if (_data->tanks.find(index) == _data->tanks.end())
			_data->tanks.insert(std::pair<uint8_t, xmlNode *>(index, xmlNewChild(_data->cur_profile, NULL, BAD_CAST "tankdata", NULL)));

		sprintf(buf, "%u", value);
		xmlNewChild(_data->tanks[index], NULL, BAD_CAST "tankpressureend", BAD_CAST buf);
		break;
	}

	case DIVE_HEADER_PMO2:
	{
		if (! value)
			break;

		if (_data->tanks.find(index) == _data->tanks.end())
			_data->tanks.insert(std::pair<uint8_t, xmlNode *>(index, xmlNewChild(_data->cur_profile, NULL, BAD_CAST "tankdata", NULL)));

		sprintf(buf, "mix-%d", index);
		std::string mix(buf);
		mix_list_t::iterator it = _data->mixes.find(mix);
		if (it == _data->mixes.end())
		{
			_data->mixes.insert(std::pair<std::string, mix_t>(mix, mix_t(value, 0)));
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

		if (_data->tanks.find(index) == _data->tanks.end())
			_data->tanks.insert(std::pair<uint8_t, xmlNode *>(index, xmlNewChild(_data->cur_profile, NULL, BAD_CAST "tankdata", NULL)));

		sprintf(buf, "mix-%d", index);
		std::string mix(buf);
		mix_list_t::iterator it = _data->mixes.find(mix);
		if (it == _data->mixes.end())
		{
			_data->mixes.insert(std::pair<std::string, mix_t>(mix, mix_t(0, value)));
		}
		else
		{
			it->second.second = value;
		}
		break;
	}

	case DIVE_HEADER_VENDOR:
	{
		char n[255];
		sprintf(n, "%s%u", name, index);
		sprintf(buf, "%u", value);
		xmlNewChild(_data->appdata, NULL, BAD_CAST n, BAD_CAST buf);
		break;
	}
	}
}

void parse_profile(void * userdata, uint8_t token, int32_t value, uint8_t index, const char * name)
{
	char buf[255];
	parser_data * _data = (parser_data *)userdata;

	switch (token)
	{
	case DIVE_WAYPOINT_TIME:
	{
		sprintf(buf, "%d", value);
		_data->cur_waypoint = xmlNewChild(_data->samples, NULL, BAD_CAST "waypoint", NULL);
		xmlNewChild(_data->cur_waypoint, NULL, BAD_CAST "divetime", BAD_CAST buf);
		break;
	}

	case DIVE_WAYPOINT_DEPTH:
	{
		sprintf(buf, "%.2f", value / 100.0);
		xmlNewChild(_data->cur_waypoint, NULL, BAD_CAST "depth", BAD_CAST buf);
		break;
	}

	case DIVE_WAYPOINT_TEMP:
	{
		sprintf(buf, "%.2f", value / 100.0 + 273.15);
		xmlNewChild(_data->cur_waypoint, NULL, BAD_CAST "temperature", BAD_CAST buf);
		break;
	}

	case DIVE_WAYPOINT_ALARM:
	{
		xmlNewChild(_data->cur_waypoint, NULL, BAD_CAST "alarm", BAD_CAST name);
		break;
	}
	}
}

void parseDives(Driver::Ptr driver, const dive_data_t & dives, std::string outfile)
{
	parser_data _data;

	// Initialize the XML File
	_data.doc = xmlNewDoc(BAD_CAST "1.0");
	_data.root = xmlNewNode(NULL, BAD_CAST "uddf");
	xmlDocSetRootElement(_data.doc, _data.root);

	// Set the UDDF Data Version
	xmlNewProp(_data.root, BAD_CAST "version", BAD_CAST "3.0.0");

	// Set Generator Data
	xmlNode * generator = xmlNewChild(_data.root, NULL, BAD_CAST "generator", NULL);
	xmlNewChild(generator, NULL, BAD_CAST "name", BAD_CAST "benthos-dc");
	xmlNewChild(generator, NULL, BAD_CAST "version", BAD_CAST BENTHOS_DC_VERSION_STRING);

	char buf[255];
	time_t t = time(NULL);
	struct tm * tm;
	tm = localtime(& t);
	strftime(buf, 250, "%Y-%m-%d", tm);
	xmlNewChild(generator, NULL, BAD_CAST "datetime", BAD_CAST buf);

	strftime(buf, 250, "%Y%m%dT%H%m%S", tm);
	_data.ts = std::string(buf);

	// Setup Gas Definitions Node
	xmlNode * gasdef = xmlNewChild(_data.root, NULL, BAD_CAST "gasdefinitions", NULL);
	xmlNode * profiles = xmlNewChild(_data.root, NULL, BAD_CAST "profiledata", NULL);

	// Initialize the Parser Data
	mix_list_t mixes;
	mixes.insert(std::pair<std::string, mix_t>("air", mix_t(210, 0)));
	mixes.insert(std::pair<std::string, mix_t>("ean32", mix_t(320, 0)));
	mixes.insert(std::pair<std::string, mix_t>("ean36", mix_t(360, 0)));

	_data.profiles = profiles;
	_data.repgrp = 0;
	_data.rgid = 0;
	_data.did = 0;

	// Parse the Dives
	dive_data_t::const_iterator it;
	for (it = dives.begin(); it != dives.end(); it++)
	{
		_data.cur_profile = 0;
		_data.cur_waypoint = 0;

		_data.mixes.clear();
		_data.tanks.clear();

		driver->parse(* it, & parse_header, & parse_profile, & _data);
		xmlAddChild(_data.repgrp, _data.cur_profile);

		// Process Tank Mixes
		std::map<uint8_t, xmlNode *>::iterator idx;
		for (idx = _data.tanks.begin(); idx != _data.tanks.end(); idx++)
		{
			char buf[255];
			sprintf(buf, "mix-%u", idx->first);
			mix_list_t::iterator dive_mix = _data.mixes.find(buf);
			if (dive_mix == _data.mixes.end())
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
				for (curmix = mixes.begin(); curmix != mixes.end(); curmix++)
					if ((curmix->second.first == o2) && (curmix->second.second == he))
						break;

				if (curmix == mixes.end())
				{
					// Add a new mix
					sprintf(buf, "mix-%lu", mixes.size());
					mixes.insert(std::pair<std::string, mix_t>(buf, mix_t(o2, he)));
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
	}

	// Save the Gas Definitions
	mix_list_t::const_iterator it2;
	for (it2 = mixes.begin(); it2 != mixes.end(); it2++)
	{
		xmlNode * mixnode = xmlNewChild(gasdef, NULL, BAD_CAST "mix", NULL);
		xmlNewProp(mixnode, BAD_CAST "id", BAD_CAST it2->first.c_str());
		uint16_t o2 = it2->second.first;
		uint16_t he = it2->second.second;
		uint16_t n2 = 1000 - (o2 + he);

		char _o2[10];	sprintf(_o2, "0.%03u", o2);
		char _he[10];	sprintf(_he, "0.%03u", he);
		char _n2[10];	sprintf(_n2, "0.%03u", n2);

		xmlNewChild(mixnode, NULL, BAD_CAST "o2", BAD_CAST _o2);
		xmlNewChild(mixnode, NULL, BAD_CAST "n2", BAD_CAST _n2);
		xmlNewChild(mixnode, NULL, BAD_CAST "he", BAD_CAST _he);
		xmlNewChild(mixnode, NULL, BAD_CAST "ar", BAD_CAST ("0.000"));
		xmlNewChild(mixnode, NULL, BAD_CAST "h2", BAD_CAST ("0.000"));
	}

	// Save the XML File
	xmlSaveFormatFileEnc(outfile.empty() ? "-" : outfile.c_str(), _data.doc, "UTF-8", 1);

	// Release Resources
	xmlFreeDoc(_data.doc);
	xmlCleanupParser();
}

int main(int argc, char ** argv)
{
	// Setup Program Options
	po::options_description generic("Generic Options");
	generic.add_options()
		("help",		"Display this help message")
		("list",		"Display all installed drivers and exit")
		("quiet,q", 	"Suppress status messages")
	;

	po::options_description transfer("Transfer Options");
	transfer.add_options()
		("driver,d", po::value<std::string>(), "Driver name")
		("dargs", po::value<std::string>(), "Driver arguments")
		("device", po::value<std::string>(), "Device name")
		("output-file,o", po::value<std::string>(), "Output file")
		("token,t", po::value<std::string>(), "Transfer token")
		("token-path", po::value<std::string>(), "Transfer token storage path")
		("no-store-token,U", "Don't update the stored Transfer Token")
	;

	po::options_description desc;
	desc.add(generic).add(transfer);

	po::positional_options_description p;
	p.add("device", 1);

	// Parse Command Line
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
	po::notify(vm);

	// Show the Help Message
	if (vm.count("help"))
	{
		std::cout << "Usage: " << argv[0] << " [options] <device>" << std::endl;
		std::cout << desc << std::endl;
		return 0;
	}

	// Show the Driver List
	if (vm.count("list"))
	{
		listDrivers();
		return 0;
	}

	// Driver must be specified for Transfer Operations
	if (! vm.count("driver"))
	{
		std::cerr << "No device driver specified" << std::endl;
		return 1;
	}

	bool quiet = (vm.count("quiet") > 0);
	std::string driver_name(vm["driver"].as<std::string>());

	// Try and load the Driver Manifest
	PluginRegistry::Ptr reg = PluginRegistry::Instance();
	const driver_manifest_t * dm = reg->findDriver(driver_name);
	if (dm == NULL)
	{
		std::cerr << "Driver " << driver_name << " is not registered" << std::endl;
	}

	if (! quiet)
		std::cout << "Loaded driver " << driver_name << " from plugin " << dm->plugin_name << std::endl;

	if ((dm->driver_intf != "irda") && ! vm.count("device"))
	{
		// IrDA Drivers are allowed to skip the driver argument
		std::cerr << "Driver " << driver_name << " requires a device to be specified" << std::endl;
		return 1;
	}

	// Load the plugin
	Plugin::Ptr plugin;
	try
	{
		plugin = reg->loadPlugin(dm->plugin_name);
	}
	catch (std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	if (! quiet)
		std::cout << "Loaded plugin " << dm->plugin_name << " from library " << plugin->path() << std::endl;

	// Connect to the Device
	std::string device_path;
	if (vm.count("device"))
		device_path = vm["device"].as<std::string>();

	std::string driver_args;
	if (vm.count("dargs"))
		driver_args = vm["dargs"].as<std::string>();

	DriverClass::Ptr dclass;
	Driver::Ptr dev;
	try
	{
		dclass = plugin->driver(driver_name);
		dev = dclass->open(device_path, driver_args);
	}
	catch (std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	if (! quiet)
	{
		std::string mfg = dev->manufacturer();
		std::cout << "Opened ";
		if (! mfg.empty())
			std::cout << mfg << " ";
		std::cout << dev->model_name() << " (sn: " << dev->serial_number() << ")";
		if (vm.count("device"))
			std::cout << " on " << device_path;
		std::cout << std::endl;
	}

	// Load the Transfer Token
	std::string token;
	std::string token_path;
	if (vm.count("token-path"))
		token_path = vm["token-path"].as<std::string>();

	std::string tokenfile = tokenFilePath(driver_name, dev->serial_number(), token_path);
	if (vm.count("token"))
	{
		token = vm["token"].as<std::string>();
		if (! quiet)
			std::cout << "Using token " << token << " from command line" << std::endl;
	}
	else
	{
		std::ifstream f(tokenfile);
		f >> token;
		f.close();

		if (! token.empty() && ! quiet)
			std::cout << "Loaded token " << token << " from " << tokenfile << std::endl;
	}

	// Set the Transfer Token
	if (! token.empty())
		dev->set_token(token);

	// Allocate storage for transferred data
	dive_data_t dive_data;

	// Get Transfer Length
	uint32_t len = dev->transfer_length();
	if (len > 0)
	{
		if (! quiet)
			std::cout << "Going to transfer " << len << " bytes" << std::endl;

		// Run the Transfer
		transfer_callback_fn_t cb = NULL;
		if (! quiet)
			cb = & progress_bar;
		dive_data = dev->transfer(cb, 0);

		if (! quiet)
			std::cout << "Transferred " << dive_data.size() << " new dives" << std::endl;
	}
	else
		std::cout << "No new data to transfer" << std::endl;

	// Store the new Transfer Token
	fs::path tokenpath(tokenfile);
	fs::path tokendir(tokenpath.parent_path());

	if (! vm.count("no-store-token"))
	{
		try
		{
			if (! fs::exists(tokendir))
				fs::create_directories(tokendir);

			token = dev->issue_token();
			std::ofstream f(tokenpath.native());
			f << token << std::endl;
			f.close();

			if (! quiet)
				std::cout << "Stored token " << token << " to " << tokenfile << std::endl;
		}
		catch (std::exception & e)
		{
			std::cerr << "Failed to save transfer token to " << tokenpath.native() << std::cerr;
			std::cerr << e.what() << std::endl;
		}
	}

	// Parse Dives
	if (len > 0)
	{
		std::string outfile;
		if (vm.count("output-file"))
			outfile = vm["output-file"].as<std::string>();

		parseDives(dev, dive_data, outfile);
		if (! quiet && ! outfile.empty())
			std::cout << "Wrote UDDF dive data to " << outfile << std::endl;
	}

	// Success!
	return 0;
}
