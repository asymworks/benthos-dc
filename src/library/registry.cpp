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

#include <cerrno>

#include <algorithm>
#include <list>
#include <map>
#include <string>

#include <dlfcn.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <benthos/divecomputer/config.h>
#include <benthos/divecomputer/registry.h>

#include <benthos/divecomputer/plugin/plugin.h>

#include "iterators.hpp"

//! Case-Insensitive Comparator
struct ci_cmp : public std::binary_function<std::string, std::string, bool>
{
    bool operator()(const std::string & lhs, const std::string & rhs) const
    {
        return boost::lexicographical_compare(lhs, rhs, boost::is_iless());
    }
};

//! Plugin Entry Structure
typedef struct
{
	std::string					lib_name;
	std::string					lib_path;

	void *						lib_handle;
	plugin_load_fn_t			lib_load_fn;
	plugin_unload_fn_t			lib_unload_fn;
	plugin_driver_table_fn_t	lib_driver_fn;

} library_entry_t;

//! Plugin Table
typedef std::map<std::string, plugin_manifest_t, ci_cmp>	manifest_table;

//! Library Table
typedef std::map<std::string, library_entry_t, ci_cmp>		library_table;

//! Registry Structure
typedef struct
{
	/* Manifest Paths and File Names */
	std::list<std::string>				manifest_paths;
	std::list<std::string>				manifest_files;

	/* Plugin Paths and File Names */
	std::list<std::string>				plugin_paths;
	std::list<std::string>				plugin_files;

	/* Plugin Library Entries */
	library_table						libraries;

	/* Plugin Manifest Entries */
	manifest_table						manifests;

	/* List of Drivers */
	std::list<const driver_info_t *>	drivers;

} plugin_registry_t;

//! Global Registry Pointer
static plugin_registry_t *		g_registry = 0;

/* Registry Driver Iterator Data Structure */
struct registry_drvit_data
{
	std::list<const driver_info_t *>::const_iterator it;
};

/* Registry Driver Iterator Info Function */
const driver_info_t * registry_drvit_info(void * arg)
{
	struct registry_drvit_data * data = static_cast<struct registry_drvit_data *>(arg);
	if (! data || ! g_registry || (data->it == g_registry->drivers.end()))
		return 0;

	return * data->it;
}

int registry_drvit_next(void * arg)
{
	struct registry_drvit_data * data = static_cast<struct registry_drvit_data *>(arg);
	if (! data || ! g_registry)
		return 0;

	data->it++;
	if (data->it == g_registry->drivers.end())
		return 0;

	return 1;
}

void registry_drvit_dispose(void * arg)
{
	struct registry_drvit_data * data = static_cast<struct registry_drvit_data *>(arg);
	if (! data || ! g_registry)
		return ;

	free(data);
}

static struct driver_iterator_fn_table registry_drvit_impl =
{
	registry_drvit_info,
	registry_drvit_next,
	registry_drvit_dispose
};

int register_manifest(const std::string & manifest_file)
{
	int rv;
	plugin_manifest_t m;
	driver_iterator_t it;
	const plugin_info_t * pi;
	const driver_info_t * di;

	/* Load the Manifest */
	rv = benthos_dc_manifest_parse(& m, manifest_file.c_str());
	if (rv != 0)
		return rv;

	pi = benthos_dc_manifest_plugin(m);
	if (! pi)
		return REGISTRY_ERR_INVALID;

	/* Check if the Plugin is already Registered */
	if (g_registry->manifests.find(pi->plugin_name) != g_registry->manifests.end())
		return REGISTRY_ERR_EXISTS;

	/* Register the Manifest */
	g_registry->manifests.insert(std::pair<std::string, plugin_manifest_t>(pi->plugin_name, m));

	/* Register the Drivers */
	it = benthos_dc_manifest_drivers(m);
	while ((di = benthos_dc_driver_iterator_info(it)) != 0)
	{
		g_registry->drivers.push_back(di);
		benthos_dc_driver_iterator_next(it);
	}

	/* Success */
	return 0;
}

void scan_manifest_path(const std::string & path)
{
	boost::filesystem::path p(path);
	boost::filesystem::directory_iterator end_;
	for (boost::filesystem::directory_iterator it(p); it != end_; it++)
	{
		std::string fn(it->path().native());
		if (boost::filesystem::is_directory(it->status()))
			scan_manifest_path(fn);
		else if (fn.find(".xml", fn.length() - 4) != std::string::npos)
			register_manifest(fn);
	}
}

