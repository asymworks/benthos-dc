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
#include <cstring>
#include <unistd.h>

#include <sys/stat.h>

#include <list>
#include <string>
#include <vector>

#include <iconv.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <benthos/divecomputer/manifest.h>

#include "iterators.hpp"
#include "wrappers.hpp"

static std::string 					g_parser_errmsg;

struct plugin_manifest_t_
{
	std::string						plugin_name;
	std::string						plugin_library;

	plugin_info_t					plugin_info;

	std::list<driver_wrapper_t>		driver_wrappers;
};

#define CONV_MAXLEN		2048

std::string conv_xmlstr(const xmlChar * s, iconv_t conv)
{
#ifdef ICONV_SECOND_ARGUMENT_IS_CONST
	const char * inptr = (const char *)(s);
#else
	char * inptr = (char *)(s);
#endif

	char outbuf[CONV_MAXLEN];
	char * outptr = outbuf;
	size_t insize = strlen(inptr);
	size_t outsize = CONV_MAXLEN-1;
	size_t rv;

	memset(outbuf, 0, CONV_MAXLEN);

	rv = iconv(conv, & inptr, & insize, & outptr, & outsize);

	if (rv != (size_t)(-1))
		return std::string(outbuf);

	return std::string("<invalid utf-8 string>");
}

struct manifest_drvit_data
{
	std::list<driver_wrapper_t> *					list;
	std::list<driver_wrapper_t>::const_iterator 	it;
};

const driver_info_t * manifest_drvit_info(void * arg)
{
	struct manifest_drvit_data * data = static_cast<struct manifest_drvit_data *>(arg);
	if (! data || (data->it == data->list->end()))
		return 0;

	return & data->it->driver_info;
}

int manifest_drvit_next(void * arg)
{
	struct manifest_drvit_data * data = static_cast<struct manifest_drvit_data *>(arg);
	if (! data)
		return 0;

	data->it++;
	if (data->it == data->list->end())
		return 0;

	return 1;
}

void manifest_drvit_dispose(void * arg)
{
	struct manifest_drvit_data * data = static_cast<struct manifest_drvit_data *>(arg);
	if (! data)
		return ;

	free(data);
}

static struct driver_iterator_fn_table manifest_drvit_impl =
{
	manifest_drvit_info,
	manifest_drvit_next,
	manifest_drvit_dispose
};

int driver_has_model(const driver_wrapper_t * m, int model_id, const std::string & model_name)
{
	std::list<model_wrapper_t>::const_iterator it;
	for (it = m->model_wrappers.begin(); it != m->model_wrappers.end(); it++)
	{
		if (it->model_info.model_number == model_id)
			return 1;
		if (strcasecmp(it->model_name.c_str(), model_name.c_str()) == 0)
			return 1;
	}

	return 0;
}

int driver_has_param(const driver_wrapper_t * m, const std::string & param_name)
{
	std::list<param_wrapper_t>::const_iterator it;
	for (it = m->param_wrappers.begin(); it != m->param_wrappers.end(); it++)
	{
		if (strcasecmp(it->param_name.c_str(), param_name.c_str()) == 0)
			return 1;
	}

	return 0;
}

int plugin_has_driver(struct plugin_manifest_t_ * p, const std::string & driver_name)
{
	std::list<driver_wrapper_t>::const_iterator it;
	for (it = p->driver_wrappers.begin(); it != p->driver_wrappers.end(); it++)
	{
		if (strcasecmp(it->driver_name.c_str(), driver_name.c_str()) == 0)
			return 1;
	}

	return 0;
}

int parse_driver_desc(driver_wrapper_t * d, xmlNode * n, iconv_t conv)
{
	if (! d->driver_desc.empty())
	{
		g_parser_errmsg = "Duplicate <description> element within <driver>";
		return MANIFEST_ERR_PARSER;
	}

	if (! n->children)
	{
		g_parser_errmsg = "Empty <description> element within <driver>";
		return MANIFEST_ERR_PARSER;
	}

	if ((n->children->type != XML_TEXT_NODE) || (n->children->next))
	{
		g_parser_errmsg = "Elements not allowed within <description> element";
		return MANIFEST_ERR_PARSER;
	}

	d->driver_desc = conv_xmlstr(n->children->content, conv);

	return 0;
}

