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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <regex.h>

#include <benthos/divecomputer/base64.h>
#include <benthos/smarti/smarti_codes.h>

#include "smarti_client.h"

struct smarti_client_t_
{
	int				s;			///< Network Socket File Descriptor
	long			timeout;	///< Network Socket Timeout (ms)

	char *			s_id;		///< Smart-I Server Name String
	char *			s_addr;		///< Smart-I Server Address
	uint16_t		s_port;		///< Smart-I Server Port

	int				errcode;	///< Error Code
	const char *	errmsg;		///< Error Message

};

/* Global Error String for Server Response Code Errors */
static char g_server_error[1024];

/* Helper to set the Client Error Information */
#define SET_ERROR(c, code, msg) \
	{ \
		c->errcode = (code); \
		c->errmsg = (msg); \
	}

/* Helper to check the Client Pointer */
#define CHECK_CLIENT_PTR(c) \
	if (! c) \
	{ \
		errno = EINVAL; \
		return -1; \
	}

/* Helper to ensure socket can be passed to select() */
#define IS_SELECTABLE(s) (((s) >= 0) && ((s) < FD_SETSIZE))

/* Helper to extract the IPv4/IPV6 in_addr from a sockaddr */
void * get_in_addr(struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET)
    {
        return & (((struct sockaddr_in *)sa)->sin_addr);
    }

    return & (((struct sockaddr_in6 *)sa)->sin6_addr);
}

/* Helper to call select() on the client socket with timeout */
int smartic_socket_select(int fd, int writing, long interval)
{
	int n;
	fd_set fds;
	struct timeval tv;

	tv.tv_sec = interval / 1000;
	tv.tv_usec = (interval % 1000) * 1000;

	FD_ZERO(& fds);
	FD_SET(fd, &fds);

	if (writing)
		n = select(fd+1, NULL, & fds, NULL, & tv);
	else
		n = select(fd+1, & fds, NULL, NULL, & tv);

	if (n < 0)
		return -1;
	if (n == 0)
		return 1;

	return 0;
}

/* Helper to call select() until timeout occurs */
#define BEGIN_SELECT_LOOP(s) \
	{ \
		long deadline, interval = s->timeout; \
		int has_timeout = (s->timeout > 0); \
		if (has_timeout) \
			deadline = (clock() * 1000 / CLOCKS_PER_SEC) + s->timeout; \
		while (1) { \
			errno = 0;

/* Helper to call select() until timeout occurs */
#define END_SELECT_LOOP(s) \
			if (! has_timeout || (! (errno == EWOULDBLOCK) && ! (errno == EAGAIN))) \
				break; \
			interval = deadline - (clock() * 1000 / CLOCKS_PER_SEC); \
		} \
	}

/* Helper to check if data is waiting to be read */
int smartic_socket_available(smarti_client_t c)
{
	int bytes;

	if (! c)
		return -1;

	if (ioctl(c->s, FIONREAD, & bytes) != 0)
		return -1;

	return bytes;
}

/* Helper to read a line from the server */
ssize_t smartic_socket_readln(smarti_client_t c, char * buffer, size_t len, int * timeoutp)
{
	ssize_t nr;
	size_t tot;
	int timeout;
	char * buf;
	char ch;

	if (! IS_SELECTABLE(c->s) || ! buffer || (len <= 0))
	{
		errno = EINVAL;
		return -1;
	}

	buf = buffer;
	tot = 0;
	for (;;)
	{
		BEGIN_SELECT_LOOP(c);
			timeout = smartic_socket_select(c->s, 0, interval);
			if (! timeout)
				nr = read(c->s, & ch, 1);

			if (timeout == 1)
			{
				if (timeoutp) * timeoutp = 1;
				break;
			}
		END_SELECT_LOOP(c);

		if (timeout)
		{
			if (tot == 0)
			{
				/* No bytes read */
				return 0;
			}
			else
			{
				/* Add NUL and return */
				break;
			}
		}

		if (nr == -1)
		{
			if (errno == EINTR)
			{
				/* Interrupted -> Restart read() */
				continue;
			}
			else
			{
				/* Some other error */
				return -1;
			}
		}
		else if (nr == 0)
		{
			/* End of File */
			if (tot == 0)
			{
				/* No bytes read */
				return 0;
			}
			else
			{
				/* Add NUL and return */
				break;
			}
		}
		else
		{
			/* Skip Newline */
			if (ch == '\n')
				break;

			/* Append the Character */
			if (tot < (len - 1))
			{
				++tot;
				*buf++ = ch;
			}
		}
	}

	* buf = 0;

	return tot;
}

