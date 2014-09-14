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

#ifndef BENTHOS_DC_REGISTRY_H_
#define BENTHOS_DC_REGISTRY_H_

/**
 * @file include/benthos/divecomputer/registry.h
 * @brief Dive Computer Driver Plugin Registry
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <benthos/divecomputer/manifest.h>
#include <benthos/divecomputer/plugin/driver.h>
#include <benthos/divecomputer/plugin/plugin.h>

/**@{
 * @brief Registry Error Codes
 */
#define REGISTRY_ERR_NOTINIT		-1		///< Registry Not Initialized
#define REGISTRY_ERR_NOTFOUND		-2		///< Plugin, Driver, or Path Not Found
#define REGISTRY_ERR_NOTDIR			-3		///< Path is not a Directory
#define REGISTRY_ERR_NOTFILE		-4		///< Path is not a File
#define REGISTRY_ERR_EXISTS			-5		///< Driver Already Registered
#define REGISTRY_ERR_INVALID		-6		///< Invalid Argument
#define REGISTRY_ERR_AMBIGUOUS		-7		///< Ambiguous Driver Name
#define REGISTRY_ERR_DLOPEN			-8		///< Failed to Open Shared Library
#define REGISTRY_ERR_DLSYM_LOAD		-9		///< Library does not contain plugin_load
#define REGISTRY_ERR_DLSYM_UNLOAD	-10		///< Library does not contain plugin_unload
#define REGISTRY_ERR_DLSYM_DRIVER	-11		///< Library does not contain plugin_load_driver
#define REGISTRY_ERR_PLUGIN_LOAD	-12		///< Failed to load Plugin for Manifest
#define REGISTRY_ERR_NOTINPLUGIN	-13		///< Driver is not provided by Plugin
/*@}*/

/**
 * @brief Initialize the Plugin Registry
 * @return Zero on Success, Non-Zero on Failure
 *
 * Initializes the Plugin Registry and performs an initial scan of pre-defined
 * plugin and manifest directories.  Call this function before calling any
 * other registry functions.
 */
int benthos_dc_registry_init(void);

/**
 * @brief Clean Up the Plugin Registry
 *
 * Releases registry resources.  Call this function prior to program exit.
 * This method will call plugin_unload() on all loaded plugins.
 */
void benthos_dc_registry_cleanup(void);

//! @return Error String for an Error Code
const char * benthos_dc_registry_strerror(int errcode);

/**
 * @brief Add a Manifest File to the Registry
 * @param[in] path Manifest File
 * @return Zero on Success, Non-Zero on Failure
 *
 * Adds a single manifest file to the registry.
 */
int benthos_dc_registry_add_manifest(const char * path);

/**
 * @brief Add a Plugin File to the Registry
 * @param[in] path Plugin File
 * @return Zero on Success, Non-Zero on Failure
 *
 * Adds a single plugin library file to the registry.
 */
int benthos_dc_registry_add_plugin(const char * path);

/**
 * @brief Add a Manifest Path to the Registry
 * @param[in] path Manifest Path
 * @return Zero on Success, Non-Zero on Failure
 *
 * Adds a directory to the registry which will be scanned for manifest
 * files.  The default system manifest directory is added by default and
 * does not need to be manually added.  Calling this function will cause
 * the given directory to be scanned and all manifests found to be registered
 * with the registry.
 */
int benthos_dc_registry_add_manifest_path(const char * path);

/**
 * @brief Add a Plugin Path to the Registry
 * @param[in] path Plugin Path
 * @return Zero on Success, Non-Zero on Failure
 *
 * Adds a directory to the registry which will be scanned for plugin library
 * files.  The default system plugin directory is added by default and
 * does not need to be manually added.  Calling this function will cause
 * the given directory to be scanned and all plugins found to be registered
 * with the registry.
 */
int benthos_dc_registry_add_plugin_path(const char * path);

/**
 * @brief Return an Iterator for all Registered Drivers
 * @return Driver Iterator
 *
 * Returns an interator which will iterate through all registered drivers
 * in the registry.
 */
driver_iterator_t benthos_dc_registry_drivers(void);

/**
 * @brief Find Plugin Information by Name
 * @param[in] plugin Plugin Name
 * @param[out] pi Plugin Information
 * @return Zero on Success, Non-Zero on Failure
 */
int benthos_dc_registry_plugin_info(const char * plugin, const plugin_info_t ** pi);

/**
 * @brief Find Driver Information by Name
 * @param[in] driver Driver Name
 * @param[out] di Driver Information
 * @return Zero on Success, Non-Zero on Failure
 *
 * The driver name may be given either as a single name or as a dotted path
 * of the form plugin.driver which will load a driver from a specific plugin
 * when multiple plugins have a driver with the same name.
 *
 * If there are multiple plugins with drivers with the same name and the name
 * does not have the plugin specified, the function will return
 * REGISTRY_ERR_AMBIGUOUS.
 */
int benthos_dc_registry_driver_info(const char * driver, const driver_info_t ** di);

/**
 * @brief Load a Driver Function Table
 * @param[in] driver Driver Name
 * @param[out] intf Driver Table
 * @return Zero on Success, Non-Zero on Failure
 *
 * Loads the specified driver function table.  This will load the plugin
 * library if it is not already loaded.  The plugin library will be
 * automatically unloaded when benthos_dc_registry_cleanup() is called so
 * the client does not have to manually unload any plugins.
 *
 * The driver name may be given either as a single name or as a dotted path
 * of the form plugin.driver which will load a driver from a specific plugin
 * when multiple plugins have a driver with the same name.
 *
 * If there are multiple plugins with drivers with the same name and the name
 * does not have the plugin specified, the function will return
 * REGISTRY_ERR_AMBIGUOUS.
 */
int benthos_dc_registry_load(const char * driver, const driver_interface_t ** intf);

#ifdef __cplusplus
}
#endif

#endif /* BENTHOS_DC_REGISTRY_H_ */
