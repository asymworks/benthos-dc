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
#include <string>
#include <vector>

#include <benthos/divecomputer/config.h>
#include <benthos/divecomputer/manifest.h>
#include <benthos/divecomputer/registry.h>

#include <benthos/divecomputer/plugin/driver.h>
#include <benthos/divecomputer/plugin/parser.h>
#include <benthos/divecomputer/plugin/plugin.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include "output_fmt.h"
#include "output_csv.h"
#include "output_uddf.h"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

typedef std::vector<uint8_t>					dive_buffer_t;
typedef std::pair<dive_buffer_t, std::string>	dive_entry_t;
typedef std::list<dive_entry_t>					dive_data_t;

int list_drivers(void)
{
	int rv;
	int err = 0;
	driver_iterator_t it;
	const driver_info_t * di;
	const driver_interface_t * drv;

	std::cout << "Registered Device Drivers" << std::endl;
	std::cout << std::endl;
	std::cout << boost::format("%-20s %-12s %-40s\n") % "Plugin" % "Driver" % "Description";
	std::cout << "--------------------------------------------------------------------------\n";

	it = benthos_dc_registry_drivers();
	while ((di = benthos_dc_driver_iterator_info(it)) != 0)
	{
		/* Check that the Driver Loads */
		rv = benthos_dc_registry_load(di->driver_name, & drv);
		if (rv != 0)
		{
			err = 1;
			std::cout << boost::format("! %-18s %-12s %-40s\n") % di->plugin->plugin_name % di->driver_name % di->driver_desc;
		}
		else
		{
			std::cout << boost::format("  %-18s %-12s %-40s\n") % di->plugin->plugin_name % di->driver_name % di->driver_desc;
		}

		benthos_dc_driver_iterator_next(it);
	}

	if (err)
	{
		std::cout << std::endl;
		std::cout << "Some drivers (marked with !) failed to load." << std::endl;
		std::cout << "Run benthos-xfr --test to see detailed information" << std::endl;
	}

	return err;
}

int test_drivers(void)
{
	int rv;
	int count = 0;
	int err = 0;
	driver_iterator_t it;
	const driver_info_t * di;
	const driver_interface_t * drv;

	it = benthos_dc_registry_drivers();
	while ((di = benthos_dc_driver_iterator_info(it)) != 0)
	{
		/* Increment Driver Count */
		++count;

		/* Check that the Driver Loads */
		rv = benthos_dc_registry_load(di->driver_name, & drv);
		if (rv != 0)
		{
			err = 1;
			std::cerr << "Failed to load driver '" << di->driver_name << "' from plugin '" << di->plugin->plugin_name << std::endl;
			std::cerr << "  Plugin Library: " << di->plugin->plugin_library << std::endl;
			std::cerr << "  benthos_dc_registry_load() returned " << rv << ": ";

			if (rv < 0)
				std::cerr << benthos_dc_registry_strerror(rv);
			else
				std::cerr << strerror(rv);

			std::cerr << std::endl;

		}

		benthos_dc_driver_iterator_next(it);
	}

	if (err)
		return err;

	if (! count)
	{
		std::cerr << "No plugins are registered with Benthos" << std::endl;
		return 2;
	}

	std::cout << "Tested " << count << " drivers: passed" << std::endl;

	return 0;
}

std::string token_path(const std::string & driver, uint32_t serial, const std::string & path)
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