int parse_driver_intf(driver_wrapper_t * d, xmlNode * n, iconv_t conv)
{
	std::string intf;

	if (d->driver_info.driver_intf != diUnknown)
	{
		g_parser_errmsg = "Duplicate <interface> element within <driver>";
		return MANIFEST_ERR_PARSER;
	}

	if (! n->children)
	{
		g_parser_errmsg = "Empty <interface> element within <driver>";
		return MANIFEST_ERR_PARSER;
	}

	if ((n->children->type != XML_TEXT_NODE) || (n->children->next))
	{
		g_parser_errmsg = "Elements not allowed within <interface> element";
		return MANIFEST_ERR_PARSER;
	}

	if (xmlStrcmp(n->children->content, BAD_CAST("serial")) == 0)
		d->driver_info.driver_intf = diSerial;
	else if (xmlStrcmp(n->children->content, BAD_CAST("usb")) == 0)
		d->driver_info.driver_intf = diUSB;
	else if (xmlStrcmp(n->children->content, BAD_CAST("bluetooth")) == 0)
		d->driver_info.driver_intf = diBluetooth;
	else if (xmlStrcmp(n->children->content, BAD_CAST("irda")) == 0)
		d->driver_info.driver_intf = diIrDA;
	else if (xmlStrcmp(n->children->content, BAD_CAST("net")) == 0)
		d->driver_info.driver_intf = diNetwork;
	else
	{
		g_parser_errmsg = "Invalid interface type: '" + conv_xmlstr(n->children->content, conv) + "' within <driver>";
		return MANIFEST_ERR_PARSER;
	}

	return 0;
}

int parse_driver_params(driver_wrapper_t * d, xmlNode * n, iconv_t conv)
{
	xmlAttr * attr;
	xmlNode * cur;

	for (cur = n->children ; cur ; cur = cur->next)
	{
		param_wrapper_t p;

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcmp(cur->name, BAD_CAST("parameter")) != 0)
		{
			g_parser_errmsg = "Invalid element <" + conv_xmlstr(cur->name, conv) + "> within <parameters>";
			return MANIFEST_ERR_PARSER;
		}

		/* Initialize Parameter */
		p.param_info.param_type = ptUnknown;

		/* Parse Attributes */
		for (attr = cur->properties ; attr ; attr = attr->next)
		{
			if (xmlStrcmp(attr->name, BAD_CAST("name")) == 0)
			{
				if (! p.param_name.empty())
				{
					g_parser_errmsg = "Duplicate 'name' attribute within <parameter>";
					return MANIFEST_ERR_PARSER;
				}

				p.param_name = conv_xmlstr(attr->children->content, conv);
			}
			else if (xmlStrcmp(attr->name, BAD_CAST("type")) == 0)
			{
				if (p.param_info.param_type != ptUnknown)
				{
					g_parser_errmsg = "Duplicate 'type' attribute within <parameter>";
					return MANIFEST_ERR_PARSER;
				}

				if (xmlStrcmp(attr->children->content, BAD_CAST("string")) == 0)
					p.param_info.param_type = ptString;
				else if (xmlStrcmp(attr->children->content, BAD_CAST("int")) == 0)
					p.param_info.param_type = ptInt;
				else if (xmlStrcmp(attr->children->content, BAD_CAST("uint")) == 0)
					p.param_info.param_type = ptUInt;
				else if (xmlStrcmp(attr->children->content, BAD_CAST("float")) == 0)
					p.param_info.param_type = ptFloat;
				else if (xmlStrcmp(attr->children->content, BAD_CAST("model")) == 0)
					p.param_info.param_type = ptModel;
				else
				{
					g_parser_errmsg = "Invalid type name: '" + conv_xmlstr(attr->children->content, conv) + "' within <parameter>";
					return MANIFEST_ERR_PARSER;
				}
			}
			else if (xmlStrcmp(attr->name, BAD_CAST("default")) == 0)
			{
				if (! p.param_default.empty())
				{
					g_parser_errmsg = "Duplicate 'default' attribute within <parameter>";
					return MANIFEST_ERR_PARSER;
				}

				p.param_default = conv_xmlstr(attr->children->content, conv);
			}
			else
			{
				g_parser_errmsg = "Unsupported Attribute: '" + conv_xmlstr(attr->name, conv) + "' within <parameter>";
				return MANIFEST_ERR_PARSER;
			}
		}

		/* Check Name and Type */
		if (p.param_name.empty())
		{
			g_parser_errmsg = "Missing 'name' attribute within <parameter>";
			return MANIFEST_ERR_PARSER;
		}

		if (p.param_info.param_type == ptUnknown)
		{
			g_parser_errmsg = "Missing 'type' attribute within <parameter>";
			return MANIFEST_ERR_PARSER;
		}

		if (driver_has_param(d, p.param_name))
		{
			g_parser_errmsg = "Duplicate parameter name '" + p.param_name + "'";
			return MANIFEST_ERR_PARSER;
		}

		/* Assign Model Parameter */
		if (p.param_info.param_type == ptModel)
		{
			if (! d->model_param.empty())
			{
				g_parser_errmsg = "Duplicate parameter of type 'model'";
				return MANIFEST_ERR_PARSER;
			}

			d->model_param = p.param_name;
		}

		/* Parse Description */
		if (cur->children)
		{
			if ((cur->children->type != XML_TEXT_NODE) || (cur->children->next))
			{
				g_parser_errmsg = "Elements not allowed within <parameter> element";
				return MANIFEST_ERR_PARSER;
			}

			p.param_desc = conv_xmlstr(cur->children->content, conv);
		}

		/* Append Parameter */
		d->param_wrappers.push_back(p);
	}

	return 0;
}

