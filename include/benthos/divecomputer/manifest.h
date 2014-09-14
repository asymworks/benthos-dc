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

#ifndef BENTHOS_DC_MANIFEST_H_
#define BENTHOS_DC_MANIFEST_H_

/**
 * @file include/benthos/divecomputer/manifest.h
 * @brief Dive Computer Driver Plugin Manifest
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

//! @brief Parameter Type Enumeration
typedef enum
{
	ptUnknown,		///< Unknown Type
	ptString,		///< String Parameter
	ptInt,			///< Integer Parameter
	ptUInt,			///< Unsigned Integer Parameter
	ptFloat,		///< Floating-Point Parameter
	ptModel,		///< Model Number Parameter

} param_type_t;

//! @brief Device Interface Type Enumeration
typedef enum
{
	diUnknown,		///< Unknown Interface
	diSerial,		///< Serial Port Interface
	diUSB,			///< USB Port Interface
	diBluetooth,	///< Bluetooth Interface
	diIrDA,			///< IrDA Interface
	diNetwork,		///< Network Interface

} intf_type_t;

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
	const char *		param_name;			///< Parameter Name
	const char *		param_desc;			///< Parameter Description
	param_type_t		param_type;			///< Parameter Type Code
	const char *		param_default;		///< Parameter Default Value

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
	int					model_number;		///< Model Number
	const char *		model_name;			///< Model Name
	const char *		manuf_name;			///< Manufacturer Name

} model_info_t;

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
	const char *		plugin_name;			///< Plugin Name
	const char *		plugin_library;			///< Plugin Library Name

	unsigned char		plugin_major_version;	///< Major Version Number
	unsigned char		plugin_minor_version;	///< Minor Version Number
	unsigned char		plugin_patch_version;	///< Patch Version Number

} plugin_info_t;

/**
 * @brief Driver Plugin Entry
 *
 * Holds parsed information about a device driver present in a plugin.  This
 * structure is created by parsing the XML manifest file for a plugin and is
 * stored as part of the Plugin Registry Entry.
 *
 * The driver_info_t.model_param specifies a parameter name which is set to
 * the model number by the client.  This gives a way to indicate to the driver
 * which device model type it should expect, if it cannot determine it on its
 * own (e.g. libdivecomputer).  The parameter specified should have the
 * ptModel type code set.
 */
typedef struct
{
	const plugin_info_t *	plugin;			///< Plugin that provides the Driver

	const char *			driver_name;	///< Driver Name
	const char *			driver_desc;	///< Driver Description
	intf_type_t				driver_intf;	///< Driver Interface Type

	unsigned int			n_params;		///< Number of Parameters in .params
	unsigned int			n_models;		///< Number of Models in .models

	const param_info_t **	params;			///< List of supported parameters
	const model_info_t **	models;			///< List of supported models

	const char *			model_param;	///< Parameter to set to the Model Name

} driver_info_t;

//! @brief Plugin Manifest Handle
typedef struct plugin_manifest_t_ *		plugin_manifest_t;

//! @brief Driver Iterator Handle
typedef struct driver_iterator_t_ *		driver_iterator_t;

/**@{
 * @name Manifest Parser Error Codes
 */
#define MANFIEST_ERR_MALFORMED		-1		///< Malformed XML File
#define MANIFEST_ERR_NOROOT			-2		///< No Root Node in XML File
#define MANIFEST_ERR_PARSER			-3		///< Parser Error
#define MANIFEST_ERR_NOTPLUGIN		-4		///< Root node is not \<plugin\>
#define MANIFEST_ERR_NONAME			-5		///< No Plugin Name defined
#define MANIFEST_ERR_NOLIB			-6		///< No Plugin Library defined
/*@}*/

/**
 * @brief Parse an XML Manifest File
 * @param[out] m Manifest Handle
 * @param[in] path Manifest File Path
 * @return Zero on Success, Non-Zero on Failure
 *
 * Parses an XML manifest file and returns a handle which can be used to
 * obtain driver and plugin information.  On platforms which support multi-byte
 * filenames, the path is assumed to be UTF-8 encoded.
 *
 * The returned handle must be released with benthos_dc_manifest_dispose()
 * prior to program exit to avoid memory leaks.
 *
 * This function returns zero on success, and non-zero on failure.  The failure
 * return value will be one of:
 * - MANIFEST_ERR_MALFORMED - Malformed XML in manifest file
 * - MANIFEST_ERR_NOROOT - No root node was found in manifest file
 * - MANIFEST_ERR_NOTPLUGIN - The root node is not \<plugin\>
 * - MANIFEST_ERR_NONAME - No plugin name was specified
 * - MANIFEST_ERR_NOLIB - No library name was specified
 * - MANIFEST_ERR_PARSER - Indicates a parser error occurred.  More informaion
 *   can be obtained by calling the benthos_dc_manifest_errmsg() function.
 * - EINVAL - Invalid arguments were passed to benthos_dc_manifest_parse()
 * - ENOMEM - Out of memory
 * - ENOENT - Manifest file does not exist or is not a file
 */
int benthos_dc_manifest_parse(plugin_manifest_t * m, const char * path);

/**
 * @brief Release memory associated with a Plugin Manifest
 * @param[in] m Manifest Handle
 */
void benthos_dc_manifest_dispose(plugin_manifest_t m);

//! @return Plugin Driver Iterator
driver_iterator_t benthos_dc_manifest_drivers(plugin_manifest_t m);

//! @return Plugin Information
const plugin_info_t * benthos_dc_manifest_plugin(plugin_manifest_t m);

//! @return Parser Error Message
const char * benthos_dc_manifest_errmsg(void);

//! @return Current Driver Information for the Driver Iterator
const driver_info_t * benthos_dc_driver_iterator_info(driver_iterator_t m);

//! @brief Advance the Driver Iterator
int benthos_dc_driver_iterator_next(driver_iterator_t m);

//! @brief Dispose of the Driver Iterator
void benthos_dc_driver_iterator_dispose(driver_iterator_t m);

#ifdef __cplusplus
}
#endif

#endif /* BENTHOS_DC_MANIFEST_H_ */
