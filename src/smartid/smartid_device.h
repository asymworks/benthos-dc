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

#ifndef SMARTID_DEVICE_H_
#define SMARTID_DEVICE_H_

/**
 * @file smartid_device.h
 * @brief Smart-I Smart Device Driver
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <stdint.h>

#include "irda.h"

//! Smart Device Handle
typedef struct smart_device_t_ * smart_device_t;

/**
 * @brief Allocate a new Smart Device
 * @param[in] IrDA Connection Handle
 * @return New Smart Device Handle
 */
smart_device_t smartid_dev_alloc(irda_t);

/**
 * @brief Dispose of the Smart Device
 * @param[in] Smart Device Handle
 *
 * Closes the Smart Device and frees all associated memory.
 */
void smartid_dev_dispose(smart_device_t);

/**
 * @brief Connect to a Smart Device
 * @param[in] Smart Device Handle
 * @param[in] IrDA Endpoint Address
 * @param[in] IrDA LSAP Specifier
 * @param[in] IrDA Chunk Size
 * @return Zero on Success, Non-Zero on Failure
 */
int smartid_dev_connect(smart_device_t, unsigned int, int, size_t);

/**
 * @brief Get the Model Number of a Smart Device
 * @param[in] Smart Device Handle
 * @param[out] Smart Device Model
 * @return Zero on Success, Non-Zero on Failure
 */
int smartid_dev_model(smart_device_t, uint8_t *);

/**
 * @brief Get the Serial Number of a Smart Device
 * @param[in] Smart Device Handle
 * @param[out] Smart Device Serial
 * @return Zero on Success, Non-Zero on Failure
 */
int smartid_dev_serial(smart_device_t, uint32_t *);

/**
 * @brief Get the Time Correction of a Smart Device
 * @param[in] Smart Device Handle
 * @param[out] Smart Device Time Correction
 * @return Zero on Success, Non-Zero on Failure
 */
int smartid_dev_tcorr(smart_device_t, uint32_t *);

/**
 * @brief Get the Time Ticks of a Smart Device
 * @param[in] Smart Device Handle
 * @param[out] Smart Device Ticks
 * @return Zero on Success, Non-Zero on Failure
 */
int smartid_dev_ticks(smart_device_t, uint32_t *);

/**
 * @brief Set the Transfer Token for the Smart Device
 * @param[in] Smart Device Handle
 * @param[in] Smart Device Token
 * @return Zero on Success, Non-Zero on Failure
 */
int smartid_dev_set_token(smart_device_t, uint32_t);

/**
 * @brief Get the Transfer Size for the Smart Device
 * @param[in] Smart Device Handle
 * @param[out] Transfer Size
 * @return Zero on Success, Non-Zero on Failure
 */
int smartid_dev_xfer_size(smart_device_t, uint32_t *);

/**
 * @brief Get the Dive Data from the Smart Device
 * @param[in] Smart Device Handle
 * @param[out] Data Buffer
 * @param[out] Data Size
 * @return Zero on Success, Non-Zero on Failure
 */
int smartid_dev_xfer_data(smart_device_t, uint8_t **, uint32_t *);

#endif /* SMARTID_DEVICE_H_ */
