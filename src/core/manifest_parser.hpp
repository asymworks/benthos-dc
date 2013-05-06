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

#ifndef MANIFEST_PARSER_HPP_
#define MANIFEST_PARSER_HPP_

/**
 * @file src/core/manifest_parser.hpp
 * @brief Plugin Manifest Parser
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <string>
#include <benthos/divecomputer/registry.hpp>

using namespace benthos::dc;

/**
 * @brief Parse a Manifest File
 * @param[in] Manifest File Name
 * @return Manifest Information
 */
plugin_manifest_t parseManifest(const std::string & manifest);

/**
 * @brief Lookup a Named Type in the Manifest
 * @param[in] Manifest
 * @param[in] Type Name
 * @return Type Specification Pointer
 */
setting_typespec_ptr manifest_lookup_type(const driver_manifest_t &, const std::string &);

/**
 * @brief Lookup a Named Setting in the Manifest
 * @param[in] Manifest
 * @param[in] Setting Name
 * @param[in] Model Identifier
 * @return Setting Specification
 *
 * Looks up a named setting in the manifest.  If a model-specific setting has
 * the same name as a global setting, the model-specific setting will override
 * the global one.
 */
setting_info_ptr manifest_lookup_setting(const driver_manifest_t &, const std::string &, int);

#endif /* MANIFEST_PARSER_HPP_ */