std::string scan_plugin_path(const std::string & path, const std::string & library_name)
{
	boost::regex e("(lib)?" + library_name + "\\.(so|dll|dylib)$");

	boost::filesystem::path p(path);
	if (! boost::filesystem::exists(p))
		return std::string();

	boost::filesystem::directory_iterator end_;
	for (boost::filesystem::directory_iterator it(p); it != end_; it++)
	{
		std::string fn(it->path().native());
		if (boost::filesystem::is_directory(it->status()))
		{
			return scan_plugin_path(fn, library_name);
		}
		else
		{
			if (boost::regex_search(fn, e))
				return boost::filesystem::canonical(fn).string();
		}
	}

	return std::string();
}

std::string find_plugin(const std::string & library_name)
{
	boost::regex e("(lib)?" + library_name + "\\.(so|dll|dylib)$");
	std::list<std::string>::const_iterator it;

	/* Scan Plugin Files */
	for (it = g_registry->plugin_files.begin(); it != g_registry->plugin_files.end(); it++)
	{
		if (boost::regex_search(* it, e))
			return * it;
	}

	/* Scan Plugin Paths */
	for (it = g_registry->plugin_paths.begin(); it != g_registry->plugin_paths.end(); it++)
	{
		std::string ret = scan_plugin_path(* it, library_name);
		if (! ret.empty())
			return ret;
	}

	/* Not Found */
	return std::string();
}

int load_plugin(const plugin_info_t * pi, const library_entry_t ** lib, int * load_ret)
{
	int rv;
	library_entry_t le;
	library_table::const_iterator it;
	std::string library_path;

	/* Check if the Library is Loaded */
	it = g_registry->libraries.find(std::string(pi->plugin_name));
	if (it != g_registry->libraries.end())
	{
		* lib = & it->second;

		return 0;
	}

	/* Find the Library File in the Plugin Paths */
	library_path = find_plugin(pi->plugin_library);
	if (library_path.empty())
		return REGISTRY_ERR_NOTFOUND;

	/* Load the Library */
	le.lib_name = pi->plugin_library;
	le.lib_path = library_path;

	le.lib_handle = dlopen(library_path.c_str(), RTLD_LAZY);
	if (! le.lib_handle)
		return REGISTRY_ERR_DLOPEN;

	le.lib_load_fn = (plugin_load_fn_t)dlsym(le.lib_handle, "plugin_load");
	if (! le.lib_load_fn)
	{
		dlclose(le.lib_handle);
		return REGISTRY_ERR_DLSYM_LOAD;
	}

	le.lib_unload_fn = (plugin_unload_fn_t)dlsym(le.lib_handle, "plugin_unload");
	if (! le.lib_unload_fn)
	{
		dlclose(le.lib_handle);
		return REGISTRY_ERR_DLSYM_UNLOAD;
	}

	le.lib_driver_fn = (plugin_driver_table_fn_t)dlsym(le.lib_handle, "plugin_load_driver");
	if (! le.lib_driver_fn)
	{
		dlclose(le.lib_handle);
		return REGISTRY_ERR_DLSYM_DRIVER;
	}

	/* Load the Plugin */
	rv = le.lib_load_fn();
	if (rv != 0)
	{
		dlclose(le.lib_handle);
		if (load_ret)
			* load_ret = rv;

		return REGISTRY_ERR_PLUGIN_LOAD;
	}

	if (load_ret)
		* load_ret = 0;

	/* Add the Library Entry to the Registry */
	g_registry->libraries.insert(std::pair<std::string, library_entry_t>(pi->plugin_name, le));

	/* Return the Library Entry */
	* lib = & g_registry->libraries.at(pi->plugin_name);

	return 0;
}

int benthos_dc_registry_add_manifest(const char * manifest_file)
{
	boost::filesystem::path p(manifest_file);
	boost::filesystem::path cp;
	std::string path;

	if (! g_registry)
		return REGISTRY_ERR_NOTINIT;

	/* Check that the Path Exists */
	if (! boost::filesystem::exists(p))
		return REGISTRY_ERR_NOTFOUND;

	/* Generate the Canonical Path */
	cp = boost::filesystem::canonical(p);
	if (! boost::filesystem::is_regular(cp))
		return REGISTRY_ERR_NOTFILE;

	/* Store the Path as a String */
	path = cp.string();

	/* Check if the File is already Added */
	if (std::find(g_registry->manifest_files.begin(), g_registry->manifest_files.end(), path) != g_registry->manifest_files.end())
		return REGISTRY_ERR_EXISTS;

	/* Register the Manifest */
	g_registry->manifest_files.push_back(cp.string());

	/* Load the Manifest */
	return register_manifest(cp.string());
}

