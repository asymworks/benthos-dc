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

#ifndef WRAPPERS_HPP_
#define WRAPPERS_HPP_

#include <list>
#include <string>
#include <vector>

#include <benthos/divecomputer/manifest.h>

typedef struct
{
	std::string		model_name;
	std::string		model_manuf;

	model_info_t	model_info;

} model_wrapper_t;

typedef struct
{
	std::string		param_name;
	std::string		param_desc;
	std::string		param_default;

	param_info_t	param_info;

} param_wrapper_t;

typedef struct
{
	std::string							driver_name;
	std::string							driver_desc;
	std::string							model_param;

	driver_info_t						driver_info;

	std::vector<const param_info_t *>	params;
	std::vector<const model_info_t *>	models;

	std::list<param_wrapper_t>			param_wrappers;
	std::list<model_wrapper_t>			model_wrappers;

} driver_wrapper_t;

#endif /* WRAPPERS_HPP_ */
