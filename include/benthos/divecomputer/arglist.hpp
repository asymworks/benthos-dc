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

#ifndef BENTHOS_DC_ARGLIST_HPP_
#define BENTHOS_DC_ARGLIST_HPP_

/**
 * @file include/benthos/divecomputer/arglist.hpp
 * @brief Dive Computer Argument List Interface
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <string>

#include <boost/optional.hpp>

#include <benthos/divecomputer/exports.hpp>

namespace benthos { namespace dc
{

/**
 * @brief Argument List Class
 *
 * Provides a C++ interface to the argument list reading functions exposed to
 * driver plugins.  This interface uses the arglist_* functions to parse a
 * given argument list and provides C++ methods to query the argument list and
 * retrieve values.
 */
class BENTHOS_API ArgumentList
{
private:
	struct	_impl;

public:

	/**
	 * @brief Class Constructor
	 * @param[in] Parameter List to Parse
	 */
	ArgumentList(const std::string & params);

	//! Class Destructor
	~ArgumentList();


public:

	//! @return Number of Parameters in the list
	size_t count() const;

	//! @return True if the Parameter List includes the given name
	bool has(const std::string & name) const;

	//! @return True if the Parameter List includes a value for the given name
	bool has_value(const std::string & name) const;

	//! @return Read the given Parameter Value as a double
	boost::optional<double> read_float(const std::string & name) const;

	//! @return Read the given Parameter Value as an integer
	boost::optional<int32_t> read_integer(const std::string & name) const;

	//! @return Read the given Parameter Value as a string
	boost::optional<std::string> read_string(const std::string & name) const;

	//! @return Read the given Parameter Value as an unsigned integer
	boost::optional<uint32_t> read_unsigned(const std::string & name) const;

private:
	_impl *					m_pimpl;

};

} } /* benthos::dc */

#endif /* BENTHOS_DC_ARGLIST_HPP_ */
