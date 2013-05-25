/*
 * Copyright (C) 2012 Asymworks, LLC.  All Rights Reserved.
 * www.asymworks.com / info@asymworks.com
 *
 * This file is part of the Benthos Dive Log Package (benthos-log.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef COMMON_UTIL_H_
#define COMMON_UTIL_H_

/**
 * @file src/common/util.h
 * @brief Common utility functions and macros
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#if defined(_WIN32) || defined(WIN32)
#include <Windows.h>
#else
#include <errno.h>
#include <error.h>
#endif

/**
 * Set the OS Error Code for an invalid handle/pointer
 */
#if defined(_WIN32) || defined(WIN32)
#define BAD_POINTER()	SetLastError(ERROR_INVALID_HANDLE);
#else
#define BAD_POINTER()	errno = EINVAL;
#endif

#endif /* COMMON_UTIL_H_ */
