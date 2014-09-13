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

/**
 * @file smarti_conn.c
 * @brief Smart-I Daemon Connection Handling
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>				// INET6_ADDRSTRLEN

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>

#include "base64.h"
#include "irda.h"
#include "smarti_codes.h"
#include "smartid_cmd.h"
#include "smartid_conn.h"
#include "smartid_device.h"
#include "smartid_logging.h"
#include "smartid_version.h"

/**
 * Connection State Structure
 *
 * Includes the basic state of a connection, including the network and IrDA
 * connection sockets, client IP, I/O buffer, and hooks to create a linked
 * list for multiple connections.
 *
 * Allocate this structure with smarti_conn_alloc() and free by calling
 * smarti_conn_dispose().
 */
struct smarti_conn_t_
{
	int			net_fd;						///< Network Socket File Descriptor
	int			shutdown;					///< Whether the socket has shut down
	char 		addr[INET6_ADDRSTRLEN];		///< Client IP Address

	irda_t					irda;			///< IrDA Socket Handle
	smart_device_t			dev;			///< Smart Device Handle

	struct event_base *		ev_loop;		///< Server Event Loop
	struct evbuffer *		ev_buffer;		///< Connection Event Buffer
	struct bufferevent *	ev_evt;			///< Connection Buffer Event

	struct smarti_conn_t_ *	prev;			///< Previous Connection in List
	struct smarti_conn_t_ *	next;			///< Next Connection in List
};

//! Smart-I Client Connection List
static struct smarti_conn_t_	g_conn_head = { .next = 0 };
static smarti_conn_t 			g_conn_list = & g_conn_head;

// Add a Connection to the Connection List
void conn_add(smarti_conn_t c)
{
	if (! c)
	{
		smartid_log_error("Invalid smarti_conn_t passed to conn_add()");
		return;
	}

	c->prev = g_conn_list;
	c->next = g_conn_list->next;

	if (g_conn_list->next)
	{
		g_conn_list->next->prev = c;
	}

	g_conn_list->next = c;
}

// Shutdown a Connection
void conn_shutdown(smarti_conn_t c)
{
	if (! c)
	{
		smartid_log_error("Invalid smarti_conn_t passed to conn_shutdown()");
		return;
	}

	if (! c->shutdown && (shutdown(c->net_fd, SHUT_RDWR) != 0))
	{
		smartid_log_error("Failed to shut down socket for %s", c->addr);
	}

	c->shutdown = 1;
}

// Connection Read Data Callback
static void conn_read(struct bufferevent * e, void * arg)
{
	smarti_conn_t conn = (smarti_conn_t)(arg);
	char * cmd_line;
	size_t cmd_len;

	if (! conn)
	{
		smartid_log_error("Null argument passed to conn_read()");
		return;
	}

	if (! conn->shutdown)
	{
		cmd_line = evbuffer_readln(bufferevent_get_input(e), & cmd_len, EVBUFFER_EOL_ANY);
		if (cmd_line)
		{
			smartid_log_debug("Received '%s' from %s", cmd_line, conn->addr);
			smartid_cmd_process(conn, cmd_line, cmd_len);
			free(cmd_line);
		}

		smartid_conn_flush(conn);
	}
}

// Connection Socket Event Callback
static void conn_event(struct bufferevent * e, short err, void * arg)
{
	smarti_conn_t conn = (smarti_conn_t)(arg);

	if (! conn)
	{
		smartid_log_error("Null argument passed to conn_err()");
		return;
	}

	// Log the Error
	if (err & BEV_EVENT_EOF)
	{
		smartid_log_info("Remote host at %s disconnected", conn->addr);
		conn->shutdown = 1;
	}
	else if (err & BEV_EVENT_TIMEOUT)
	{
		smartid_log_warning("Remote host at %s timed out", conn->addr);
	}
	else if (err & BEV_EVENT_ERROR)
	{
		smartid_log_error("Error on connection to Remote Host at %s: 0x%hx", conn->addr, err);
	}
	else
	{
		return;
	}

	// Close the Connection
	smartid_conn_dispose(conn);
}

