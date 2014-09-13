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
 * @file src/smartid/smartid_server.c
 * @brief Smart-I Daemon Control Utilities
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>			// inet_ntop

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>

#include "smartid_conn.h"
#include "smartid_logging.h"
#include "smartid_server.h"
#include "smartid_version.h"

/* Global Event Loop Pointer */
static struct event_base * g_evloop = 0;

/* Global Connection Event Pointer */
struct event * g_evt_connect = 0;

/* Server Connection Callback */
static void serv_connect(int listener, short event, void * arg)
{
	struct event_base * evbase = (struct event_base *)arg;

	int fd;
	char addr_str[INET6_ADDRSTRLEN];
	void * addr_ptr;

	struct sockaddr_storage ss;
	socklen_t slen = sizeof(struct sockaddr_storage);

	smarti_conn_t conn;

	/* Accept the Connection */
	fd = accept(listener, (struct sockaddr *)(& ss), & slen);
	if (fd < 0)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			smartid_log_error("Error accepting connection (code %d: %s)", errno, strerror(errno));
			return;
		}

		return;
	}

	/* Get the Client IP Address */
	if (ss.ss_family == AF_INET)
	{
		addr_ptr = & ((struct sockaddr_in *)(& ss))->sin_addr;
	}
	else
	{
		addr_ptr = & ((struct sockaddr_in6 *)(& ss))->sin6_addr;
	}

	inet_ntop(ss.ss_family, addr_ptr, addr_str, INET6_ADDRSTRLEN);
	smartid_log_info("Got connection from %s", addr_str);

	/* Make the Socket Non-Blocking */
	if (evutil_make_socket_nonblocking(fd))
	{
		smartid_log_error("Failed to set non-blocking I/O on socket");
		close(fd);
		return;
	}

	/* Create Connection State */
	conn = smartid_conn_alloc(fd, addr_str, evbase);
	if (! conn)
	{
		close(fd);
		return;
	}
}

int smartid_init(void)
{
	struct event_base * evloop;

	/* Setup LibEvent */
	evloop = event_base_new();
	if (! evloop)
	{
		smartid_log_error("event_base_new() failed");
		return -1;
	}

	g_evloop = evloop;
	smartid_log_debug("Using libevent backend %s", event_base_get_method(evloop));

	return 0;
}

void smartid_cleanup(void)
{
	/* Cleanup Connection Resources */
	smartid_conn_cleanup();

	/* Close Connection Event */
	if (g_evt_connect)
	{
		event_del(g_evt_connect);
		event_free(g_evt_connect);
		g_evt_connect = 0;
	}

	/* Release Event Loop */
	if (g_evloop)
	{
		event_base_free(g_evloop);
		g_evloop = 0;
	}
}

int smartid_start(int listenfd, const char * saddr, int sport)
{
	int rv;
	struct event_base * evloop;


	/* Make the Socket Non-Blocking */
	rv = evutil_make_socket_nonblocking(listenfd);
	if (rv != 0)
	{
		smartid_log_error("Failed to make socket non-blocking (code %d: %s)", errno, strerror(errno));
		close(listenfd);
		event_base_free(g_evloop);
		return rv;
	}

	/* Listen on the Listener Socket */
	rv = listen(listenfd, 8);
	if (rv == -1)
	{
		smartid_log_error("Failed to listen on %s port %d (code %d: %s)", saddr, sport, errno, strerror(errno));
		close(listenfd);
		event_base_free(g_evloop);
		return rv;
	}

	/* Setup the Connection Event */
	g_evt_connect = event_new(g_evloop, listenfd, EV_READ | EV_PERSIST, serv_connect, (void *)g_evloop);
	if (! g_evt_connect)
	{
		smartid_log_error("Failed to create event for listener socket");
		close(listenfd);
		event_base_free(g_evloop);
		return -1;
	}

	rv = event_add(g_evt_connect, NULL);
	if (rv != 0)
	{
		smartid_log_error("Failed to create event for listener socket");
		event_free(g_evt_connect);
		close(listenfd);
		event_base_free(g_evloop);
		return -1;
	}

	smartid_log_info("%s is active", SMARTID_APP_TITLE);

	/* Run the Event Loop */
	rv = event_base_dispatch(g_evloop);
	if (rv != 0)
	{
		smartid_log_error("Failed to run event loop");
		return rv;
	}

	/* Successful Exit */
	return 0;
}

int smartid_stop(int sig)
{
	int rv;

	smartid_log_debug("sigterm_handler(%d)", sig);

	rv = event_base_loopexit(g_evloop, 0);
	if (rv != 0)
	{
		smartid_log_error("Failed to exit from event loop");
	}

	return rv;
}
