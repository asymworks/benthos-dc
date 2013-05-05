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

#ifndef PLUGIN_DRIVER_H_
#define PLUGIN_DRIVER_H_

/**
 * @file include/benthos/divecomputer/plugin/driver.h
 * @brief Plugin Driver Interface
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Opaque Pointer to a Device driver Instance
 */
typedef struct device_t *		dev_handle_t;

/**
 * @brief Driver Device Data Callback
 * @param[in] User Data Pointer
 * @param[in] Device Model Number
 * @param[in] Device Serial Number
 * @param[in] Device Tick Count
 * @param[out] Transfer Token
 * @param[out] Free Transfer Token Memory
 * @return Error Code or 0 to Continue
 *
 * This function is called by the driver when it has connected to a device and
 * has received device information.  The client application should use this
 * information to lookup a token for the device.  If the client cannot process
 * data for this device it should return a non-zero error code which will
 * cancel the transfer (if possible).
 *
 * The callback function has the option of returning a static character pointer
 * or a dynamic memory buffer for the transfer token.  If it returns a static
 * pointer it should set the final argument to 0, otherwise it should allocate
 * a buffer using malloc() and set the final argument to 1, which will cause the
 * driver to free the memory.
 */
typedef int (* device_callback_fn_t)(void *, uint8_t, uint32_t, uint32_t, char **, int *);

/**
 * @brief Data Transfer Callback
 * @param[in] User Data Pointer
 * @param[in] Current Number of Bytes Transferred
 * @param[in] Total Number of Bytes to Transfer
 * @param[out] Cancel Transfer Flag
 */
typedef void (* transfer_callback_fn_t)(void *, uint32_t, uint32_t, int *);

/**
 * @brief Dive Data Callback
 * @param[in] User Data Pointer
 * @param[in] Dive Data Buffer
 * @param[in] Size of the Data Buffer
 * @param[in] Token for this Dive
 */
typedef void (* divedata_callback_fn_t)(void *, void *, uint32_t, const char *);

/**
 * @brief Create a Device Handle
 * @param[out] Pointer to new Device Handle
 * @return Error value or 0 for success
 */
typedef int (* plugin_driver_create_fn_t)(dev_handle_t *);

/**
 * @brief Open a Device
 * @param[in] Pointer to Device Handle
 * @param[in] Device Name or Path
 * @param[in] Argument List
 * @return Error value or 0 for success.
 */
typedef int (* plugin_driver_open_fn_t)(dev_handle_t, const char *, const char *);

/**
 * @brief Close a Device
 * @param[in] Device Handle
 */
typedef void (* plugin_driver_close_fn_t)(dev_handle_t);

/**
 * @brief Shutdown a Device
 * @param[in] Device Handle
 *
 * Causes an asynchronous shutdown of the device connection socket, cancelling
 * blocking I/O.
 */
typedef void (* plugin_driver_shutdown_fn_t)(dev_handle_t);

/**
 * @brief Get the Device Driver Name
 * @return Driver Name
 */
typedef const char * (* plugin_driver_name_fn_t)(dev_handle_t);

/**
 * @brief Get the Error String from a Plugin
 * @param[in] Device Handle
 * @return Error String
 */
typedef const char * (* plugin_driver_errmsg_fn_t)(dev_handle_t);

/**
 * @brief Get the Device Model Number
 * @param[in] Device Handle
 * @param[out] Model Number
 * @return Error value or 0 for success
 */
typedef int (* plugin_driver_get_model_fn_t)(dev_handle_t, uint8_t *);

/**
 * @brief Get the Device Serial Number
 * @param[in] Device Handle
 * @param[out] Serial Number
 * @return Error value or 0 for success
 */
typedef int (* plugin_driver_get_serial_fn_t)(dev_handle_t, uint32_t *);

/**
 * @brief Transfer bytes from the Dive Computer
 * @param[in] Device Handle
 * @param[out] Data Buffer
 * @param[out] Buffer Size
 * @param[in] Device Data Callback Function Pointer
 * @param[in] Transfer Progress Callback Function Pointer
 * @param[in] Callback Function User Data
 * @return Error value or 0 for success
 *
 * Transfers data from the dive computer and stores it in the given buffer.  The
 * caller is responsible freeing the data buffer after it has been filled by
 * the transfer function.
 *
 * The device data callback function will be called once before data is
 * transferred to allow the client to set a token.  The progress callback
 * function will be called before data transfer starts, periodically during
 * transfer, and once when all bytes have been transferred.
 *
 * The callbacks may be NULL.
 */
typedef int (* plugin_driver_transfer_fn_t)(dev_handle_t, void **, uint32_t *, device_callback_fn_t, transfer_callback_fn_t, void *);

/**
 * @brief Extract Dives from the Transferred Data
 * @param[in] Device Handle
 * @param[in] Data Buffer
 * @param[in] Buffer Size
 * @param[in] Callback Function Pointer
 * @param[in] Callback Function User Data
 * @return Error value or 0 for success
 *
 * Extracts individual dives from the transferred data and calls the given
 * callback function on each dive.
 */
typedef int (* plugin_driver_extract_fn_t)(dev_handle_t, void *, uint32_t, divedata_callback_fn_t, void *);

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_DRIVER_H_ */