int benthos_dc_registry_add_plugin(const char * plugin_file)
{
	boost::filesystem::path p(plugin_file);
	boost::filesystem::path cp;
	std::string path;

	if (! g_registry)
		return REGISTRY_ERR_NOTINIT;

	/* Check that the Path Exists */
	if (! boost::filesystem::exists(p))
		return REGISTRY_ERR_NOTFOUND;

	/* Generate the Canonical Path */
	cp = boost::filesystem::canonical(p);
	if (! boost::filesystem::is_regular(cp))
		return REGISTRY_ERR_NOTFILE;

	/* Check if the File is already Added */
	if (std::find(g_registry->plugin_files.begin(), g_registry->plugin_files.end(), cp.string()) != g_registry->plugin_files.end())
		return REGISTRY_ERR_EXISTS;

	/* Add the Plugin File */
	g_registry->plugin_files.push_back(cp.string());

	/* Success */
	return 0;
}

int benthos_dc_registry_add_manifest_path(const char * manifest_path)
{
	boost::filesystem::path p(manifest_path);
	boost::filesystem::path cp;

	if (! g_registry)
		return REGISTRY_ERR_NOTINIT;

	/* Check that the Path exists and is a Directory */
	if (! boost::filesystem::exists(p))
		return REGISTRY_ERR_NOTFOUND;

	/* Generate the Canonical Path */
	cp = boost::filesystem::canonical(p);
	if (! boost::filesystem::is_directory(cp))
		return REGISTRY_ERR_NOTDIR;

	/* Check if the Path is already Added */
	if (std::find(g_registry->manifest_paths.begin(), g_registry->manifest_paths.end(), cp.string()) != g_registry->manifest_paths.end())
		return REGISTRY_ERR_EXISTS;

	/* Add the Manifest Path */
	g_registry->manifest_paths.push_back(cp.string());

	/* Scan the Manifest Path */
	scan_manifest_path(cp.string());

	/* Success */
	return 0;
}

int benthos_dc_registry_add_plugin_path(const char * plugin_path)
{
	boost::filesystem::path p(plugin_path);
	boost::filesystem::path cp;
	std::string path;

	if (! g_registry)
		return REGISTRY_ERR_NOTINIT;

	/* Check that the Path exists and is a Directory */
	if (! boost::filesystem::exists(p))
		return REGISTRY_ERR_NOTFOUND;

	/* Generate the Canonical Path */
	cp = boost::filesystem::canonical(p);
	if (! boost::filesystem::is_directory(cp))
		return REGISTRY_ERR_NOTDIR;

	/* Check if the Path is already Added */
	if (std::find(g_registry->plugin_paths.begin(), g_registry->plugin_paths.end(), cp.string()) != g_registry->plugin_paths.end())
		return REGISTRY_ERR_EXISTS;

	/* Add the Plugin Path */
	g_registry->plugin_paths.push_back(cp.string());

	/* Success */
	return 0;
}

void benthos_dc_registry_cleanup(void)
{
	manifest_table::iterator mit;
	library_table::iterator lit;

	if (! g_registry)
		return;

	/* Unload Loaded Plugins */
	for (lit = g_registry->libraries.begin(); lit != g_registry->libraries.end(); lit++)
	{
		lit->second.lib_unload_fn();

		dlclose(lit->second.lib_handle);
	}

	/* Dispose of Manifest Data */
	for (mit = g_registry->manifests.begin(); mit != g_registry->manifests.end(); mit++)
		benthos_dc_manifest_dispose(mit->second);

	/* Free the Registry */
	delete g_registry;
	g_registry = 0;
}

driver_iterator_t benthos_dc_registry_drivers(void)
{
	driver_iterator_t it;

	if (! g_registry)
		return 0;

	it = (driver_iterator_t)malloc(sizeof(struct driver_iterator_t_));
	if (! it)
		return 0;

	struct registry_drvit_data * data = (struct registry_drvit_data *)malloc(sizeof(struct registry_drvit_data));
	if (! data)
		return 0;

	data->it = g_registry->drivers.begin();

	it->data = data;
	it->impl = & registry_drvit_impl;

	return it;
}

