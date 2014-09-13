/*
 * Copyright (C) 2014 Asymworks, LLC.  All Rights Reserved.
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

#ifndef SMARTI_CLIENT_H_
#define SMARTI_CLIENT_H_

/**
 * @file src/plugins/smarti/smarti_client.h
 * @brief Smart-I Client Functions
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <stdint.h>

//! Opaque Smart-I Client Handle Type
typedef struct smarti_client_t_ *	smarti_client_t;

/**
 * @brief Smart-I Device Enumeration Callback
 * @param[in] Callback Data
 * @param[in] Device Address
 * @param[in] Device Name
 * @return Return non-zero to cancel enumeration
 */
typedef int (* smarti_enum_cb_t)(void *, uint32_t, const char *);

/**
 * @brief Initialize the Smart-I Client
 * @return Zero on Success, Non-Zero on Failure
 *
 * Initializes the Smart-I client subsystem.  Must be called prior to calling
 * any other Smart-I client functions.
 */
int smarti_client_init();

//! @brief Clean Up the Smart-I Client
void smarti_client_cleanup();

/**
 * @brief Allocate a new Smart-I Client Object
 * @param[out] New Smart-I Client Handle
 * @return Zero on Success, Non-Zero on Failure
 */
int smarti_client_alloc(smarti_client_t *);

//! @brief Dispose of a Smart-I Client Object
void smarti_client_dispose(smarti_client_t);

//! @return Smart-I Client Error Code
int smarti_client_errcode(smarti_client_t);

//! @return Smart-I Client Error Message
const char * smarti_client_errmsg(smarti_client_t);

/**
 * @brief Connect to a Smart-I Protocol Server
 * @param[in] Smart-I Client Handle
 * @param[in] Server Address or Host Name
 * @param[in] Server Port
 * @return Zero on Success, Non-Zero on Failure
 *
 * Attempts to connect to a Smart-I protocol daemon running on the given
 * host and port.
 */
int smarti_client_connect(smarti_client_t, const char *, uint16_t);

//! @brief Disconnect from a Smart-I Protocol Server
int smarti_client_disconnect(smarti_client_t);


//! @return Smart-I Server Address
const char * smarti_client_server_addr(smarti_client_t);

//! @return Smart-I Server ID String
const char * smarti_client_server_id(smarti_client_t);

//! @return Smart-I Server Port
uint16_t smarti_client_server_port(smarti_client_t);

/**
 * @brief Enumerate Smart Devices
 * @param[in] Smart-I Client Handle
 * @param[in] Callback Function
 * @param[in] Callback Data
 * @return Zero on Success, Non-Zero on Failure
 *
 * Enumerates devices available on the server and calls the callback function
 * for each device returned.  If no devices are found the function will return
 * the SMARTI_STATUS_NO_DEVS status code.
 */
int smarti_client_enumerate(smarti_client_t, smarti_enum_cb_t, void *);

/**
 * @brief Open a Smart Device
 * @param[in] Smart-I Client Handle
 * @param[in] Device Address
 * @param[in] LSAP Endpoint
 * @param[in] Chunk Size
 * @return Zero on Success, Non-Zero on Failure
 *
 * Opens a connection to the device on the Smart-I server.  The address should
 * be the same as what was provided to the callback function during enumeration.
 */
int smarti_client_open(smarti_client_t, uint32_t, uint32_t, uint8_t);

//! @brief Close the current Smart Device
int smarti_client_close(smarti_client_t);

/**
 * @brief Get the Smart Device Model Number
 * @param[in] Smart-I Client Handle
 * @param[out] Smart Device Model Number
 * @return Zero on Success, Non-Zero on Failure
 */
int smarti_client_model(smarti_client_t, uint8_t *);

/**
 * @brief Get the Smart Device Serial Number
 * @param[in] Smart-I Client Handle
 * @param[out] Smart Device Serial Number
 * @return Zero on Success, Non-Zero on Failure
 */
int smarti_client_serial(smarti_client_t, uint32_t *);

/**
 * @brief Get the Smart Device Tick Count
 * @param[in] Smart-I Client Handle
 * @param[out] Smart Device Tick Count
 * @return Zero on Success, Non-Zero on Failure
 */
int smarti_client_ticks(smarti_client_t, uint32_t *);

/**
 * @brief Set the Smart Device Transfer Token
 * @param[in] Smart-I Client Handle
 * @param[in] Transfer Token
 * @return Zero on Success, Non-Zero on Failure
 *
 * Sets the transfer token for the next data transfer.  For Smart devices, the
 * token is the tick count after which dives should be transferred.
 */
int smarti_client_set_token(smarti_client_t, uint32_t);

/**
 * @brief Get the Transfer Data Size
 * @param[in] Smart-I Client Handle
 * @param[out] Transfer Data Size
 * @return Zero on Success, Non-Zero on Failure
 *
 * Returns the number of bytes that are available to transfer following the
 * currently-set transfer token.  If no dives were made after the token time,
 * the data size will be returned as zero.
 */
int smarti_client_size(smarti_client_t, uint32_t *);

/**
 * @brief Transfer Data from the Device
 * @param[in] Smart-I Client Handle
 * @param[out] Data Buffer Pointer
 * @param[out] Data Buffer Size
 * @return Zero on Success, Non-Zero on Failure
 *
 * Transfers data from the device following the currently-set transfer token.
 * The function will allocate the buffer and store it in the location given,
 * and the client is responsible for free()'ing the buffer when finished.  The
 * data buffer size will be set to the actual buffer size and will match the
 * size returned in smarti_client_size() if no errors occur.
 */
int smarti_client_xfer(smarti_client_t, void **, size_t *);

#endif /* SMARTI_CLIENT_H_ */
