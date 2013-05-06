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

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "manifest_parser.hpp"

using namespace benthos::dc;

static const setting_typespec_t _bdc_bi_typespec_int =
{
	NULL,
	"int",
	stInt,
	etRange,
	INT32_MIN, INT32_MAX, 1,
	std::list<std::pair<int64_t, std::string> >(),
	std::map<std::string, setting_typespec_ptr>(),
	10, 0
};

static const setting_typespec_t _bdc_bi_typespec_uint =
{
	NULL,
	"uint",
	stInt,
	etRange,
	0, UINT32_MAX, 1,
	std::list<std::pair<int64_t, std::string> >(),
	std::map<std::string, setting_typespec_ptr>(),
	10, 0
};

static const setting_typespec_t _bdc_bi_typespec_bool =
{
	NULL,
	"bool",
	stBool,
	etNone,
	0, 0, 0,
	std::list<std::pair<int64_t, std::string> >(),
	std::map<std::string, setting_typespec_ptr>(),
	10, 0
};

static const setting_typespec_t _bdc_bi_typespec_datetime =
{
	NULL,
	"datetime",
	stDateTime,
	etNone,
	0, 0, 0,
	std::list<std::pair<int64_t, std::string> >(),
	std::map<std::string, setting_typespec_ptr>(),
	10, 0
};

static const setting_typespec_t _bdc_bi_typespec_string =
{
	NULL,
	"string",
	stBool,
	etNone,
	0, 0, 0,
	std::list<std::pair<int64_t, std::string> >(),
	std::map<std::string, setting_typespec_ptr>(),
	10, 0
};

void _bdc_static_deleter(const struct setting_typespec_t_ *)
{
}

setting_typespec_ptr manifest_lookup_type(const driver_manifest_t & dm, const std::string & name)
{
	std::list<setting_typespec_ptr>::const_iterator it;
	for (it = dm.global_types.begin(); it != dm.global_types.end(); it++)
		if ((* it)->name == name)
			return (* it);

	return setting_typespec_ptr();
}

setting_info_ptr manifest_lookup_setting(const driver_manifest_t & dm, const std::string & name, int model)
{
	setting_info_ptr ret;
	std::list<setting_info_ptr>::const_iterator it;
	for (it = dm.global_settings.begin(); it != dm.global_settings.end(); it++)
		if ((* it)->setting_name == name)
			ret = * it;

	std::map<int, model_info_t>::const_iterator mit = dm.driver_models.find(model);
	if (mit != dm.driver_models.end())
	{
		const model_info_t & mdl = mit->second;
		for (it = mdl.settings.begin(); it != mdl.settings.end(); it++)
			if ((* it)->setting_name == name)
				ret = * it;
	}

	return ret;
}

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

		if (pi.param_type == ptModel)
		{
			if (! m.model_param.empty())
				throw std::runtime_error("Only one parameter of type 'model' is allowed");
			else
				m.model_param = pi.param_name;
		}

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

void parseDriverTypes(xmlNode * node, driver_manifest_t & m)
{
	xmlNode * cur = node->children;
	for ( ; cur ; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (strcmp((char *)cur->name, "typedef") != 0)
			throw std::runtime_error("Unsupported node <" + std::string((char *)cur->name) + "> within <types>");
	}
}

