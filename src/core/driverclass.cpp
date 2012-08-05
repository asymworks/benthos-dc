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

#include <benthos/divecomputer/driver.hpp>
#include <benthos/divecomputer/driverclass.hpp>
#include <benthos/divecomputer/registry.hpp>

using namespace benthos::dc;

DriverClass::DriverClass(const driver_manifest_t * manifest, const driver_interface_t * table)
	: m_manifest(manifest), m_table(table)
{
}

DriverClass::~DriverClass()
{
}

const std::string & DriverClass::description() const
{
	return m_manifest->driver_desc;
}

const std::string & DriverClass::interface() const
{
	return m_manifest->driver_intf;
}

const std::string & DriverClass::model_parameter() const
{
	return m_manifest->model_param;
}

const std::map<int, model_info_t> & DriverClass::models() const
{
	return m_manifest->driver_models;
}

const std::string & DriverClass::name() const
{
	return m_manifest->driver_name;
}

boost::shared_ptr<Driver> DriverClass::open(const std::string & device, const std::string & args) const
{
	return boost::shared_ptr<Driver>(new Driver(m_table, m_manifest, device, args));
}

const std::list<param_info_t> & DriverClass::parameters() const
{
	return m_manifest->driver_params;
}

const std::string & DriverClass::plugin() const
{
	return m_manifest->plugin_name;
}