int parse_driver_models(driver_wrapper_t * d, xmlNode * n, iconv_t conv)
{
	std::string mfg;
	xmlAttr * attr;
	xmlNode * cur;

	/* Parse Default Manufacturer */
	if (n->properties)
	{
		if (xmlStrcmp(n->properties->name, BAD_CAST("manufacturer")) != 0)
		{
			g_parser_errmsg = "Unsupported Attribute: '" + conv_xmlstr(n->properties->name, conv) + "' within <models>";
			return MANIFEST_ERR_PARSER;
		}

		if (n->properties->next)
		{
			g_parser_errmsg = "Unsupported Attribute: '" + conv_xmlstr(n->properties->next->name, conv) + "' within <models>";
			return MANIFEST_ERR_PARSER;
		}

		mfg = conv_xmlstr(n->properties->children->content, conv);
	}

	/* Parse Model Entries */
	for (cur = n->children ; cur ; cur = cur->next)
	{
		model_wrapper_t m;

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcmp(cur->name, BAD_CAST("model")) != 0)
		{
			g_parser_errmsg = "Invalid element <" + conv_xmlstr(cur->name, conv) + "> within <models>";
			return MANIFEST_ERR_PARSER;
		}

		/* Initialize Model */
		m.model_info.model_number = -1;

		/* Parse Attributes */
		for (attr = cur->properties ; attr ; attr = attr->next)
		{
			if (xmlStrcmp(attr->name, BAD_CAST("id")) == 0)
			{
				std::string content = conv_xmlstr(attr->children->content, conv);

				if (m.model_info.model_number != -1)
				{
					g_parser_errmsg = "Duplicate 'id' attribute in <model>";
					return MANIFEST_ERR_PARSER;
				}

				if (sscanf(content.c_str(), "%u", & m.model_info.model_number) != 1)
				{
					g_parser_errmsg = "Invalid model number '" + content + "' within <model>";
					return MANIFEST_ERR_PARSER;
				}
			}
			else if (xmlStrcmp(attr->name, BAD_CAST("manufacturer")) == 0)
			{
				if (! m.model_manuf.empty())
				{
					g_parser_errmsg = "Duplicate 'manufactuer' attribute in <model>";
					return MANIFEST_ERR_PARSER;
				}

				m.model_manuf = conv_xmlstr(attr->children->content, conv);
			}
			else
			{
				g_parser_errmsg = "Unsupported Attribute: '" + conv_xmlstr(attr->name, conv) + "' within <model>";
				return MANIFEST_ERR_PARSER;
			}
		}

		/* Check Model Id and Manufacturer */
		if (m.model_info.model_number == -1)
		{
			g_parser_errmsg = "Missing 'id' attribute in <model>";
			return MANIFEST_ERR_PARSER;
		}

		if (m.model_manuf.empty())
			m.model_manuf = mfg;

		/* Parse Model Name */
		if (! cur->children)
		{
			g_parser_errmsg = "Empty <model> element";
			return MANIFEST_ERR_PARSER;
		}

		if ((cur->children->type != XML_TEXT_NODE) || (cur->children->next))
		{
			g_parser_errmsg = "Elements not allowed within <model> element";
			return MANIFEST_ERR_PARSER;
		}

		m.model_name = conv_xmlstr(cur->children->content, conv);

		if (driver_has_model(d, m.model_info.model_number, m.model_name))
		{
			g_parser_errmsg = "Duplicate model number or name '" + m.model_name + "'";
			return MANIFEST_ERR_PARSER;
		}

		/* Append Model */
		d->model_wrappers.push_back(m);
	}

	return 0;
}

