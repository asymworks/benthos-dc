/*
 * smarti_codes.h
 *
 *  Created on: Aug 29, 2014
 *      Author: jkrauss
 */

#ifndef SMARTI_CODES_H_
#define SMARTI_CODES_H_

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
