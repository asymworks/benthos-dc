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

#include <cstring>
#include <stdexcept>
#include <string>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "manifest_parser.hpp"

using namespace benthos::dc;

void parseDriverName(xmlAttr * attr, driver_manifest_t & m)
{
	m.driver_name = (char *)attr->children->content;
}

void parseDriverDesc(xmlNode * node, driver_manifest_t & m)
{
	if (! m.driver_desc.empty())
		throw std::runtime_error("Duplicate <description> node within driver '" + m.driver_name + "'");

	xmlNode * cur = node->children;
	for ( ; cur ; cur = cur->next)
	{
		if (cur->type == XML_TEXT_NODE)
			m.driver_desc = (char *)cur->content;
		else
			throw std::runtime_error("Nodes are not allowed within <description>");
	}
}

void parseDriverIntf(xmlNode * node, driver_manifest_t & m)
{
	if (! m.driver_intf.empty())
		throw std::runtime_error("Duplicate <interface> node within driver '" + m.driver_name + "'");

	xmlNode * cur = node->children;
	for ( ; cur ; cur = cur->next)
	{
		if (cur->type == XML_TEXT_NODE)
			m.driver_intf = (char *)cur->content;
		else
			throw std::runtime_error("Nodes are not allowed within <interface>");
	}
}

void parseDriverParams(xmlNode * node, driver_manifest_t & m)
{
	xmlNode * cur = node->children;
	for ( ; cur ; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (strcmp((char *)cur->name, "parameter") != 0)
			throw std::runtime_error("Unsupported node <" + std::string((char *)cur->name) + "> within <parameters>");

		param_info_t pi;
		pi.param_type = (param_type_t)-1;

		xmlAttr * attr = cur->properties;
		for ( ; attr ; attr = attr->next)
		{
			if (strcmp((char *)attr->name, "name") == 0)
			{
				if (! pi.param_name.empty())
					throw std::runtime_error("Duplicate 'name' attribute in <parameter>");
				pi.param_name = (char *)attr->children->content;
			}
			else if (strcmp((char *)attr->name, "type") == 0)
			{
				if (pi.param_type != (param_type_t)-1)
					throw std::runtime_error("Duplicate 'type' attribute in <parameter>");
				char * type = (char *)attr->children->content;
				if (strcmp(type, "string") == 0)
					pi.param_type = ptString;
				else if (strcmp(type, "int") == 0)
					pi.param_type = ptInt;
				else if (strcmp(type, "uint") == 0)
					pi.param_type = ptUInt;
				else if (strcmp(type, "float") == 0)
					pi.param_type = ptFloat;
				else if (strcmp(type, "model") == 0)
					pi.param_type = ptModel;
				else
					throw std::runtime_error("Unknown parameter type '" + std::string(type) + "'");
			}
			else if (strcmp((char *)attr->name, "default") == 0)
			{
				if (! pi.param_default.empty())
					throw std::runtime_error("Duplicate 'default' attribute in <parameter>");
				pi.param_default = (char *)attr->children->content;
			}
			else
				throw std::runtime_error("Unsupported Attribute '" + std::string((char *)attr->name) + "' within <parameter>");
		}

		if (pi.param_name.empty())
			throw std::runtime_error("Missing 'name' attribute in <parameter>");
		if (pi.param_type == (param_type_t)-1)
			throw std::runtime_error("Missing 'type' attribute in <parameter>");

		xmlNode * child = cur->children;
		for ( ; child ; child = child->next )
		{
			if (child->type == XML_TEXT_NODE)
				pi.param_desc = (char *)child->content;
			else
				throw std::runtime_error("Nodes are not allowed within <parameter>");
		}

		m.driver_params.push_back(pi);
	}
}

void parseDriverModels(xmlNode * node, driver_manifest_t & m)
{
	std::string mfg;

	xmlAttr * attr = node->properties;
	for ( ; attr ; attr = attr->next)
	{
		if (strcmp((char *)attr->name, "manufacturer") == 0)
			mfg = (char *)attr->children->content;
		else
			throw std::runtime_error("Unsupported Attribute '" + std::string((char *)attr->name) + "' within <models>");
	}

	int rc;
	xmlNode * cur = node->children;
	for ( ; cur ; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (strcmp((char *)cur->name, "model") != 0)
			throw std::runtime_error("Unsupported node <" + std::string((char *)cur->name) + "> within <models>");

		int id = -1;
		model_info_t mi;

		attr = cur->properties;
		for ( ; attr ; attr = attr->next)
		{
			if (strcmp((char *)attr->name, "id") == 0)
			{
				if (id != -1)
					throw std::runtime_error("Duplicate 'id' attribute in <model>");
				rc = sscanf((char *)attr->children->content, "%u", & id);
				if ((rc != 1) || (id < 0))
					throw std::runtime_error("Invalid model number '" + std::string((char *)attr->children->content) + "' within <model>");
			}
			else if (strcmp((char *)attr->name, "manufacturer") == 0)
			{
				if (! mi.manuf_name.empty())
					throw std::runtime_error("Duplicate 'manufacturer' attribute in <model>");
				mi.manuf_name = (char *)attr->children->content;
			}
			else
				throw std::runtime_error("Unsupported Attribute '" + std::string((char *)attr->name) + "' within <parameter>");
		}

		if (id == -1)
			throw std::runtime_error("Missing 'id' attribute in <model>");
		if (mi.manuf_name.empty())
			mi.manuf_name = mfg;

		xmlNode * child = cur->children;
		for ( ; child ; child = child->next )
		{
			if (child->type == XML_TEXT_NODE)
				mi.model_name = (char *)child->content;
			else
				throw std::runtime_error("Nodes are not allowed within <model>");
		}

		m.driver_models.insert(std::pair<int, model_info_t>(id, mi));
	}
}