void setup_pointers(plugin_manifest_t m)
{
	std::list<driver_wrapper_t>::iterator dit;

	/* Setup Pointers for Manifest */
	m->plugin_info.plugin_name = m->plugin_name.c_str();
	m->plugin_info.plugin_library = m->plugin_library.c_str();

	/* Iterate through Drivers */
	for (dit = m->driver_wrappers.begin(); dit != m->driver_wrappers.end(); dit++)
	{
		std::list<param_wrapper_t>::iterator pit;
		std::list<model_wrapper_t>::iterator mit;

		/* Setup Pointer to Plugin */
		dit->driver_info.plugin = & m->plugin_info;

		/* Setup Pointers for Driver Information */
		dit->driver_info.driver_name = dit->driver_name.c_str();
		dit->driver_info.driver_desc = dit->driver_desc.c_str();
		dit->driver_info.model_param = dit->model_param.c_str();

		/* Clear Vectors */
		dit->params.clear();
		dit->models.clear();

		dit->driver_info.params = 0;
		dit->driver_info.models = 0;

		dit->driver_info.n_params = 0;
		dit->driver_info.n_models = 0;

		/* Iterate through Parameters */
		for (pit = dit->param_wrappers.begin(); pit != dit->param_wrappers.end(); pit++)
		{
			pit->param_info.param_name = pit->param_name.c_str();
			pit->param_info.param_desc = pit->param_desc.c_str();
			pit->param_info.param_default = pit->param_default.c_str();

			dit->params.push_back(& pit->param_info);
		}

		/* Iterate through Models */
		for (mit = dit->model_wrappers.begin(); mit != dit->model_wrappers.end(); mit++)
		{
			mit->model_info.model_name = mit->model_name.c_str();
			mit->model_info.manuf_name = mit->model_manuf.c_str();

			dit->models.push_back(& mit->model_info);
		}

		/* Setup Array Pointers */
		if (dit->params.size() > 0)
		{
			dit->driver_info.params = & dit->params[0];
			dit->driver_info.n_params = dit->params.size();
		}

		if (dit->models.size() > 0)
		{
			dit->driver_info.models = & dit->models[0];
			dit->driver_info.n_models = dit->models.size();
		}
	}
}