void parseDriverSettings(xmlNode * node, driver_manifest_t & m)
{
	xmlNode * cur = node->children;
	for ( ; cur ; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (strcmp((char *)cur->name, "types") == 0)
			parseDriverTypes(cur, m);
		else if (strcmp((char *)cur->name, "table") != 0)
			throw std::runtime_error("Unsupported node <" + std::string((char *)cur->name) + "> within <settings>");

		int rc;
		int model_id = -1;
		xmlAttr * attr = cur->properties;
		for ( ; attr ; attr = attr->next)
		{
			if (strcmp((char *)attr->name, "model") == 0)
			{
				if (model_id != -1)
					throw std::runtime_error("Duplicate 'model' attribute in <table>");
				rc = sscanf((char *)attr->children->content, "%u", & model_id);
				if ((rc != 1) || (m.driver_models.find(model_id) == m.driver_models.end()))
					throw std::runtime_error("Invalid model number '" + std::string((char *)attr->children->content) + "' within <table>");
			}
			else
				throw std::runtime_error("Unsupported Attribute '" + std::string((char *)attr->name) + "' within <table>");
		}

		xmlNode * child = cur->children;
		for ( ; child ; child = child->next )
		{
			if (child->type != XML_ELEMENT_NODE)
				continue;

			if (strcmp((char *)child->name, "setting") != 0)
				throw std::runtime_error("Unsupported node <" + std::string((char *)child->name) + "> within <table>");

			std::string g_name;
			std::string s_name;
			std::string s_desc;
			std::string s_type;
			std::string s_flags;
			std::string d_name;

			for (attr = child->properties ; attr ; attr = attr->next)
			{
				if (strcmp((char *)attr->name, "group") == 0)
					g_name = (char *)attr->children->content;
				else if (strcmp((char *)attr->name, "name") == 0)
					s_name = (char *)attr->children->content;
				else if (strcmp((char *)attr->name, "type") == 0)
					s_type = (char *)attr->children->content;
				else if (strcmp((char *)attr->name, "display") == 0)
					d_name = (char *)attr->children->content;
				else if (strcmp((char *)attr->name, "flags") == 0)
					s_flags = (char *)attr->children->content;
				else
					throw std::runtime_error("Unsupported Attribute '" + std::string((char *)attr->name) + "' within <setting>");
			}

			xmlNode * schild = child->children;
			for ( ; schild ; schild = schild->next)
				if (schild->type == XML_TEXT_NODE)
					s_desc = (char *)schild->content;
				else
					throw std::runtime_error("Nodes are not allowed within <setting>");

			if (s_name.empty())
				throw std::runtime_error("Missing 'name' attribute in <setting>");

			setting_info_ptr p1 = manifest_lookup_setting(m, s_name, -1);
			setting_info_ptr p2 = manifest_lookup_setting(m, s_name, model_id);
			if (p1 && p2 && (p1 != p2))
				throw std::runtime_error("Duplicate setting '" + s_name + "' within <table>");

			if (s_type.empty())
				throw std::runtime_error("Missing 'type' attribute for setting '" + s_name + "'");

			setting_typespec_ptr type_ = manifest_lookup_type(m, s_type);
			if (! type_)
				throw std::runtime_error("Undefined type '" + s_type + "' for setting '" + s_name + "'");

			setting_info_mptr si(new setting_info_t);
			si->group_name = g_name;
			si->setting_name = s_name;
			si->setting_desc = s_desc;
			si->setting_dispname = d_name;
			si->setting_type = type_;
			si->setting_readonly = false;
			si->setting_writeonly = false;

			if (! s_flags.empty())
			{
				boost::char_separator<char> sep(", ");
				boost::tokenizer<boost::char_separator<char> > tokens(s_flags, sep);
				BOOST_FOREACH (const std::string & t, tokens)
				{
					if (t == "readonly")
						si->setting_readonly = true;
					else if (t == "writeonly")
						si->setting_writeonly = true;
					else
						throw std::runtime_error("Unsupported flag '" + t + "' within setting '" + s_name + "'");
				}

				if (si->setting_readonly && si->setting_writeonly)
					throw std::runtime_error("Setting '" + s_name + "' may not specify both read only and write only");
			}

			if (model_id == -1)
				m.global_settings.push_back(si);
			else
				m.driver_models[model_id].settings.push_back(si);
		}
	}
}

void parseDriver(xmlNode * driver, plugin_manifest_t & m)
{
	if (driver->type != XML_ELEMENT_NODE)
		throw std::runtime_error("Driver node must be an element");

	if (strcmp((const char *)driver->name, "driver") != 0)
		throw std::runtime_error("Driver node must be <driver>");

	driver_manifest_t dm;

	dm.global_types.push_back(setting_typespec_ptr(& _bdc_bi_typespec_int,		_bdc_static_deleter));
	dm.global_types.push_back(setting_typespec_ptr(& _bdc_bi_typespec_uint,		_bdc_static_deleter));
	dm.global_types.push_back(setting_typespec_ptr(& _bdc_bi_typespec_bool,		_bdc_static_deleter));
	dm.global_types.push_back(setting_typespec_ptr(& _bdc_bi_typespec_datetime,	_bdc_static_deleter));
	dm.global_types.push_back(setting_typespec_ptr(& _bdc_bi_typespec_string,	_bdc_static_deleter));

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
		else if (strcmp((char *)cur->name, "settings") == 0)
			parseDriverSettings(cur, dm);
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