void parseDriver(xmlNode * driver, plugin_manifest_t & m)
{
	if (driver->type != XML_ELEMENT_NODE)
		throw std::runtime_error("Driver node must be an element");

	if (strcmp((const char *)driver->name, "driver") != 0)
		throw std::runtime_error("Driver node must be <driver>");

	driver_manifest_t dm;

	xmlAttr * attr = driver->properties;
	for ( ; attr ; attr = attr->next)
	{
		if (strcmp((char *)attr->name, "name") == 0)
			parseDriverName(attr, dm);
		else
			throw std::runtime_error("Unsupported Attribute '" + std::string((char *)attr->name) + "' within <driver>");
	}

	xmlNode * cur = driver->children;
	for ( ; cur ; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (strcmp((char *)cur->name, "description") == 0)
			parseDriverDesc(cur, dm);
		else if (strcmp((char *)cur->name, "interface") == 0)
			parseDriverIntf(cur, dm);
		else if (strcmp((char *)cur->name, "parameters") == 0)
			parseDriverParams(cur, dm);
		else if (strcmp((char *)cur->name, "models") == 0)
			parseDriverModels(cur, dm);
		else
			throw std::runtime_error("Unsupported node <" + std::string((char *)cur->name) + "> within <driver>");
	}

	dm.plugin_name = m.plugin_name;
	m.plugin_drivers.push_back(dm);
}

void parsePluginName(xmlAttr * attr, plugin_manifest_t & m)
{
	m.plugin_name = (char *)attr->children->content;
}

void parsePluginLibrary(xmlAttr * attr, plugin_manifest_t & m)
{
	m.plugin_library = (char *)attr->children->content;
}

void parsePluginVersion(xmlAttr * attr, plugin_manifest_t & m)
{
	char * data = (char *)attr->children->content;
	int rc = sscanf(data, "%u.%u.%u",
		& m.plugin_major_version,
		& m.plugin_minor_version,
		& m.plugin_patch_version);

	if (rc <= 0)
		throw std::runtime_error("Invalid module version '" + std::string(data) + "'");

	if (rc < 2)
		m.plugin_patch_version = 0;
	if (rc < 3)
		m.plugin_minor_version = 0;
}

void doParse(xmlNode * root, plugin_manifest_t & m)
{
	if (root->type != XML_ELEMENT_NODE)
		throw std::runtime_error("Root node must be an element");

	if (strcmp((const char *)root->name, "plugin") != 0)
		throw std::runtime_error("Root node must be <plugin>");

	xmlAttr * attr = root->properties;
	for ( ; attr ; attr = attr->next)
	{
		if (strcmp((char *)attr->name, "name") == 0)
			parsePluginName(attr, m);
		else if (strcmp((char *)attr->name, "library") == 0)
			parsePluginLibrary(attr, m);
		else if (strcmp((char *)attr->name, "version") == 0)
			parsePluginVersion(attr, m);
		else
			throw std::runtime_error("Unsupported Attribute '" + std::string((char *)attr->name) + "'");
	}

	if (m.plugin_name.empty())
		throw std::runtime_error("Plugin Name was not defined");
	if (m.plugin_library.empty())
		throw std::runtime_error("Plugin Library was not defined");

	xmlNode * cur = root->children;
	for ( ; cur ; cur = cur->next)
	{
		if (cur->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char *)cur->name, "driver") == 0)
				parseDriver(cur, m);
			else
				throw std::runtime_error("Unsupported node <" + std::string((char *)cur->name) + "> within <plugin>");
		}
	}
}

plugin_manifest_t parseManifest(const std::string & manifest)
{
	plugin_manifest_t result;

	xmlDoc * doc;
	xmlNode * root;

	// Load the XML File
	doc = xmlReadFile(manifest.c_str(), NULL, 0);
	if (doc == NULL)
		throw std::runtime_error("Failed to parse manifest file");

	try
	{
		// Parse the Root Node
		root = xmlDocGetRootElement(doc);
		if (root == NULL)
			throw std::runtime_error("Manifest has no root node");

		doParse(root, result);
	}
	catch (std::exception & e)
	{
		xmlFreeDoc(doc);
		xmlCleanupParser();
		throw;
	}

	// Free Resources
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return result;
}
