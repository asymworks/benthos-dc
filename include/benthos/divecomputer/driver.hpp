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

#ifndef BENTHOS_DC_DRIVER_HPP_
#define BENTHOS_DC_DRIVER_HPP_

/**
 * @file include/benthos/divecomputer/driver.hpp
 * @brief Dive Computer Driver Interface
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <benthos/divecomputer/driverclass.hpp>
#include <benthos/divecomputer/plugin.hpp>

namespace benthos { namespace dc
{

typedef std::vector<uint8_t>							dive_buffer_t;
typedef std::pair<dive_buffer_t, std::string>			dive_entry_t;

/**
 * @brief Device Driver Class
 *
 * Encapsulates a connection to a device.  This class may not be created
 * directly; instead use the factory method provided by DriverClass.
 */
class Driver
{
public:
	typedef boost::shared_ptr<Driver>	Ptr;

protected:

	/**
	 * @brief Class Constructor
	 * @param[in] Driver Interface Table
	 * @param[in] Driver Manifest
	 * @param[in] Device Path
	 * @param[in] Driver Arguments
	 */
	Driver(const driver_interface_t * driver, const driver_manifest_t * manifest, const std::string & path, const std::string & args);

public:

	//! Class Destructor
	~Driver();

public:

	//! @return Last Driver Error Message
	std::string errmsg() const;

	/**
	 * @brief Get the Manufacturer Name for a Model Number
	 * @param[in] Model Number
	 * @return Manufacturer Name
	 */
	std::string manufacturer(uint8_t model_number);

	/**
	 * @brief Get the Model Name for a Model Number
	 * @param[in] Model Number
	 * @return Model Name
	 */
	std::string model_name(uint8_t model_number);

	//! @return Model Number of Connected Device
	uint8_t model_number() const;

	//! @return Serial Number of Connected Device
	uint32_t serial_number() const;

	//! @return Driver Name
	const std::string & name() const;

	//! @return Plugin Name
	const std::string & plugin() const;

public:

	/**
	 * @brief Parse a Dive
	 * @param[in] Dive Data
	 * @param[in] Header Data Callback
	 * @param[in] Profile Data Callback
	 * @param[in] User Data for Callback
	 */
	void parse(dive_buffer_t data, header_callback_fn_t hcb, waypoint_callback_fn_t pcb, void * userdata);

	/**
	 * @brief Transfer Dives from the Device
	 * @param[in] Device Callback Function
	 * @param[in] Progress Callback Function
	 * @param[in] User Data for Callback
	 *
	 * Transfers all data from the device since the transfer token (if set).
	 * The progress callback is called periodically (no guaranteed rate) during
	 * data transfer to report current transfer progress.
	 */
	std::list<dive_entry_t> transfer(device_callback_fn_t dcb, transfer_callback_fn_t pcb, void * userdata) const;

private:
	const driver_interface_t *			m_driver;
	const driver_manifest_t *			m_manifest;
	dev_handle_t 						m_device;
	parser_handle_t						m_parser;

	friend class DriverClass;

};

} } /* benthos::dc */

#endif /* BENTHOS_DC_DRIVER_HPP_ */
