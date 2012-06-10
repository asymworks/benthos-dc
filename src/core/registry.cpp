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

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <benthos/divecomputer/config.hpp>
#include <benthos/divecomputer/plugin.hpp>
#include <benthos/divecomputer/registry.hpp>

#include "manifest_parser.hpp"

using namespace benthos::dc;

PluginRegistry::Ptr PluginRegistry::s_instance;
PluginRegistry::Ptr PluginRegistry::Instance()
{
	if (! s_instance)
	{
		s_instance = Ptr(new PluginRegistry);

#ifdef BENTHOS_DC_MANIFESTDIR
		s_instance->addPluginManifestPath(BENTHOS_DC_MANIFESTDIR);
#endif
#ifdef BENTHOS_DC_PLUGINDIR
		s_instance->addPluginLibraryPath(BENTHOS_DC_PLUGINDIR);
#endif
	}

	return s_instance;
}

PluginRegistry::PluginRegistry()
	: m_manifest_paths(), m_library_paths(), m_plugins(), m_loaded()
{
}

PluginRegistry::~PluginRegistry()
{
}

void PluginRegistry::addPluginLibraryPath(const std::string & path)
{
	std::pair<std::set<std::string>::iterator, bool> ret = m_library_paths.insert(path);
}

void PluginRegistry::addPluginManifestPath(const std::string & path)
{
	std::pair<std::set<std::string>::iterator, bool> ret = m_manifest_paths.insert(path);
	if (ret.second)
		scanManifestPath(path);
}

const driver_manifest_t * PluginRegistry::findDriver(const std::string & name) const
{
	std::list<plugin_manifest_t>::const_iterator it;
	for (it = m_plugins.begin(); it != m_plugins.end(); it++)
	{
		std::list<driver_manifest_t>::const_iterator it2;
		for (it2 = it->plugin_drivers.begin(); it2 != it->plugin_drivers.end(); it2++)
		{
			if (it2->driver_name == name)
				return & (* it2);
		}
	}

	return NULL;
}

PluginRegistry::DriverClassPtr PluginRegistry::loadDriver(const std::string & name)
{
	// Find Driver
	const driver_manifest_t * dm = findDriver(name);
	if (dm == NULL)
		throw std::runtime_error("Driver '" + name + "' not found");

	// Load the Plugin
	PluginPtr plugin = loadPlugin(dm->plugin_name);
	if (! plugin)
		throw std::runtime_error("Failed to load plugin '" + dm->plugin_name + "'");

	// Load the Driver Class
	DriverClassPtr dcls = plugin->driver(name);
	if (! dcls)
		throw std::runtime_error("Failed to load driver '" + name + "' from plugin '" + dm->plugin_name + "'");

	// Return Driver Class
	return dcls;
}

const plugin_manifest_t * PluginRegistry::findPlugin(const std::string & name) const
{
	std::list<plugin_manifest_t>::const_iterator it;
	for (it = m_plugins.begin(); it != m_plugins.end(); it++)
	{
		if (it->plugin_name == name)
			return & (* it);
	}

	return NULL;
}

PluginRegistry::PluginPtr PluginRegistry::loadPlugin(const std::string & name)
{
	std::map<std::string, PluginPtr>::iterator it = m_loaded.find(name);
	if (it != m_loaded.end())
		return it->second;

	const plugin_manifest_t * m = findPlugin(name);
	if (m == NULL)
		throw std::runtime_error("Plugin '" + name + "' is not registered");

	std::string path = findPluginPath(m->plugin_library);
	if (path.empty())
		throw std::runtime_error("Plugin Library '" + m->plugin_library + "' not found");

	PluginPtr p(new Plugin(m, path));
	m_loaded.insert(std::pair<std::string, PluginPtr>(name, p));
	return p;
}

std::string PluginRegistry::scanPluginPath(const std::string & path, const std::string & lib_name) const
{
	boost::regex e("(lib)?" + lib_name + "\\.(so|dll)$");

	boost::filesystem::path p(path);
	if (! boost::filesystem::exists(p))
		return std::string();

	boost::filesystem::directory_iterator end_;
	for (boost::filesystem::directory_iterator it(p); it != end_; it++)
	{
		std::string fn(it->path().native());
		if (boost::filesystem::is_directory(it->status()))
		{
			return scanPluginPath(fn, lib_name);
		}
		else
		{
			if (boost::regex_search(fn, e))
				return fn;
		}
	}

	return std::string();
}

std::string PluginRegistry::findPluginPath(const std::string & lib_name) const
{
	std::set<std::string>::const_iterator it;
	for (it = m_library_paths.begin(); it != m_library_paths.end(); it++)
	{
		std::string path = scanPluginPath(* it, lib_name);
		if (! path.empty())
			return path;
	}

	return std::string();
}

void PluginRegistry::loadManifest(const std::string & path)
{
	try
	{
		plugin_manifest_t m = parseManifest(path);

		// Check for Plugin Naming Conflicts
		if (findPlugin(m.plugin_name))
			throw std::runtime_error("A Plugin named '" + m.plugin_name + "' is already registered");

		// Check for Driver Naming Conflicts
		std::list<driver_manifest_t>::const_iterator it;
		for (it = m.plugin_drivers.begin(); it != m.plugin_drivers.end(); it++)
			if (findDriver(it->driver_name))
				throw std::runtime_error("A Driver named '" + it->driver_name + "' is already registered");

		// Add the Plugin Information
		m_plugins.push_back(m);
	}
	catch (std::exception & e)
	{
		fprintf(stderr, "%s\n", e.what());
	}
}

void PluginRegistry::scanManifestPath(const std::string & path)
{
	boost::filesystem::path p(path);
	if (! boost::filesystem::exists(p))
		return;

	boost::filesystem::directory_iterator end_;
	for (boost::filesystem::directory_iterator it(p); it != end_; it++)
	{
		std::string fn(it->path().native());
		if (boost::filesystem::is_directory(it->status()))
		{
			scanManifestPath(fn);
		}
		else
		{
			if (fn.find(".xml", fn.length() - 4) != std::string::npos)
			{
				loadManifest(fn);
			}
		}
	}
}

const std::list<plugin_manifest_t> & PluginRegistry::plugins() const
{
	return m_plugins;
}
