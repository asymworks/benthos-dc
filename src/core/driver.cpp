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
#include <stdexcept>

#include <benthos/divecomputer/driver.hpp>

using namespace benthos::dc;

Driver::Driver(const driver_interface_t * driver, const driver_manifest_t * manifest, const std::string & path, const std::string & args)
	: m_driver(driver), m_manifest(manifest), m_device(0), m_parser(0)
{
	int rc = m_driver->driver_create(& m_device);
	if (rc != 0)
		throw std::runtime_error("Failed to create device handle: " + std::string(strerror(errno)));

	rc = m_driver->driver_open(m_device, path.c_str(), args.c_str());
	if (rc != 0)
	{
		if (path.empty())
			throw std::runtime_error("Failed to open '" +
					std::string(m_driver->driver_name(m_device)) + "' device: " +
					std::string(m_driver->driver_errmsg(m_device)));
		else
			throw std::runtime_error("Failed to open '" + path + ": " +
					std::string(m_driver->driver_errmsg(m_device)));
	}

	rc = m_driver->parser_create(& m_parser, m_device);
	if (rc != 0)
		throw std::runtime_error("Failed to create data parser: " + std::string(strerror(errno)));
}

Driver::~Driver()
{
	m_driver->parser_close(m_parser);
	m_driver->driver_close(m_device);
}

std::string Driver::errmsg() const
{
	return std::string(m_driver->driver_errmsg(m_device));
}

std::string Driver::manufacturer(uint8_t model_number)
{
	std::map<int, model_info_t>::const_iterator it = m_manifest->driver_models.find(model_number);
	if (it != m_manifest->driver_models.end())
	{
		return it->second.manuf_name;
	}

	return std::string();
}

std::string Driver::model_name(uint8_t model_number)
{
	std::map<int, model_info_t>::const_iterator it = m_manifest->driver_models.find(model_number);
	if (it != m_manifest->driver_models.end())
	{
		return it->second.model_name;
	}

	return std::string();
}

const std::string & Driver::name() const
{
	return m_manifest->driver_name;
}

const std::string & Driver::plugin() const
{
	return m_manifest->plugin_name;
}

void Driver::parse(std::vector<uint8_t> data, header_callback_fn_t hcb, waypoint_callback_fn_t pcb, void * userdata)
{
	int rc = m_driver->parser_reset(m_parser);
	if (rc != 0)
		throw std::runtime_error("Failed to reset data parser: " + errmsg());

	rc = m_driver->parser_parse_header(m_parser, data.data(), data.size(), hcb, userdata);
	if (rc != 0)
		throw std::runtime_error("Failed to parse dive header: " + errmsg());

	rc = m_driver->parser_parse_profile(m_parser, data.data(), data.size(), pcb, userdata);
	if (rc != 0)
		throw std::runtime_error("Failed to parse dive profile: " + errmsg());
}

void _benthos_dc_extract_dives_cb(void * userdata, void * data, uint32_t length, const char * token)
{
	std::list<dive_entry_t> * dl = (std::list<dive_entry_t> *)(userdata);
	dl->push_back(std::pair<dive_buffer_t, std::string>(dive_buffer_t((uint8_t *)data, (uint8_t *)(data) + length), token));
}

std::list<dive_entry_t> Driver::transfer(device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata) const
{
	std::list<dive_entry_t> result;
	uint32_t tlen = 0;
	void * data = NULL;

	int rc = m_driver->driver_transfer(m_device, & data, & tlen, dcb, pcb, userdata);
	if (rc != 0)
	{
		if (data != NULL)
			free(data);
		throw std::runtime_error("Failed to transfer dive data: " + errmsg());
	}

	rc = m_driver->driver_extract(m_device, data, tlen, & _benthos_dc_extract_dives_cb, & result);
	if (rc != 0)
	{
		free(data);
		throw std::runtime_error("Failed to extract dive data: " + errmsg());
	}

	free(data);
	return result;
}