/* Helper to write data to the server */
int smartic_socket_write(smarti_client_t c, const char * buffer, size_t * len, int * timeoutp)
{
	int timeout = 0;
	size_t nbytes = 0;

	if (! IS_SELECTABLE(c->s) || ! buffer || ! len || (* len <= 0))
	{
		errno = EINVAL;
		return -1;
	}

	while (nbytes < (* len))
	{
		ssize_t n;

		BEGIN_SELECT_LOOP(c);
			timeout = smartic_socket_select(c->s, 1, interval);
			if (! timeout)
				n = write(c->s, buffer + nbytes, (* len) - nbytes);

			if (timeout == 1)
			{
				if (timeoutp) * timeoutp = 1;
				break;
			}
		END_SELECT_LOOP(c);

		if (timeout)
			break;

		if (n < 0)
		{
			if (errno == EINTR)
			{
				/* Interrupted -> Restart write() */
				continue;
			}
			else
			{
				/* Some other error */
				* len = nbytes;
				return -1;
			}
		}

		if (n == 0)
		{
			/* No bytes written */
			break;
		}

		nbytes += n;
	}

	* len = nbytes;

	return 0;
}

/**
 * @brief Parse a Server Response
 * @param[in] Server Response Line
 * @param[out] Response Code
 * @param[out] Response Message
 * @return Zero on Success, Non-Zero on Failure
 *
 * Performs a first-pass parse of the server response line, extracting the
 * three-digit status code and the remainder of the message.  The function
 * returns:
 *
 * - 0 if the response was successfully parsed
 * - 1 if the arguments were invalid (errno is set to EINVAL)
 * - 2 if the response line is invalid (not of the form [code] [message])
 *
 * Note that the message parameter will be set to a position within line, not
 * to a new string.  In particular it will be set to the fifth character of
 * line assuming that the first three characters are digits and the fourth
 * is a space.
 */
int smartic_parse_response(const char * line, int * code, const char ** message)
{
	if (! line || ! code || ! message)
	{
		errno = EINVAL;
		return 1;
	}

	if ((strlen(line) < 4) || ! isdigit(line[0]) || ! isdigit(line[1]) || ! isdigit(line[2]) || ! isspace(line[3]))
		return 2;

	* code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');
	* message = & line[4];

	return 0;
}

/**
 * @brief Parse the Greeting Message
 * @param[in] Greeting Line
 * @param[out] Status Code
 * @param[out] Server ID String
 * @param[out] Extra Message
 * @return Zero on Success, Non-Zero on Failure
 *
 * Parses the greeting string from the server, which is defined to be formatted
 * <SERVER_ID> <VER_MAJ>.<VER_MIN>[.<VER_PATCH>[_<VER_REL>]] <EXTRA>
 *
 * So a valid server greeting string would look like
 * Smart-I Daemon 1.0.5 Ready
 *
 * which parses into
 * - NAME: Smart-I Daemon
 * - VERS: 1.0.5
 * - EXTRA: Ready
 */
int smartic_parse_greeting(const char * line, char ** name, char ** version, char ** extra)
{
	int rv;
	regex_t re;
	regmatch_t m[12];

	rv = regcomp(& re, "(.*) (([0-9]+)\\.([0-9]+)(\\.([0-9])(_([A-Za-z0-9]*))?)?) (.*)", REG_EXTENDED);
	if (rv != 0)
		return rv;

	rv = regexec(& re, line, 12, m, 0);
	if (rv == 0)
	{
		* name 		= strndup(line + m[1].rm_so, (m[1].rm_eo - m[1].rm_so));
		* version 	= strndup(line + m[2].rm_so, (m[2].rm_eo - m[2].rm_so));
		* extra     = strndup(line + m[9].rm_so, (m[9].rm_eo - m[9].rm_so));
	}
	else if (rv == REG_NOMATCH)
	{
		regfree(& re);
		return EINVAL;
	}

	regfree(& re);
	return rv;
}

