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

#ifndef SMARTI_CODES_H_
#define SMARTI_CODES_H_

/**
 * @file include/benthos/smarti/smarti_codes.h
 * @brief Smart-I Daemon Response Codes
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

/**@{
 * @name Smart-I Daemon Response Codes
 */
#define SMARTI_STATUS_SUCCESS			210		///< Operation Successful
#define	SMARTI_STATUS_READY				220		///< Daemon Ready to Receive Command
#define SMARTI_STATUS_INFO				230		///< Device or Daemon Information
#define SMARTI_STATUS_NO_DEVS			241		///< No Devices Found (ENUM command)
#define SMARTI_STATUS_NO_DATA			243		///< No Data to Transfer (XFER command)

#define SMARTI_DATA_FOLLOWS				301		///< Base-64 Data Follows (XFER command)

#define SMARTI_ERROR_NO_DEVICE			400		///< No Device is Connected
#define SMARTI_ERROR_CONNECT			401		///< Unable to Connect to the Device
#define SMARTI_ERROR_HANDSHAKE			402		///< Handshake Failed with the Device
#define SMARTI_ERROR_IO					403		///< An I/O Error Occurred with the Device
#define SMARTI_ERROR_EXISTS				410		///< A Device is Already Connected (OPEN command)
#define SMARTI_ERROR_TIMEOUT			440		///< Transfer Timed Out with Device

#define SMARTI_ERROR_UNKNOWN_COMMAND	500		///< Unknown Command
#define SMARTI_ERROR_INVALID_COMMAND	501		///< Invalid Command (syntax error)
#define SMARTI_ERROR_IRDA				510		///< An IrDA Error Occurred
#define SMARTI_ERROR_INTERNAL			520		///< An Unspecified Error Occurred
#define SMARTI_ERROR_DEVICE				540		///< A Device Error Occurred (XFER command)
/*@}*/

#endif /* SMARTI_CODES_H_ */
