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

#include <stdexcept>

#include <common/arglist.h>
#include <benthos/divecomputer/arglist.hpp>

using namespace benthos::dc;

struct ArgumentList::_impl
{
	std::string			m_params;
	arglist_t			m_args;
};

ArgumentList::ArgumentList(const std::string & params)
	: m_pimpl(0)
{
	m_pimpl = new _impl;
	if (! m_pimpl)
		throw std::runtime_error("Could not allocate a new implementation for ArgumentList");

	m_pimpl->m_params = params;
	m_pimpl->m_args = 0;

	int rv = arglist_parse(& m_pimpl->m_args, params.c_str());
	if (rv != 0)
		throw std::runtime_error("Invalid parameter list");
}

ArgumentList::~ArgumentList()
{
	if (m_pimpl && m_pimpl->m_args)
		arglist_close(m_pimpl->m_args);
}

size_t ArgumentList::count() const
{
	return arglist_count(m_pimpl->m_args);
}

bool ArgumentList::has(const std::string & name) const
{
	return (bool)arglist_has(m_pimpl->m_args, name.c_str());
}

bool ArgumentList::has_value(const std::string & name) const
{
	return (bool)arglist_hasvalue(m_pimpl->m_args, name.c_str());
}

boost::optional<double> ArgumentList::read_float(const std::string & name) const
{
	double ret;
	int rv = arglist_read_float(m_pimpl->m_args, name.c_str(), & ret);
	if (rv == 0)
		return boost::optional<double>(ret);
	if (rv == 1)
		return boost::optional<double>();

	throw std::runtime_error("Failed to read value");
}

boost::optional<int32_t> ArgumentList::read_integer(const std::string & name) const
{
	int32_t ret;
	int rv = arglist_read_int(m_pimpl->m_args, name.c_str(), & ret);
	if (rv == 0)
		return boost::optional<int32_t>(ret);
	if (rv == 1)
		return boost::optional<int32_t>();

	throw std::runtime_error("Failed to read value");
}

boost::optional<std::string> ArgumentList::read_string(const std::string & name) const
{
	const char * ret;
	int rv = arglist_read_string(m_pimpl->m_args, name.c_str(), & ret);
	if (rv == 0)
		return boost::optional<std::string>(std::string(ret));
	if (rv == 1)
		return boost::optional<std::string>();

	throw std::runtime_error("Failed to read value");
}

boost::optional<uint32_t> ArgumentList::read_unsigned(const std::string & name) const
{
	uint32_t ret;
	int rv = arglist_read_uint(m_pimpl->m_args, name.c_str(), & ret);
	if (rv == 0)
		return boost::optional<uint32_t>(ret);
	if (rv == 1)
		return boost::optional<uint32_t>();

	throw std::runtime_error("Failed to read value");
}