/**
 * @brief Parse a Enumeration Info Message
 * @param[in] Device Info Message
 * @param[out] Device Address
 * @param[out] Device Name
 * @return Zero on Success, Non-Zero on Failure
 *
 * Parses a Enumeration Information string from the server, which is defined to
 * be formatted as
 * [DESC STRING]: [ADDRESS] [NAME]
 *
 * The [DESC STRING] may be any descriptive string and is terminated by a colon
 * and followed by the device address and name.
 */
int smartic_parse_enuminfo(const char * line, uint32_t * addr, char ** name)
{
	int rv;
	regex_t re;
	regmatch_t m[5];

	rv = regcomp(& re, "(.*): ([0-9]+) (.*)", REG_EXTENDED);
	if (rv != 0)
		return rv;

	rv = regexec(& re, line, 5, m, 0);
	if (rv == 0)
	{
		char * tmp;

		  tmp  = strndup(line + m[2].rm_so, (m[2].rm_eo - m[2].rm_so));
		* name = strndup(line + m[3].rm_so, (m[3].rm_eo - m[3].rm_so));
		* addr = strtoul(tmp, 0, 10);

		free(tmp);
	}
	else if (rv == REG_NOMATCH)
	{
		regfree(& re);
		return EINVAL;
	}

	regfree(& re);
	return rv;
}

/**
 * @brief Parse a Device Info Message
 * @param[in] Device Info Message
 * @param[out] Device Info
 * @return Zero on Success, Non-Zero on Failure
 *
 * Parses a Device Information string from the server, which is defined to be
 * formatted as
 * [DESC STRING]: [INFO]
 *
 * The [DESC STRING] may be any descriptive string and is terminated by a colon
 * and followed by the device information requested.
 */
int smartic_parse_devinfo(const char * line, char ** info)
{
	int rv;
	regex_t re;
	regmatch_t m[5];

	rv = regcomp(& re, "(.*): (.*)", REG_EXTENDED);
	if (rv != 0)
		return rv;

	rv = regexec(& re, line, 5, m, 0);
	if (rv == 0)
	{
		* info = strndup(line + m[2].rm_so, (m[2].rm_eo - m[2].rm_so));
			}
	else if (rv == REG_NOMATCH)
	{
		regfree(& re);
		return EINVAL;
	}

	regfree(& re);
	return rv;
}

int smarti_client_alloc(smarti_client_t * c)
{
	smarti_client_t ret;

	CHECK_CLIENT_PTR(c)

	ret = (smarti_client_t)malloc(sizeof(struct smarti_client_t_));
	if (! ret)
		return -1;

	ret->s = -1;
	ret->timeout = 2000;

	ret->s_id = 0;
	ret->s_addr = 0;
	ret->s_port = 0;

	ret->errcode = 0;
	ret->errmsg = 0;

	* c = ret;

	return 0;
}

void smarti_client_cleanup(void)
{
}

int smarti_client_close(smarti_client_t c)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;

	const char * cmd = "CLOSE\n";
	size_t len = strlen(cmd);

	CHECK_CLIENT_PTR(c)

	/* Send CLOSE Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
		return rv;

	/* Read Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for Success */
	if (r_code != SMARTI_STATUS_SUCCESS)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	return 0;
}

