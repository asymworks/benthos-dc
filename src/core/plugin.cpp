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

#include <cerrno>
#include <cstring>
#include <dlfcn.h>

#include <stdexcept>
#include <string>

#include <benthos/divecomputer/driverclass.hpp>
#include <benthos/divecomputer/plugin.hpp>
#include <benthos/divecomputer/registry.hpp>

using namespace benthos::dc;

Plugin::Plugin(const plugin_manifest_t * manifest, const std::string & lib_path)
	: m_manifest(manifest), m_lib_path(lib_path), m_lib_handle(0),
	  m_load_fn(0), m_unload_fn(0), m_driver_fn(0), m_driver_tables()
{
	if (! m_manifest)
		throw std::runtime_error("Invalid plugin manifest");

	if (m_lib_path.empty())
		throw std::runtime_error("Invalid library path");

	// Open the Dynamic Library
	m_lib_handle = dlopen(m_lib_path.c_str(), RTLD_LAZY);
	if (! m_lib_handle)
		throw std::runtime_error("Failed to load library: " + std::string(dlerror()));

	// Load the Entry Points
	m_load_fn = (plugin_load_fn_t)dlsym(m_lib_handle, "plugin_load");
	if (! m_load_fn)
		throw std::runtime_error("Entry point 'plugin_load' does not exist in '" + m_lib_path);

	m_unload_fn = (plugin_unload_fn_t)dlsym(m_lib_handle, "plugin_unload");
	if (! m_unload_fn)
		throw std::runtime_error("Entry point 'plugin_unload' does not exist in '" + m_lib_path);

	m_driver_fn = (plugin_driver_table_fn_t)dlsym(m_lib_handle, "plugin_load_driver");
	if (! m_driver_fn)
		throw std::runtime_error("Entry point 'plugin_load_driver' does not exist in '" + m_lib_path);

	// Initialize the Plugin
	if (m_load_fn())
		throw std::runtime_error("Failed to initialize plugin: " + std::string(strerror(errno)));
}

Plugin::~Plugin()
{
	// Unload the Plugin
	if (m_lib_handle != 0)
	{
		dlclose(m_lib_handle);

		m_lib_handle = 0;
		m_load_fn = 0;
		m_unload_fn = 0;
		m_driver_fn = 0;
	}
}

PluginRegistry::DriverClassPtr Plugin::driver(const std::string & name)
{
	const driver_manifest_t * mf = 0;
	const driver_interface_t * tbl = 0;

	std::list<driver_manifest_t>::const_iterator it;
	for (it = m_manifest->plugin_drivers.begin(); it != m_manifest->plugin_drivers.end(); it++)
	{
		if (it->driver_name == name)
			mf = & (* it);
	}

	if (! mf)
		throw std::runtime_error("Driver class '" + name + "' is not provided by plugin '" + m_manifest->plugin_name + "'");

	std::map<std::string, const driver_interface_t *>::iterator it2 = m_driver_tables.find(name);
	if (it2 == m_driver_tables.end())
	{
		tbl = m_driver_fn(name.c_str());
		if (tbl == NULL)
			throw std::runtime_error("Driver class '" + name + "' is not provided by plugin '" + m_manifest->plugin_name + "'");

		m_driver_tables.insert(std::pair<std::string, const driver_interface_t *>(name, tbl));
	}
	else
		tbl = it2->second;

	return PluginRegistry::DriverClassPtr(new DriverClass(mf, tbl));
}

std::list<std::string> Plugin::drivers() const
{
	std::list<std::string> result;
	std::list<driver_manifest_t>::const_iterator it;
	for (it = m_manifest->plugin_drivers.begin(); it != m_manifest->plugin_drivers.end(); it++)
		result.push_back(it->driver_name);
	return result;
}

const std::string & Plugin::library() const
{
	return m_manifest->plugin_library;
}

const std::string & Plugin::name() const
{
	return m_manifest->plugin_name;
}

const std::string & Plugin::path() const
{
	return m_lib_path;
}

int Plugin::version_major() const
{
	return m_manifest->plugin_major_version;
}

int Plugin::version_minor() const
{
	return m_manifest->plugin_minor_version;
}

int Plugin::version_patch() const
{
	return m_manifest->plugin_patch_version;
}