int benthos_dc_registry_driver_info(const char * name, const driver_info_t ** info)
{
	std::list<const driver_info_t *>::const_iterator it;
	std::string driver(name);
	std::string plugin;
	const driver_info_t * ret;
	size_t pos;

	if (! name || ! info)
		return EINVAL;
	if (! g_registry)
		return REGISTRY_ERR_NOTINIT;

	/* Split the Plugin Name if Present */
	if ((pos = driver.find(".")) != std::string::npos)
	{
		plugin = driver.substr(0, pos);
		driver = driver.substr(pos+1);
	}

	/* Lookup Driver */
	ret = 0;
	for (it = g_registry->drivers.begin(); it != g_registry->drivers.end(); it++)
	{
		const driver_info_t * found = 0;

		if (! plugin.empty() && (strcasecmp((* it)->plugin->plugin_name, plugin.c_str()) != 0))
			continue;

		if (strcasecmp((* it)->driver_name, driver.c_str()) == 0)
			found = (* it);

		if (found && ret)
		{
			/* Found Multiple Drivers */
			return REGISTRY_ERR_AMBIGUOUS;
		}

		ret = found;
	}

	/* Return Driver Info */
	if (! ret)
		return REGISTRY_ERR_NOTFOUND;

	* info = ret;

	return 0;
}

int benthos_dc_registry_load(const char * name, const driver_interface_t ** intf)
{
	int rv, load_rv;
	const driver_info_t * di;
	const library_entry_t * li;

	if (! name || ! intf)
		return EINVAL;
	if (! g_registry)
		return REGISTRY_ERR_NOTINIT;

	/* Load the Driver Information */
	rv = benthos_dc_registry_driver_info(name, & di);
	if (rv != 0)
		return rv;

	/* Find the Plugin Library */
	rv = load_plugin(di->plugin, & li, & load_rv);
	if (rv != 0)
		return rv;

	/* Load the Driver */
	* intf = li->lib_driver_fn(di->driver_name);
	if (! (* intf))
		return REGISTRY_ERR_NOTINPLUGIN;

	/* Success */
	return 0;
}

int benthos_dc_registry_plugin_info(const char * name, const plugin_info_t ** info)
{
	manifest_table::const_iterator it;

	if (! name || ! info)
		return EINVAL;
	if (! g_registry)
		return REGISTRY_ERR_NOTINIT;

	it = g_registry->manifests.find(std::string(name));
	if (it == g_registry->manifests.end())
		return REGISTRY_ERR_NOTFOUND;

	* info = benthos_dc_manifest_plugin(it->second);

	return 0;
}

int benthos_dc_registry_init(void)
{
	if (g_registry)
		return 0;

	/* Allocate the Registry */
	g_registry = new plugin_registry_t;
	if (! g_registry)
		return 1;

	/* Initialize Registry Paths */
#if defined(BENTHOS_DC_MANIFESTDIR)
	benthos_dc_registry_add_manifest_path(BENTHOS_DC_MANIFESTDIR);
#endif
#if defined(BENTHOS_DC_PLUGINDIR)
	benthos_dc_registry_add_plugin_path(BENTHOS_DC_PLUGINDIR);
#endif

	/* Success */
	return 0;
}

const char * benthos_dc_registry_strerror(int c)
{
	switch (c)
	{
	case REGISTRY_ERR_NOTINIT:
		return "Registry not initialized";
	case REGISTRY_ERR_NOTFOUND:
		return "File or path not found";
	case REGISTRY_ERR_NOTDIR:
		return "Manifest or Plugin Path is not a directory";
	case REGISTRY_ERR_NOTFILE:
		return "Manifest or Plugin Path is not a file";
	case REGISTRY_ERR_EXISTS:
		return "Plugin or driver already exists in registry";
	case REGISTRY_ERR_INVALID:
		return "Manifest file is invalid";
	case REGISTRY_ERR_AMBIGUOUS:
		return "Ambigious driver name (use plugin.driver)";
	case REGISTRY_ERR_DLOPEN:
		return "Could not load shared library";
	case REGISTRY_ERR_DLSYM_LOAD:
		return "Plugin shared library does not export plugin_load()";
	case REGISTRY_ERR_DLSYM_UNLOAD:
		return "Plugin shared library does not export plugin_unload()";
	case REGISTRY_ERR_DLSYM_DRIVER:
		return "Plugin shared library does not export plugin_load_driver()";
	case REGISTRY_ERR_NOTINPLUGIN:
		return "Driver not provided by plugin shared library";
	}

	return "Unknown Error";
}
