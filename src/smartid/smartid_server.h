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

#ifndef SMARTI_SERVER_H_
#define SMARTI_SERVER_H_

/**
 * @file smarti_server.h
 * @brief Smart-I Daemon Server Control
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 *
 * Functions to handle controlling the Smart-I Daemon
 */

/**
 * @brief Initialize the Smart-I Daemon
 * @return Zero On Success, Non-Zero on Failure
 *
 * Initailizes the Daemon State.  Must be called before any other daemon
 * calls are made.
 */
int smartid_init(void);

/**
 * @brief Clean Up the Smart-I Daemon
 *
 * Clean up the Daemon State.  Must be called following daemon shutdown and
 * prior to program exit.
 */
void smartid_cleanup(void);

/**
 * @brief Start the Smart-I Daemon
 * @param[in] Listener Socket
 * @param[in] Server Bind Address
 * @param[in] Server Bind Port
 * @return Zero on Success, Non-Zero on Failure
 *
 * Starts the main daemon event loop.  This call is blocking, but the event
 * loop can be exited by calling smartid_stop().
 */
int smartid_start(int, const char *, int);

/**
 * @brief Stop the Smart-I Daemon
 * @param[in] Signal Code
 * @return Zero on Success, Non-Zero on Failure
 *
 * Stops the main daemon event loop.  This call is guaranteed to be non-
 * blocking.
 */
int smartid_stop(int);

#endif /* SMARTI_SERVER_H_ */
