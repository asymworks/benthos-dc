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

#ifndef SMARTI_CONN_H_
#define SMARTI_CONN_H_

/**
 * @file smarti_conn.h
 * @brief Smart-I Daemon Connection
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 *
 * Functions to handle client connections in the Smart-I Daemon.
 */

#include "irda.h"
#include "smarti_codes.h"
#include "smartid_device.h"
#include "smartid_version.h"

//! Smart-I Connection Handle
typedef struct smarti_conn_t_ *		smarti_conn_t;

/**
 * @brief Create a new Smart-I Connection
 * @param[in] Client Socket File Descriptor
 * @param[in] Client IP Address
 * @param[in] Server Event Loop
 * @return Connection Handle
 *
 * Initializes a new connection instance for the socket descriptor and sets up
 * the connection resources.  The returned handle must be freed by calling
 * smartid_conn_dispose() when finished.
 */
smarti_conn_t smartid_conn_alloc(int sockfd, const char * client_addr, struct event_base * evloop);

/**
 * @brief Dispose of a Smart-I Connection
 * @param[in] Connection Handle
 *
 * Releases a connection instance and frees all associated memory.
 */
void smartid_conn_dispose(smarti_conn_t conn);

/**
 * @brief Clenaup Smart-I Connections
 *
 * Releases all active connection instances and frees memory.  This function
 * should be called prior to exiting the main program.
 */
void smartid_conn_cleanup(void);

/**
 * @brief Send a Response Message to the Connection
 * @param[in] Connection Handle
 * @param[in] Status Code
 * @param[in] Status Message
 */
void smartid_conn_send_response(smarti_conn_t conn, uint16_t code, const char * msg);

/**
 * @brief Send a Formatted Response Message to the Connection
 * @param[in] Connection Handle
 * @param[in] Status Code
 * @param[in] Status Message
 * @param Formatting Arguments
 */
void smartid_conn_send_responsef(smarti_conn_t conn, uint16_t code, const char * msg, ...);

/**
 * @brief Send a Data Buffer to the Connection
 * @param[in] Connection Handle
 * @param[in] Data Buffer Pointer
 * @param[in] Data Buffer Length
 *
 * Sends the contents of a data buffer to the connection, which first sends
 * the status reply "301 Data Follows" followed by the base64-encoded data.
 * The end of the data is signified by a blank line.
 */
void smartid_conn_send_buffer(smarti_conn_t conn, void * data, size_t len);

/**
 * @brief Flush the Connection Output Buffer
 * @param[in] Connection Handle
 */
void smartid_conn_flush(smarti_conn_t conn);

//! @return Connection Client Address
const char * smartid_conn_client(smarti_conn_t conn);

//! @return Connection IrDA Handle
irda_t smartid_conn_irda(smarti_conn_t conn);

//! @return Connection Device Handle
smart_device_t smartid_conn_device(smarti_conn_t conn);

//! Set Connection Device Handle
void smartid_conn_set_device(smarti_conn_t conn, smart_device_t dev);

//! Send Ready Response
#define smartid_conn_send_ready(c) \
	smartid_conn_send_responsef(c, SMARTI_STATUS_READY, "%s %s Ready", \
		SMARTID_APP_TITLE, \
		SMARTID_VERSION_STRING)

#endif /* SMARTI_CONN_H_ */
