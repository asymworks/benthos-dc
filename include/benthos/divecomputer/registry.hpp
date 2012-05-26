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

#ifndef BENTHOS_DC_REGISTRY_HPP_
#define BENTHOS_DC_REGISTRY_HPP_

/**
 * @file include/benthos/divecomputer/registry.hpp
 * @brief Dive Computer Driver Plugin Registry
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <cstdint>
#include <list>
#include <map>
#include <set>
#include <string>

#include <boost/shared_ptr.hpp>

namespace benthos
{

/**
 * @brief Parameter Type Enumeration
 */
typedef enum
{
	ptString,		///< String Parameter
	ptInt,			///< Integer Parameter
	ptUInt,			///< Unsigned Integer Parameter
	ptFloat			///< Floating-Point Parameter

} param_type_t;

/**
 * @brief Parameter Specification Entry
 *
 * Holds parsed information about a driver parameter including name, type,
 * description and default value.  This information is used as hints to the
 * GUI when displaying driver configuration options and is not guaranteed
 * to be the set of parameters the driver implementation loads.
 */
typedef struct
{
	std::string			param_name;
	std::string			param_desc;
	param_type_t		param_type;
	std::string			param_default;

} param_info_t;

/**
 * @brief Device Model Information
 *
 * Holds hints for the GUI about a model number's actual model and manufacturer
 * name.  This is used as GUI hints only and cannot be used to influence driver
 * or parser behavior.
 */
typedef struct
{
	std::string			model_name;
	std::string			manuf_name;

} model_info_t;

/**
 * @brief Driver Plugin Entry
 *
 * Holds parsed information about a device driver present in a plugin.  This
 * structure is created by parsing the XML manifest file for a plugin and is
 * stored as part of the Plugin Registry Entry.
 */
typedef struct
{
	std::string					plugin_name;

	std::string					driver_name;
	std::string					driver_desc;
	std::string					driver_intf;

	std::list<param_info_t>		driver_params;
	std::map<int, model_info_t>	driver_models;

} driver_manifest_t;

/**
 * @brief Plugin Registry Entry
 *
 * Holds parsed information about a plugin.  This structure is created by
 * parsing the XML manifest file for the plugin and is stored in the registry
 * as the primary holder of plugin information.
 *
 * Note that having an instance of this structure present does not imply that
 * the library is loaded; library load is on-demand when a plugin is requested.
 */
typedef struct
{
	std::string						plugin_name;
	std::string						plugin_library;

	unsigned char					plugin_major_version;
	unsigned char					plugin_minor_version;
	unsigned char					plugin_patch_version;

	std::list<driver_manifest_t>	plugin_drivers;

} plugin_manifest_t;

// Forward Definition of Plugin and DriverClass classes
class Plugin;
class DriverClass;

/**
 * @brief Dive Computer Plugin Registry Class
 *
 * Singleton class which provides access to dynamically-loaded plugin modules
 * to provide dive computer input and output.  To use the registry, the search
 * paths must first be initialized with addPluginManifestPath() and
 * addPluginLibraryPath().
 *
 * If the library is configured with BENTHOS_DC_MANIFESTDIR and
 * BENTHOS_DC_PLUGINDIR defined, they will be added to the search paths by
 * default.
 */
class PluginRegistry
{
public:
	typedef boost::shared_ptr<PluginRegistry>	Ptr;

	typedef boost::shared_ptr<Plugin>			PluginPtr;
	typedef boost::shared_ptr<DriverClass>		DriverClassPtr;

protected:

	//! Class Constructor (called by singleton accessor)
	PluginRegistry();

public:

	/**
	 * @brief Return a Singleton Instance of the Plugin Registry
	 * @return Singleton Instance
	 */
	static Ptr Instance();

	//! Class Destructor
	~PluginRegistry();

public:

	/**
	 * @brief Add a Plugin Library Search Path
	 * @param[in] Search Path
	 */
	void addPluginLibraryPath(const std::string & path);

	/**
	 * @brief Add a Plugin Manifest Search Path
	 * @param[in] Search Path
	 */
	void addPluginManifestPath(const std::string & path);

	/**
	 * @brief Lookup Driver by Name
	 * @param[in] Driver Name
	 * @return Driver Entry
	 */
	const driver_manifest_t * findDriver(const std::string & name) const;

	/**
	 * @brief Lookup Plugin by Name
	 * @param[in] Plugin Name
	 * @return Plugin Manifest or NULL
	 */
	const plugin_manifest_t * findPlugin(const std::string & name) const;

	/**
	 * @brief Load a Driver by Name
	 * @param[in] Driver Name
	 * @return Driver Pointer
	 *
	 * Loads the driver specified in the name parameter from a plugin module.
	 * This may cause the appropriate plugin library to be loaded, equivalent
	 * to calling loadPlugin().  The returned pointer is not owned by the
	 * registry.
	 */
	DriverClassPtr loadDriver(const std::string & name);

	/**
	 * @brief Load a Plugin by Name
	 * @param[in] Plugin Name
	 * @return Plugin Pointer
	 *
	 * Loads the plugin specified in the name parameter and calls appropriate
	 * initialization function.  The loaded plugin is added to the registry and
	 * will not be re-loaded during the lifetime of the registry.  The plugin
	 * will be unloaded when the registry is destroyed.
	 */
	PluginPtr loadPlugin(const std::string & name);

protected:

	/**
	 * @brief Find a Plugin Library
	 * @param[in] Plugin Library Name
	 * @return Plugin Library Path
	 */
	std::string findPluginPath(const std::string & lib_name) const;

	/**
	 * @brief Load a Manifest File into the Registry
	 * @param[in] Manifest File Path
	 *
	 * Loads a manifest file into the registry, creating a new entry in the
	 * m_plugins table.  Raises an exception if the manifest cannot be loaded.
	 */
	void loadManifest(const std::string & path);

	/**
	 * @brief Scan a Manifest Path for Plugin Manifests
	 * @param[in] Manifest Search Path
	 *
	 * Scans the given path for manifest files and for those found, inserts
	 * a plugin manifest descriptor into the registry.  This method is called
	 * internally by addPluginManifestPath().
	 */
	void scanManifestPath(const std::string & path);

	/**
	 * @brief Scan a Library Path for a Plugin
	 * @param[in] Library Path
	 * @param[in] Library Name
	 * @return Path if Found else Empty String
	 */
	std::string scanPluginPath(const std::string & path, const std::string & lib_name) const;

private:
	static Ptr							s_instance;

private:
	std::set<std::string>				m_manifest_paths;
	std::set<std::string>				m_library_paths;

	std::list<plugin_manifest_t>		m_plugins;
	std::map<std::string, PluginPtr>	m_loaded;

};

} /* benthos */

#endif /* BENTHOS_DC_REGISTRY_HPP_ */
