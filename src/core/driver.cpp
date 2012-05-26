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

using namespace benthos;

Driver::Driver(const driver_interface_t * driver, const std::string & path, const std::string & args)
	: m_driver(driver), m_device(0)
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
}

Driver::~Driver()
{
	m_driver->driver_close(m_device);
}