int smarti_client_connect(smarti_client_t c, const char * addr, uint16_t port)
{
	int rv;
	int fd = -1;
	int delay_flag;
	int timeout = 0;
	char port_str[10];
	char addr_str[INET6_ADDRSTRLEN];
	char g_line[1024];
	struct addrinfo hints;
	struct addrinfo * servinfo;
	struct addrinfo * it;
	int g_code;
	const char * g_msg;
	char sv_id[1024];
	char * sv_name;
	char * sv_ver;
	char * sv_extra;

	CHECK_CLIENT_PTR(c)

	/* Set the Port String */
	snprintf(port_str, 10, "%hu", port);

	/* Retrieve Server Address Info */
	memset(& hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(addr, port_str, & hints, & servinfo)) != 0)
	{
		SET_ERROR(c, rv, gai_strerror(rv))
		return -1;
	}

	/* Connect to First Available Address */
	for (it = servinfo; it; it = it->ai_next)
	{
		fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (! fd)
			continue;

		rv = connect(fd, it->ai_addr, it->ai_addrlen);
		if (rv == -1)
			continue;

		break;
	}

	if (! it)
	{
		freeaddrinfo(servinfo);
		SET_ERROR(c, ENOTCONN, "Could not connect to server")
		return -1;
	}

	/* Store the Server Address and Port */
	inet_ntop(it->ai_family, get_in_addr((struct sockaddr *)it->ai_addr), addr_str, INET6_ADDRSTRLEN);

	c->s = fd;
	c->timeout = 2000;
	c->s_addr = strdup(addr_str);
	c->s_port = port;

	freeaddrinfo(servinfo);

	/* Set Non-Blocking I/O */
	delay_flag = fcntl(fd, F_GETFL, 0);
	delay_flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, delay_flag);

	/* Read the Greeting Message */
	rv = smartic_socket_readln(c, g_line, 1024, & timeout);
	if (rv == -1)
	{
		smarti_client_disconnect(c);
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Check the Greeting Message */
	rv = smartic_parse_response(g_line, & g_code, & g_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid greeting string received")
		}
		else
		{
			SET_ERROR(c, rv, "Unknown Error")
		}

		smarti_client_disconnect(c);
		return -1;
	}

	/* Check for Ready Status */
	if (g_code != SMARTI_STATUS_READY)
	{
		snprintf(g_server_error, 1024, "%d %s", g_code, g_msg);
		SET_ERROR(c, g_code, g_server_error);
		smarti_client_disconnect(c);
		return -1;
	}

	/* Parse the Greeting Message */
	rv = smartic_parse_greeting(g_msg, & sv_name, & sv_ver, & sv_extra);
	if (rv != 0)
	{
		SET_ERROR(c, EINVAL, "Invalid greeting string received")
		smarti_client_disconnect(c);
		return -1;
	}

	snprintf(sv_id, 1024, "%s %s", sv_name, sv_ver);
	c->s_id = strdup(sv_id);

	free(sv_name);
	free(sv_ver);
	free(sv_extra);

	/* Successfully Connected */
	return 0;
}

int smarti_client_disconnect(smarti_client_t c)
{
	int rv = 0;

	CHECK_CLIENT_PTR(c)

	if (c->s != -1)
		rv = close(c->s);

	if (c->s_id)
		free(c->s_id);
	if (c->s_addr)
		free(c->s_addr);

	c->s = -1;
	c->s_id = 0;
	c->s_addr = 0;
	c->s_port = 0;

	return rv;
}

void smarti_client_dispose(smarti_client_t c)
{
	if (! c)
		return;

	if (c->s != -1)
		smarti_client_disconnect(c);

	if (c->s_id)
		free(c->s_id);
	if (c->s_addr)
		free(c->s_addr);

	free(c);
}

int smarti_client_enumerate(smarti_client_t c, smarti_enum_cb_t cb, void * data)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;

	const char * cmd = "ENUM\n";
	size_t len = strlen(cmd);

	unsigned long old_timeout;

	CHECK_CLIENT_PTR(c)
	CHECK_CLIENT_PTR(cb)

	/* Set Socket Timeout to 10s */
	old_timeout = c->timeout;
	c->timeout = 10000;

	/* Send ENUM Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
	{
		c->timeout = old_timeout;
		return rv;
	}

	/* Read First Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		c->timeout = old_timeout;
		return -1;
	}

	/* Restore Timeout */
	c->timeout = old_timeout;

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for No Devices */
	if (r_code == SMARTI_STATUS_NO_DEVS)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	while (1)
	{
		uint32_t d_addr;
		char * d_name;

		/* Check Return Code */
		if (r_code != SMARTI_STATUS_INFO)
		{
			SET_ERROR(c, rv, r_msg);
			return -1;
		}

		/* Parse Device Information */
		rv = smartic_parse_enuminfo(r_msg, & d_addr, & d_name);
		if (rv != 0)
		{
			SET_ERROR(c, EINVAL, "Invalid greeting string received")
			return -1;
		}

		/* Invoke Callback */
		rv = cb(data, d_addr, d_name);
		if (rv != 0)
		{
			SET_ERROR(c, rv, "Cancelled by User");
			free(d_name);
			return -1;
		}

		free(d_name);

		/* Check for End of Response */
		if (smartic_socket_available(c) == 0)
			break;

		/* Read Next Line */
		rv = smartic_socket_readln(c, r_line, 1024, & timeout);
		if (rv == -1)
		{
			SET_ERROR(c, errno, strerror(errno))
			return -1;
		}

		if (timeout)
		{
			SET_ERROR(c, ETIMEDOUT, "Timed Out")
			return -1;
		}

		/* Parse Line */
		rv = smartic_parse_response(r_line, & r_code, & r_msg);
		if (rv != 0)
		{
			if (rv == 1)
			{
				SET_ERROR(c, errno, strerror(errno))
			}
			else if (rv == 2)
			{
				SET_ERROR(c, EINVAL, "Invalid response string received")
			}
			else
			{
				SET_ERROR(c, rv, r_msg)
			}

			return -1;
		}
	}

	return 0;
}