int register_paths(const po::variables_map & vm)
{
	int rv;

	if (vm.count("manifest-file"))
	{
		std::vector<std::string> files(vm["manifest-file"].as<std::vector<std::string> >());
		std::vector<std::string>::const_iterator it;

		for (it = files.begin(); it != files.end(); it++)
		{
			rv = benthos_dc_registry_add_manifest(it->c_str());
			if (rv != 0)
			{
				std::cerr << "Failed to add manifest '" << * it << "': " << benthos_dc_registry_strerror(rv) << std::endl;
				return rv;
			}
		}
	}

	if (vm.count("manifest-path"))
	{
		std::vector<std::string> files(vm["manifest-path"].as<std::vector<std::string> >());
		std::vector<std::string>::const_iterator it;

		for (it = files.begin(); it != files.end(); it++)
		{
			rv = benthos_dc_registry_add_manifest_path(it->c_str());
			if (rv != 0)
			{
				std::cerr << "Failed to add manifest path '" << * it << "': " << benthos_dc_registry_strerror(rv) << std::endl;
				return rv;
			}
		}
	}

	if (vm.count("plugin-file"))
	{
		std::vector<std::string> files(vm["plugin-file"].as<std::vector<std::string> >());
		std::vector<std::string>::const_iterator it;

		for (it = files.begin(); it != files.end(); it++)
		{
			rv = benthos_dc_registry_add_plugin(it->c_str());
			if (rv != 0)
			{
				std::cerr << "Failed to add plugin '" << * it << "': " << benthos_dc_registry_strerror(rv) << std::endl;
				return rv;
			}
		}
	}

	if (vm.count("plugin-path"))
	{
		std::vector<std::string> files(vm["plugin-path"].as<std::vector<std::string> >());
		std::vector<std::string>::const_iterator it;

		for (it = files.begin(); it != files.end(); it++)
		{
			rv = benthos_dc_registry_add_plugin_path(it->c_str());
			if (rv != 0)
			{
				std::cerr << "Failed to add plugin path '" << * it << "': " << benthos_dc_registry_strerror(rv) << std::endl;
				return rv;
			}
		}
	}

	return 0;
}

typedef struct {
	const driver_info_t *		di;

	const driver_interface_t *	drv;
	dev_handle_t				dev;

	bool						quiet;

	std::string					device_path;
	std::string					token_file;
	std::string					token_path;
	std::string					token;

	uint8_t						model;
	uint32_t					serial;
	uint32_t					ticks;

} devcb_data;

const char * model_mfg(const driver_info_t * di, uint8_t model)
{
	int i;

	for (i = 0; i < di->n_models; ++i)
		if (di->models[i]->model_number == model)
			return di->models[i]->manuf_name;

	return "";
}

const char * model_name(const driver_info_t * di, uint8_t model)
{
	int i;

	for (i = 0; i < di->n_models; ++i)
		if (di->models[i]->model_number == model)
			return di->models[i]->model_name;

	return "";
}