int parse_driver(plugin_manifest_t m, xmlNode * n, iconv_t conv)
{
	driver_wrapper_t d;

	int rv;
	xmlAttr * attr;
	xmlNode * cur;

	/* Initialize Driver Wrapper */
	d.driver_info.plugin = & m->plugin_info;
	d.driver_info.driver_intf = diUnknown;
	d.driver_info.n_models = 0;
	d.driver_info.n_params = 0;
	d.driver_info.models = 0;
	d.driver_info.params = 0;

	/* Parse Driver Name */
	for (attr = n->properties ; attr ; attr = attr->next)
	{
		if (xmlStrcmp(attr->name, BAD_CAST("name")) == 0)
		{
			d.driver_name = conv_xmlstr(attr->children->content, conv);
		}
		else
		{
			g_parser_errmsg = "Unsupported Attribute: '" + conv_xmlstr(attr->name, conv) + "' within <driver>";
			return MANIFEST_ERR_PARSER;
		}
	}

	/* Parse Driver Information */
	for (cur = n->children ; cur ; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcmp(cur->name, BAD_CAST("description")) == 0)
		{
			rv = parse_driver_desc(& d, cur, conv);
			if (rv != 0)
				return rv;
		}
		else if (xmlStrcmp(cur->name, BAD_CAST("interface")) == 0)
		{
			rv = parse_driver_intf(& d, cur, conv);
			if (rv != 0)
				return rv;
		}
		else if (xmlStrcmp(cur->name, BAD_CAST("parameters")) == 0)
		{
			rv = parse_driver_params(& d, cur, conv);
			if (rv != 0)
				return rv;
		}
		else if (xmlStrcmp(cur->name, BAD_CAST("models")) == 0)
		{
			rv = parse_driver_models(& d, cur, conv);
			if (rv != 0)
				return rv;
		}
		else
		{
			g_parser_errmsg = "Unsupported node <" + conv_xmlstr(cur->name, conv) + "> within <plugin>";
			return MANIFEST_ERR_PARSER;
		}
	}

	/* Check Driver Name */
	if (plugin_has_driver(m, d.driver_name))
	{
		g_parser_errmsg = "Duplicate driver name '" + d.driver_name + "' in plugin '" + m->plugin_name + "'";
		return MANIFEST_ERR_PARSER;
	}

	/* Add Driver Wrapper to Plugin */
	m->driver_wrappers.push_back(d);

	/* Success */
	return 0;
}

int parse_manifest(plugin_manifest_t m, xmlNode * root, iconv_t conv)
{
	int rv;
	xmlAttr * attr;
	xmlNode * cur;

	/* Initialize Version Information */
	m->plugin_info.plugin_major_version = 0;
	m->plugin_info.plugin_minor_version = 0;
	m->plugin_info.plugin_patch_version = 0;

	/* Parse Attributes */
	for (attr = root->properties ; attr ; attr = attr->next)
	{
		if (xmlStrcmp(attr->name, BAD_CAST("name")) == 0)
		{
			m->plugin_name = conv_xmlstr(attr->children->content, conv);
		}
		else if (xmlStrcmp(attr->name, BAD_CAST("library")) == 0)
		{
			m->plugin_library = conv_xmlstr(attr->children->content, conv);
		}
		else if (xmlStrcmp(attr->name, BAD_CAST("version")) == 0)
		{
			std::string verstr = conv_xmlstr(attr->children->content, conv);

			rv = sscanf(verstr.c_str(), "%hhu.%hhu.%hhu",
				& m->plugin_info.plugin_major_version,
				& m->plugin_info.plugin_minor_version,
				& m->plugin_info.plugin_patch_version);

			if (rv < 0)
			{
				g_parser_errmsg = "Invalid version string: '" + verstr + "'";
				return MANIFEST_ERR_PARSER;
			}
		}
		else
		{
			g_parser_errmsg = "Unsupported Attribute: '" + conv_xmlstr(attr->name, conv) + "' within <plugin>";
			return MANIFEST_ERR_PARSER;
		}
	}

	/* Check Name and Library */
	if (m->plugin_name.empty())
		return MANIFEST_ERR_NONAME;
	if (m->plugin_library.empty())
		return MANIFEST_ERR_NOLIB;

	/* Parse Drivers */
	for (cur = root->children ; cur ; cur = cur->next)
	{
		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcmp(cur->name, BAD_CAST("driver")) == 0)
		{
			rv = parse_driver(m, cur, conv);
			if (rv != 0)
				return rv;
		}
		else
		{
			g_parser_errmsg = "Unsupported node <" + conv_xmlstr(cur->name, conv) + "> within <plugin>";
			return MANIFEST_ERR_PARSER;
		}
	}

	/* Setup String Pointers */
	setup_pointers(m);

	/* Success */
	return 0;
}