int smarti_client_errcode(smarti_client_t c)
{
	if (! c)
		return EINVAL;

	return c->errcode;
}

const char * smarti_client_errmsg(smarti_client_t c)
{
	if (! c)
		return 0;

	return c->errmsg;
}

int smarti_client_init()
{
	return 0;
}

int smarti_client_model(smarti_client_t c, uint8_t * model)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;
	char * r_info;

	const char * cmd = "MODEL\n";
	size_t len = strlen(cmd);

	CHECK_CLIENT_PTR(c)
	CHECK_CLIENT_PTR(model)

	/* Send MODEL Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
		return rv;

	/* Read Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for Info */
	if (r_code != SMARTI_STATUS_INFO)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	/* Parse Response */
	rv = smartic_parse_devinfo(r_msg, & r_info);
	if (rv != 0)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	* model = (uint8_t)strtoul(r_info, 0, 10);

	return 0;
}

int smarti_client_open(smarti_client_t c, uint32_t addr, uint32_t lsap, uint8_t chunk_size)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;

	char cmd[1024];
	size_t len;

	CHECK_CLIENT_PTR(c)

	/* Check Chunk Size is 2-32 */
	if (chunk_size < 2)
		chunk_size = 2;
	if (chunk_size > 32)
		chunk_size = 32;

	/* Format OPEN Command */
	rv = snprintf(cmd, 1024, "OPEN %u %u %u\n", addr, lsap, chunk_size);
	if (rv < 0)
		return rv;

	len = strlen(cmd);

	/* Send OPEN Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
		return rv;

	/* Read Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for Success */
	if (r_code != SMARTI_STATUS_SUCCESS)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	return 0;
}

int smarti_client_serial(smarti_client_t c, uint32_t * serial)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;
	char * r_info;

	const char * cmd = "SERIAL\n";
	size_t len = strlen(cmd);

	CHECK_CLIENT_PTR(c)
	CHECK_CLIENT_PTR(serial)

	/* Send SERIAL Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
		return rv;

	/* Read Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for Info */
	if (r_code != SMARTI_STATUS_INFO)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	/* Parse Response */
	rv = smartic_parse_devinfo(r_msg, & r_info);
	if (rv != 0)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	* serial = strtoul(r_info, 0, 10);

	return 0;
}

const char * smarti_client_server_addr(smarti_client_t c)
{
	if (! c)
		return 0;

	return c->s_addr;
}

const char * smarti_client_server_id(smarti_client_t c)
{
	if (! c)
		return 0;

	return c->s_id;
}

uint16_t smarti_client_server_port(smarti_client_t c)
{
	if (! c)
		return 0;

	return c->s_port;
}

int smarti_client_set_token(smarti_client_t c, uint32_t token)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;

	char cmd[1024];
	size_t len;

	CHECK_CLIENT_PTR(c)

	/* Format TOKEN Command */
	rv = snprintf(cmd, 1024, "TOKEN %u\n", token);
	if (rv < 0)
		return rv;

	len = strlen(cmd);

	/* Send TOKEN Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
		return rv;

	/* Read Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for Success */
	if (r_code != SMARTI_STATUS_SUCCESS)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	return 0;
}

int smarti_client_size(smarti_client_t c, uint32_t * size)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;
	char * r_info;

	const char * cmd = "SIZE\n";
	size_t len = strlen(cmd);

	CHECK_CLIENT_PTR(c)
	CHECK_CLIENT_PTR(size)

	/* Send SERIAL Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
		return rv;

	/* Read Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for Info */
	if (r_code != SMARTI_STATUS_INFO)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	/* Parse Response */
	rv = smartic_parse_devinfo(r_msg, & r_info);
	if (rv != 0)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	* size = strtoul(r_info, 0, 10);

	return 0;
}

