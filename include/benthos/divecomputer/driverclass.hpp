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

#ifndef BENTHOS_DC_DRIVERCLASS_HPP_
#define BENTHOS_DC_DRIVERCLASS_HPP_

/**
 * @file include/benthos/divecomputer/driver.hpp
 * @brief Dive Computer Driver Interface
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <list>
#include <map>
#include <string>

#include <benthos/divecomputer/plugin.hpp>

namespace benthos
{

// Forward Declaration of Driver class
class Driver;

/**
 * @brief Driver Class Interface
 *
 * Provides an object-oriented wrapper around a Device Driver Class.  The
 * class provides basic factory methods for Devices and provides an object-
 * oriented interface to driver properties.
 */
class DriverClass
{
public:
	typedef boost::shared_ptr<DriverClass>	Ptr;

public:

	/**
	 * @brief Class Constructor
	 * @param[in] Driver Manifest Entry
	 * @param[in] Driver Interface Table
	 * @param[in] Device Path
	 * @param[in] Driver Arguments
	 *
	 * Opens a connection to the given device using the specified driver
	 * information.
	 */
	DriverClass(const driver_manifest_t * manifest, const driver_interface_t * table);

	//! Class Destructor
	~DriverClass();

public:

	//! @return Driver Description
	const std::string & description() const;

	//! @return Driver Interface Type
	const std::string & interface() const;

	//! @return List of Models
	const std::map<int, model_info_t> models() const;

	//! @return Driver Name
	const std::string & name() const;

	//! @return List of Driver Parameters
	const std::list<param_info_t> & parameters() const;

	//! @return Plugin Name
	const std::string & plugin() const;

public:

	/**
	 * @brief Open a Connection to a Device
	 * @param[in] Device Path
	 * @param[in] Driver Arguments
	 *
	 * Attempts to connect to the given device and returns a Driver instance
	 * which can be used to communicate with the device.  If the driver is
	 * unable to connect to the device, an exception is raised.
	 */
	boost::shared_ptr<Driver> open(const std::string & device, const std::string & args) const;

private:
	const driver_manifest_t *		m_manifest;
	const driver_interface_t *		m_table;

};

} /* benthos */

#endif /* BENTHOS_DC_DRIVERCLASS_HPP_ */