int device_cb(void * userdata, uint8_t model, uint32_t serial, uint32_t ticks, char ** token_, int * free_token)
{
	devcb_data * a = (devcb_data *)(userdata);
	if (a == NULL)
		return -1;

	/* Store Device Information */
	a->model = model;
	a->serial = serial;
	a->ticks = ticks;

	if (! a->quiet)
	{
		std::string mfg(model_mfg(a->di, model));
		std::string mname(model_name(a->di, model));
		std::cout << "Opened ";
		if (! mfg.empty())
			std::cout << mfg << " ";
		std::cout << mname << " (sn: " << serial << ")";
		if (! a->device_path.empty())
			std::cout << " at " << a->device_path;
		std::cout << std::endl;
	}

	/* Load the Transfer Token */
	std::string token;
	a->token_file = token_path(a->di->driver_name, serial, a->token_path);
	if (! a->token.empty())
	{
		if (a->token != "-")
		{
			token = a->token;
			if (! a->quiet)
				std::cout << "Using token " << token << " from command line" << std::endl;
		}
		else
		{
			if (! a->quiet)
				std::cout << "Skipping token (downloading all dives)" << std::endl;
		}
	}
	else
	{
		std::ifstream f(a->token_file);
		f >> token;
		f.close();

		if (! token.empty() && ! a->quiet)
			std::cout << "Loaded token " << token << " from " << a->token_file << std::endl;
	}

	/* Set the Transfer Token */
	if (! token.empty())
	{
		(* token_) = strdup(token.c_str());
		(* free_token) = 1;
	}
	else
	{
		(* token_) = NULL;
		(* free_token) = 0;
	}

	return 0;
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

void transfer_cb(void * userdata, uint32_t transferred, uint32_t total, int *)
{
	devcb_data * a = (devcb_data *)(userdata);
	if (a == NULL)
		return;

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

void extract_cb(void * userdata, void * buffer_ptr, uint32_t buffer_len, const char * token)
{
	dive_data_t * data = (dive_data_t *)(userdata);
	if (! data)
		return;

	dive_buffer_t buffer((uint8_t *)buffer_ptr, (uint8_t *)buffer_ptr + buffer_len);
	dive_entry_t entry(buffer, std::string(token));

	data->push_back(entry);
}

int run_parser(const po::variables_map & vm, const driver_interface_t * drv, dev_handle_t dev,
		const driver_info_t * di, devcb_data * dev_data, const char * drv_args,
		const char * dev_path, const dive_data_t & dive_data)
{
	int rv;
	struct output_fmt_data_t_ * fmt_data;
	std::string outfile;
	std::string format("uddf");
	parser_handle_t parser;
	dive_data_t::const_iterator it;

	/* Allocate the Formatting Data */
	fmt_data = (struct output_fmt_data_t_ *)malloc(sizeof(struct output_fmt_data_t_));
	if (! fmt_data)
		return ENOMEM;

	/* Initialize the Formatter Data */
	fmt_data->magic = 0;
	fmt_data->fmt_data = 0;

	fmt_data->driver_name = di->driver_name;
	fmt_data->driver_args = drv_args;
	fmt_data->device_path = dev_path;
	fmt_data->driver_info = di;

	fmt_data->dev_model = dev_data->model;
	fmt_data->dev_serial = dev_data->serial;

	if (vm.count("output-file"))
		outfile = vm["output-file"].as<std::string>();

	fmt_data->output_file = outfile.c_str();
	fmt_data->output_header = 1;
	fmt_data->output_profile = 1 - vm.count("header-only");

	fmt_data->quiet = vm.count("quiet");

	fmt_data->header_cb = 0;
	fmt_data->profile_cb = 0;
	fmt_data->dispose_fn = 0;
	fmt_data->close_fn = 0;
	fmt_data->prolog_fn = 0;
	fmt_data->epilog_fn = 0;

	/* Initialize the Output Formatter */
	if (vm.count("output-format"))
		format = vm["output-format"].as<std::string>();

	if (format == "uddf")
	{
		rv = uddf_init_formatter(fmt_data);
		if (rv != 0)
		{
			std::cerr << "Failed to initialize UDDF Formatter: " << strerror(rv) << std::endl;
			free(fmt_data);
			return rv;
		}
	}
	else if (format == "csv")
	{
		rv = csv_init_formatter(fmt_data);
		if (rv != 0)
		{
			std::cerr << "Failed to initialize CSV Formatter: " << strerror(rv) << std::endl;
			free(fmt_data);
			return rv;
		}
	}
	else
	{
		std::cerr << "Unknown output format '" + format + "'" << std::endl;
		free(fmt_data);
		return EINVAL;
	}

	/* Create the Parser */
	rv = drv->parser_create(& parser, dev);
	if (rv != 0)
	{
		std::cerr << "Failed to create parser: '" + std::string(drv->driver_errmsg(dev)) << "'" << std::endl;
		fmt_data->dispose_fn(fmt_data);
		free(fmt_data);
		return rv;
	}

	/* Parse Dives */
	for (it = dive_data.begin(); it != dive_data.end(); it++)
	{
		if (fmt_data->prolog_fn)
		{
			rv = fmt_data->prolog_fn(fmt_data);
			if (rv != 0)
			{
				std::cerr << "Failed to run output formatter prolog: " << strerror(rv) << std::endl;
				fmt_data->dispose_fn(fmt_data);
				free(fmt_data);
				return rv;
			}
		}

		rv = drv->parser_reset(parser);
		if (rv != 0)
		{
			std::cerr << "Failed to reset parser: '" + std::string(drv->driver_errmsg(dev)) << "'" << std::endl;
			fmt_data->dispose_fn(fmt_data);
			free(fmt_data);
			return rv;
		}

		rv = drv->parser_parse_header(parser, it->first.data(), it->first.size(), fmt_data->header_cb, fmt_data);
		if (rv != 0)
		{
			std::cerr << "Failed to parse header: '" + std::string(drv->driver_errmsg(dev)) << "'" << std::endl;
			fmt_data->dispose_fn(fmt_data);
			free(fmt_data);
			return rv;
		}

		rv = drv->parser_parse_profile(parser, it->first.data(), it->first.size(), fmt_data->profile_cb, fmt_data);
		if (rv != 0)
		{
			std::cerr << "Failed to parse profile: '" + std::string(drv->driver_errmsg(dev)) << "'" << std::endl;
			fmt_data->dispose_fn(fmt_data);
			free(fmt_data);
			return rv;
		}

		if (fmt_data->epilog_fn)
		{
			rv = fmt_data->epilog_fn(fmt_data);
			if (rv != 0)
			{
				std::cerr << "Failed to run output formatter epilog: " << strerror(rv) << std::endl;
				fmt_data->dispose_fn(fmt_data);
				free(fmt_data);
				return rv;
			}
		}
	}

	/* Close Parser */
	drv->parser_close(parser);

	/* Close and Dispose of Formatter Data */
	fmt_data->close_fn(fmt_data);
	fmt_data->dispose_fn(fmt_data);
	free(fmt_data);

	return 0;
}

int run_transfer(const po::variables_map & vm)
{
	int rv;
	int quiet;
	const driver_interface_t * drv;
	const driver_info_t * di;
	dev_handle_t dev;
	std::string drv_name;
	std::string drv_path;
	std::string drv_args;
	devcb_data cb_data;

	void * buffer_ptr;
	uint32_t buffer_len;
	dive_data_t dive_data;

	fs::path tokenpath;
	fs::path tokendir;
	std::string token;

	// Set Quiet Mode
	quiet = vm.count("quiet");

	// Driver must be specified for Transfer Operations
	if (! vm.count("driver"))
	{
		std::cerr << "No device driver specified" << std::endl;
		return 1;
	}

	drv_name = vm["driver"].as<std::string>();

	// Load the Driver Information
	rv = benthos_dc_registry_driver_info(drv_name.c_str(), & di);
	if (rv != 0)
	{
		std::cerr << "Failed to load driver '" << drv_name << "': " << benthos_dc_registry_strerror(rv) << std::endl;
		return 1;
	}

	// Check the Device Path
	if ((di->driver_intf != diIrDA) && (! vm.count("device")))
	{
		std::cerr << "No device specified" << std::endl;
		return 1;
	}

	// Load the Driver Interface
	rv = benthos_dc_registry_load(drv_name.c_str(), & drv);
	if (rv != 0)
	{
		std::cerr << "Failed to load driver '" << drv_name << "': " << benthos_dc_registry_strerror(rv) << std::endl;
		return 1;
	}

	// Driver Loaded
	if (! quiet)
		std::cout << "Loaded driver '" << di->driver_name << "' from plugin '" << di->plugin->plugin_name << "'" << std::endl;

	// Open a Device Handle
	rv = drv->driver_create(& dev);
	if (rv != DRIVER_ERR_SUCCESS)
	{
		std::cerr << "Failed to open '" << di->driver_name << "' device: " << strerror(errno) << std::endl;
		return 1;
	}

	// Open the Device
	if (vm.count("device"))
		drv_path = vm["device"].as<std::string>();
	if (vm.count("dargs"))
		drv_args = vm["dargs"].as<std::string>();

	rv = drv->driver_open(dev, drv_path.c_str(), drv_args.c_str());
	if (rv != DRIVER_ERR_SUCCESS)
	{
		std::cerr << "Failed to open device at '" << drv_path << "': " << drv->driver_errmsg(dev) << std::endl;
		drv->driver_shutdown(dev);
		return 1;
	}

	// Load Device Callback Data
	cb_data.di = di;
	cb_data.drv = drv;
	cb_data.dev = dev;

	cb_data.device_path = drv_path;

	cb_data.token = "";
	cb_data.token_file = "";
	cb_data.token_path = "";

	if (vm.count("token-path"))
		cb_data.token_path = vm["token-path"].as<std::string>();

	if (vm.count("token"))
		cb_data.token = vm["token"].as<std::string>();

	// Run Transfer
	rv = drv->driver_transfer(dev, & buffer_ptr, & buffer_len, device_cb, transfer_cb, & cb_data);
	if (rv != DRIVER_ERR_SUCCESS)
	{
		std::cerr << "Failed to transfer data from device at '" << drv_path << "': " << drv->driver_errmsg(dev) << std::endl;
		drv->driver_close(dev);
		drv->driver_shutdown(dev);
		return 1;
	}

	// Extract Dives
	if (buffer_len > 0)
	{
		rv = drv->driver_extract(dev, buffer_ptr, buffer_len, extract_cb, & dive_data);
		if (rv != DRIVER_ERR_SUCCESS)
		{
			std::cerr << "Failed to extract dive data from transfer: " << drv->driver_errmsg(dev) << std::endl;
			drv->driver_close(dev);
			drv->driver_shutdown(dev);
			return 1;
		}

		// Free Data Buffer
		free(buffer_ptr);
	}

	// Dives Transferred
	if (dive_data.size() > 0)
	{
		if (! quiet)
			std::cout << "Transferred " << dive_data.size() << " new dives" << std::endl;

		// Parse Dives
		rv = run_parser(vm, drv, dev, di, & cb_data, drv_args.c_str(), drv_path.c_str(), dive_data);
		if (rv != 0)
		{
			drv->driver_close(dev);
			drv->driver_shutdown(dev);
			return 1;
		}
	}
	else
		std::cout << "No new data to transfer" << std::endl;

	// Write Transfer Token
	tokenpath = cb_data.token_file;
	tokendir = tokenpath.parent_path();

	if (! vm.count("no-store-token"))
	{
		try
		{
			if (! fs::exists(tokendir))
				fs::create_directories(tokendir);

			if (dive_data.size() > 0)
			{
				token = dive_data.rbegin()->second;

				std::ofstream f(tokenpath.native());
				f << token << std::endl;
				f.close();

				if (! quiet)
					std::cout << "Stored token " << token << " to " << cb_data.token_file << std::endl;
			}
		}
		catch (std::exception & e)
		{
			std::cerr << "Failed to save transfer token to " << tokenpath.native() << std::cerr;
			std::cerr << e.what() << std::endl;
		}
	}

	// Close Device
	drv->driver_close(dev);
	drv->driver_shutdown(dev);

	// Transfer Done
	return 0;
}

int main(int argc, char ** argv)
{
	int rv;

	// Setup Program Options
	po::options_description generic("Generic Options");
	generic.add_options()
		("help,?",		"Display this help message")
		("list",		"Display all installed drivers and exit")
		("test,T",      "Test installed plugins and exit")
		("quiet,q", 	"Suppress status messages")
		("version,v",	"Display version information and exit")
	;

	po::options_description transfer("Transfer Options");
	transfer.add_options()
		("driver,d", po::value<std::string>(), "Driver name")
		("dargs", po::value<std::string>(), "Driver arguments")
		("device", po::value<std::string>(), "Device name")
		("token,t", po::value<std::string>(), "Transfer token")
		("token-path", po::value<std::string>(), "Transfer token storage path")
		("no-store-token,U", "Don't update the stored Transfer Token")
	;

	po::options_description output("Output Options");
	output.add_options()
		("header-only,h", "Save header only, not profile data")
		("output-file,o", po::value<std::string>(), "Output file")
		("output-format,f", po::value<std::string>(), "Output format")
	;

	po::options_description registry("Registry Options");
	registry.add_options()
		("manifest-file", po::value<std::vector<std::string> >(), "Extra Manifest File")
		("manifest-path,m", po::value<std::vector<std::string> >(), "Extra Manifest Path")
		("plugin-file", po::value<std::vector<std::string> >(), "Extra Plugin File")
		("plugin-path,p", po::value<std::vector<std::string> >(), "Extra Plugin Path")
	;

	po::options_description desc;
	desc.add(generic).add(transfer).add(output).add(registry);

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

	// Show the Version Information
	if (vm.count("version"))
	{
		std::cout << "Benthos Dive Computer Library version " << BENTHOS_DC_VERSION_STRING << std::endl;
		std::cout << "Copyright (c) 2011-2013 Asymworks, LLC.  All Rights Reserved." << std::endl;
		return 0;
	}

	// Initialize the Driver Registry
	rv = benthos_dc_registry_init();
	if (rv != 0)
	{
		std::cerr << "Failed to initialize plugin registry: " << benthos_dc_registry_strerror(rv) << std::endl;
		return 1;
	}

	// Add Manifest and Plugin Paths and Files
	if (register_paths(vm) != 0)
	{
		benthos_dc_registry_cleanup();
		return 1;
	}

	// Show the Driver List
	if (vm.count("list"))
	{
		list_drivers();
		benthos_dc_registry_cleanup();
		return 0;
	}

	// Test Plugins
	if (vm.count("test"))
	{
		rv = test_drivers();
		benthos_dc_registry_cleanup();
		return rv;
	}

	// Run the Transfer
	rv = run_transfer(vm);

	// Cleanup
	benthos_dc_registry_cleanup();

	// Exit
	return rv;
}