int smarti_client_ticks(smarti_client_t c, uint32_t * ticks)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;
	char * r_info;

	const char * cmd = "TICKS\n";
	size_t len = strlen(cmd);

	CHECK_CLIENT_PTR(c)
	CHECK_CLIENT_PTR(ticks)

	/* Send SERIAL Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
		return rv;

	/* Read Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		return -1;
	}

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for Info */
	if (r_code != SMARTI_STATUS_INFO)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	/* Parse Response */
	rv = smartic_parse_devinfo(r_msg, & r_info);
	if (rv != 0)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	* ticks = strtoul(r_info, 0, 10);

	return 0;
}

#define isb64(c) ((('A' <= (c)) && ((c) <= 'Z')) || (('a' <= (c)) && ((c) <= 'z')) || (('0' <= (c)) && ((c) <= '9')) || ((c) == '+') || ((c) == '/') || ((c) == '='))

int smarti_client_xfer(smarti_client_t c, void ** buffer, size_t * size)
{
	int rv;
	int timeout = 0;
	char r_line[1024];
	int r_code;
	const char * r_msg;
	char * r_info;
	uint32_t r_len;

	char * b64_data;
	size_t b64_len;
	ssize_t b64_nr;

	unsigned long old_timeout;

	const char * cmd = "XFER\n";
	size_t len = strlen(cmd);

	CHECK_CLIENT_PTR(c)
	CHECK_CLIENT_PTR(buffer)
	CHECK_CLIENT_PTR(size)

	/* Set Socket Timeout to 60s */
	old_timeout = c->timeout;
	c->timeout = 60000;

	/* Send SERIAL Command */
	rv = smartic_socket_write(c, cmd, & len, & timeout);
	if (rv != 0)
	{
		c->timeout = old_timeout;
		return rv;
	}

	/* Read Response Line */
	rv = smartic_socket_readln(c, r_line, 1024, & timeout);
	if (rv == -1)
	{
		SET_ERROR(c, errno, strerror(errno))
		c->timeout = old_timeout;
		return -1;
	}

	if (timeout)
	{
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		c->timeout = old_timeout;
		return -1;
	}

	/* Restore Timeout */
	c->timeout = old_timeout;

	/* Parse Line */
	rv = smartic_parse_response(r_line, & r_code, & r_msg);
	if (rv != 0)
	{
		if (rv == 1)
		{
			SET_ERROR(c, errno, strerror(errno))
		}
		else if (rv == 2)
		{
			SET_ERROR(c, EINVAL, "Invalid response string received")
		}
		else
		{
			SET_ERROR(c, rv, r_msg)
		}

		return -1;
	}

	/* Check for Data */
	if (r_code != SMARTI_DATA_FOLLOWS)
	{
		SET_ERROR(c, r_code, r_msg);
		return -1;
	}

	/* Parse Response */
	rv = smartic_parse_devinfo(r_msg, & r_info);
	if (rv != 0)
	{
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	r_len = strtoul(r_info, 0, 10);

	/* Initialize Data Buffer */
	b64_len = r_len * 2;
	b64_data = (char *)malloc(b64_len * sizeof(char));

	if (! b64_data)
	{
		SET_ERROR(c, ENOMEM, "Failed to allocate data buffer")
		return -1;
	}

	/* Read Data */
	b64_nr = smartic_socket_readln(c, b64_data, b64_len, & timeout);
	if (b64_nr < 0)
	{
		free(b64_data);
		SET_ERROR(c, errno, strerror(errno))
		return -1;
	}

	if (timeout)
	{
		free(b64_data);
		SET_ERROR(c, ETIMEDOUT, "Timed Out")
		c->timeout = old_timeout;
		return -1;
	}

	/* Trim Trailing Invalid Characters */
	while (! isb64(b64_data[b64_nr - 1]))
		b64_data[--b64_nr] = '\0';

	/* Check Data Length */
	if (b64_nr != r_len)
	{
		free(b64_data);
		SET_ERROR(c, EIO, "Data Length Mismatch in XFER")
		return -1;
	}

	/* Base 64 Decode */
	* ((unsigned char **) buffer) = base64_decode(b64_data, b64_nr, size);

	return 0;
}