int benthos_dc_manifest_parse(plugin_manifest_t * ptr, const char * path)
{
	int rv;
	struct plugin_manifest_t_ * manifest;
	struct stat finfo;
	xmlDoc * doc;
	xmlNode * root;
	iconv_t conv;

	if (! ptr || ! path)
		return EINVAL;

	/* Create the Encoding Converter */
	conv = iconv_open("ISO-8859-1", "UTF-8");
	if (conv == (iconv_t)(-1))
		return errno;

	/* Create the Manifest Object */
	manifest = new struct plugin_manifest_t_;
	if (! manifest)
	{
		iconv_close(conv);
		return ENOMEM;
	}

	/* Check that the File Exists */
	rv = stat(path, & finfo);
	if (rv != 0)
	{
		delete manifest;
		iconv_close(conv);
		return errno;
	}

	if (! S_ISREG(finfo.st_mode))
	{
		delete manifest;
		iconv_close(conv);
		return ENOENT;
	}

	/* Load the Manifest File */
	doc = xmlReadFile(path, 0, 0);
	if (doc == NULL)
	{
		delete manifest;
		xmlCleanupParser();
		iconv_close(conv);
		return MANFIEST_ERR_MALFORMED;
	}

	/* Load the Root Node */
	root = xmlDocGetRootElement(doc);
	if ((root == NULL) || (root->type != XML_ELEMENT_NODE))
	{
		delete manifest;
		xmlFreeDoc(doc);
		xmlCleanupParser();
		iconv_close(conv);
		return MANIFEST_ERR_NOROOT;
	}

	/* Check the Root Node */
	if (xmlStrcmp(root->name, BAD_CAST("plugin")) != 0)
	{
		delete manifest;
		xmlFreeDoc(doc);
		xmlCleanupParser();
		iconv_close(conv);
		return MANIFEST_ERR_NOTPLUGIN;
	}

	/* Parse the Manifest Entry */
	rv = parse_manifest(manifest, root, conv);
	if (rv != 0)
	{
		delete manifest;
		xmlFreeDoc(doc);
		xmlCleanupParser();
		iconv_close(conv);
		return rv;
	}

	/* Set Manifest Pointer */
	(* ptr) = manifest;

	/* Cleanup */
	xmlFreeDoc(doc);
	xmlCleanupParser();
	iconv_close(conv);

	return 0;
}

void benthos_dc_manifest_dispose(plugin_manifest_t m)
{
	if (! m)
		return;

	delete m;
}

driver_iterator_t benthos_dc_manifest_drivers(plugin_manifest_t m)
{
	driver_iterator_t it;

	if (! m)
		return 0;

	it = (driver_iterator_t)malloc(sizeof(struct driver_iterator_t_));
	if (! it)
		return 0;

	struct manifest_drvit_data * data = (struct manifest_drvit_data *)malloc(sizeof(struct manifest_drvit_data));
	if (! data)
		return 0;

	data->list = & m->driver_wrappers;
	data->it = data->list->begin();

	it->data = data;
	it->impl = & manifest_drvit_impl;

	return it;
}

const char * benthos_dc_manifest_errmsg(void)
{
	return g_parser_errmsg.c_str();
}

const plugin_info_t * benthos_dc_manifest_plugin(plugin_manifest_t m)
{
	if (! m)
		return 0;

	return & m->plugin_info;
}

const driver_info_t * benthos_dc_driver_iterator_info(driver_iterator_t it)
{
	if (! it)
		return 0;

	return it->impl->info_fn(it->data);
}

int benthos_dc_driver_iterator_next(driver_iterator_t it)
{
	if (! it)
		return 0;

	return it->impl->next_fn(it->data);
}

void benthos_dc_driver_iterator_dispose(driver_iterator_t it)
{
	if (! it)
		return;

	return it->impl->dispose_fn(it->data);
}