smarti_conn_t smartid_conn_alloc(int sockfd, const char * client_addr, struct event_base * evloop)
{
	int rv;
	smarti_conn_t ret;
	struct timeval	tv_read = { .tv_sec = 300, .tv_usec = 0 };

	/* Allocate a new Connection Instance */
	ret = (smarti_conn_t)malloc(sizeof(struct smarti_conn_t_));
	if (! ret)
	{
		smartid_log_error("Failed to allocate memory for smarti_conn_t");
		return 0;
	}

	/* Setup the New Instance */
	memset(ret, 0, sizeof(struct smarti_conn_t_));
	ret->net_fd = sockfd;
	ret->irda = 0;
	ret->dev = 0;
	ret->ev_loop = evloop;
	strncpy(ret->addr, client_addr, INET6_ADDRSTRLEN);

	/* Create the IrDA Socket */
	rv = irda_socket_open(& ret->irda);
	if (rv != 0)
	{
		smartid_log_error("Failed to open IrDA handle (code %d: %s)", irda_errcode(), irda_errmsg());
		smartid_conn_dispose(ret);
		return 0;
	}

	/* Set IrDA Socket Timeout */
	rv = irda_socket_set_timeout(ret->irda, 2000);
	if (rv != 0)
	{
		smartid_log_error("Failed to set IrDA socket timeout (code %d: %s)", irda_errcode(), irda_errmsg());
		smartid_conn_dispose(ret);
		return 0;
	}

	/* Add to the Global Connection List */
	conn_add(ret);

	/* Setup the Input Buffer Event */
	ret->ev_evt = bufferevent_socket_new(evloop, sockfd, 0);
	if (! ret->ev_evt)
	{
		smartid_log_error("Failed to allocate bufferevent for %s", client_addr);
		smartid_conn_dispose(ret);
		return 0;
	}

	/* Setup the Input Buffer Callbacks */
	bufferevent_setcb(ret->ev_evt, conn_read, 0, conn_event, ret);
	bufferevent_set_timeouts(ret->ev_evt, & tv_read, 0);

	if (bufferevent_enable(ret->ev_evt, EV_READ) != 0)
	{
		smartid_log_error("Failed to enable buffered I/O for %s", client_addr);
		smartid_conn_dispose(ret);
		return 0;
	}

	/* Setup the Output Buffer */
	ret->ev_buffer = evbuffer_new();
	if (! ret->ev_buffer)
	{
		smartid_log_error("Failed to enable buffered I/O for %s", client_addr);
		smartid_conn_dispose(ret);
		return 0;
	}

	/* Send a Welcome Message */
	smartid_conn_send_ready(ret);
	smartid_conn_flush(ret);

	return ret;
}

void smartid_conn_dispose(smarti_conn_t c)
{
	if (! c)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_dispose()");
		return;
	}

	/* Remove the Connection from the List */
	if (c->prev->next == c)
	{
		c->prev->next = c->next;
	}
	else
	{
		smartid_log_warning("Bug: Socket list is inconsistent (c->prev->next != c)");
	}

	if (c->next)
	{
		if (c->next->prev == c)
		{
			c->next->prev = c->prev;
		}
		else
		{
			smartid_log_warning("Bug: Socket list is inconsistent (c->next->prev != c)");
		}
	}

	/* Free Connection Resources */
	if (c->dev)
		smartid_dev_dispose(c->dev);

	if (c->ev_buffer)
		evbuffer_free(c->ev_buffer);
	if (c->ev_evt)
		bufferevent_free(c->ev_evt);

	if (c->irda)
		irda_socket_close(c->irda);

	if (c->net_fd != -1)
	{
		conn_shutdown(c);
		close(c->net_fd);
	}

	/* Free Connection */
	free(c);
}

void smartid_conn_cleanup(void)
{
	while (g_conn_list->next)
		smartid_conn_dispose(g_conn_list->next);
}

void smartid_conn_send_response(smarti_conn_t conn, uint16_t code, const char * msg)
{
	if (! conn)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_send_response()");
		return;
	}

	if (evbuffer_add_printf(conn->ev_buffer, "%d %s\n", code, msg) < 0)
	{
		smartid_log_error("Failed to send data to %s", conn->addr);
	}
}

void smartid_conn_send_responsef(smarti_conn_t conn, uint16_t code, const char * msg, ...)
{
	va_list args;
	char buf[2048];

	if (! conn)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_send_responsef()");
		return;
	}

	va_start(args, msg);
	vsnprintf(buf, 2047, msg, args);
	va_end(args);

	smartid_conn_send_response(conn, code, buf);
}

void smartid_conn_send_buffer(smarti_conn_t conn, void * data, size_t len)
{
	char * b64data;
	char * b64line;
	size_t b64len;

	if (! conn)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_send_buffer()");
		return;
	}

	if (! data || ! len)
	{
		smartid_log_warning("Null buffer passed to smartid_conn_send_buffer()");
		return;
	}

	b64data = base64_encode((const unsigned char *)data, len, & b64len);
	if (! b64data)
	{
		smartid_log_error("Failed to encode data to base64");
		return;
	}

	smartid_conn_send_responsef(conn, SMARTI_DATA_FOLLOWS, "Data Follows: %u", b64len);

	b64line = (char *)malloc(b64len + 1);
	b64line = strncpy(b64line, b64data, b64len);
	b64line[b64len] = 0;

	evbuffer_add_printf(conn->ev_buffer, "%s\n", b64line);

	free(b64line);
}

void smartid_conn_flush(smarti_conn_t conn)
{
	if (! conn)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_flush()");
		return;
	}

	if (bufferevent_write_buffer(conn->ev_evt, conn->ev_buffer) != 0)
	{
		smartid_log_error("Failed to send data to client at %s", conn->addr);
	}
}

const char * smartid_conn_client(smarti_conn_t conn)
{
	if (! conn)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_client()");
		return 0;
	}

	return conn->addr;
}

irda_t smartid_conn_irda(smarti_conn_t conn)
{
	if (! conn)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_irda()");
		return 0;
	}

	return conn->irda;
}

smart_device_t smartid_conn_device(smarti_conn_t conn)
{
	if (! conn)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_device()");
		return 0;
	}

	return conn->dev;
}

void smartid_conn_set_device(smarti_conn_t conn, smart_device_t dev)
{
	if (! conn)
	{
		smartid_log_error("Invalid smarti_conn_t passed to smartid_conn_set_device()");
		return;
	}

	conn->dev = dev;
}
