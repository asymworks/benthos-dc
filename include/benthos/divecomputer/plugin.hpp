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

#ifndef BENTHOS_DC_PLUGIN_HPP_
#define BENTHOS_DC_PLUGIN_HPP_

/**
 * @file include/benthos/divecomputer/plugin.hpp
 * @brief Benthos Plugin Wrapper Class
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <string>

#include <benthos/divecomputer/plugin/plugin.h>
#include <benthos/divecomputer/registry.hpp>

namespace benthos
{

/**
 * @brief Benthos Plugin Class
 *
 * Provides a wrapper around a plugin, with an object-oriented interface to the
 * plugin attributes as well as managing the shared library lifetime.
 */
class Plugin
{
public:
	typedef boost::shared_ptr<Plugin>	Ptr;

public:

	/**
	 * @brief Class Constructor
	 * @param[in] Plugin Manifest Entry
	 * @param[in] Shared Library Path
	 *
	 * Loads the plugin specified in the shared library path.  If the library
	 * cannot be found or is not a valid Benthos plugin an exception will be
	 * raised.  The constructor calls the plugin_load() entry point.
	 */
	Plugin(const plugin_manifest_t * manifest, const std::string & lib_path);

	/**
	 * @brief Class Destructor
	 *
	 * Calls the plugin_unload() entry point and unloads the shared library.
	 */
	~Plugin();

public:

	//! @return Driver Instance
	PluginRegistry::DriverClassPtr driver(const std::string & name);

	//! @return List of Drivers
	std::list<std::string> drivers() const;

	//! @return Plugin Library Name
	const std::string & library() const;

	//! @return Plugin Name
	const std::string & name() const;

	//! @return Plugin Library Path
	const std::string & path() const;

	//! @return Plugin Major Version
	int version_major() const;

	//! @return Plugin Minor Version
	int version_minor() const;

	//! @return Plugin Patch Version
	int version_patch() const;

private:
	const plugin_manifest_t *							m_manifest;
	std::string											m_lib_path;
	void *												m_lib_handle;

	plugin_load_fn_t									m_load_fn;
	plugin_unload_fn_t									m_unload_fn;
	plugin_driver_table_fn_t							m_driver_fn;

	std::map<std::string, const driver_interface_t *>	m_driver_tables;

};

} /* benthos */

#endif /* BENTHOS_DC_PLUGIN_HPP_ */
