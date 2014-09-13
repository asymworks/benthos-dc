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
 * @file src/smartid/smartid_main.c
 * @brief Smart-I Daemon Main Entry Point
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <event2/event.h>

#include "irda.h"
#include "smartid_logging.h"
#include "smartid_server.h"
#include "smartid_version.h"

/**@{
 * @name Global Variables
 */
int		g_debug;		///< Debugging level
int		g_ipv4;			///< Bind to IPv4 Addresses Only
int		g_ipv6;			///< Bind to IPv6 Addresses Only
int		g_fork;			///< Fork and become a daemon
char *	g_pidfile;		///< PID File for daemon
int		g_port;			///< TCP Port for Smart-I (default in smartid_version.h)
/*@}*/

void sigterm_handler(int sig)
{
	smartid_stop(sig);
}

void sigchld_handler(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

int smartid_setup_signals()
{
	struct sigaction act_exit;
	struct sigaction act_chld;

	memset(& act_exit, 0, sizeof(struct sigaction));
	act_exit.sa_handler = sigterm_handler;
	act_exit.sa_flags = 0;
	sigemptyset(& act_exit.sa_mask);

	if (sigaction(SIGTERM, & act_exit, NULL) == -1)
	{
		smartid_log_syserror("Failed to initialize handler for SIGTERM");
		return 1;
	}

	if (sigaction(SIGINT, & act_exit, NULL) == -1)
	{
		smartid_log_syserror("Failed to initialize handler for SIGINT");
		return 1;
	}

	memset(& act_chld, 0, sizeof(struct sigaction));
	act_chld.sa_handler = sigchld_handler;
	act_chld.sa_flags = SA_RESTART;
	sigemptyset(& act_chld.sa_mask);

	if (sigaction(SIGCHLD, & act_chld, NULL) == -1)
	{
		smartid_log_syserror("Failed to initialize handler for SIGCHLD");
		return 1;
	}

	return 0;
}

void smartid_usage()
{
	fprintf(stderr, "Usage: %s [-46Ddhv] [-p PORT] [--pidfile PIDFILE]\n", SMARTID_APP_NAME);
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -4                : only bind to IPv4 addresses (default)\n");
	fprintf(stderr, "  -6                : only bind to IPv6 addresses\n");
	fprintf(stderr, "  -D                : do not detach and become a daemon\n");
	fprintf(stderr, "  -d                : log debugging information\n");
	fprintf(stderr, "  -h|--help         : show program options (this screen)\n");
	fprintf(stderr, "  -p|--port PORT    : define the TCP port on which to listen (default: %d)\n", SMARTID_PORT);
	fprintf(stderr, "  -v|--version      : show version number\n");
	fprintf(stderr, "  --pidfile PIDFILE : write the daemon PID to the specified file (default: %s)\n", SMARTID_PIDFILE);
}

void smartid_version()
{
	printf("Server version: %s/%s (%s)\n", SMARTID_APP_TITLE, SMARTID_VERSION_STRING, "RPi");
	printf("Server built:   %s %s\n", __DATE__, __TIME__);
	printf("Copyright:      %s\n", SMARTID_COPYRIGHT);
}

static struct option g_options[] =
{
	{ "help",		no_argument,		0,	'h' },
	{ "version",	no_argument,		0,	'v' },
	{ "pidfile",	required_argument,	0,	1	},
	{ "port",		required_argument,	0,	'p' },
	{ 0,			0,					0,	0 }
};

int smartid_parse_args(int argc, char ** argv, int * exit)
{
	int ch;
	int port_arg = -1;
	int ip4_flag = 0;
	int ip6_flag = 0;
	int show_help = 0;
	int show_version = 0;

	/* Initialize Defaults */
	g_debug = 0;
	g_fork = 1;
	g_ipv4 = 1;
	g_ipv6 = 0;
	g_pidfile = strdup(SMARTID_PIDFILE);
	g_port = SMARTID_PORT;

	/* Parse Arguments */
	while ((ch = getopt_long(argc, argv, "46Ddp:hv", g_options, 0)) != -1)
	{
		switch (ch)
		{
		case 1:
			g_pidfile = strdup(optarg);
			break;

		case '4':
			ip4_flag = 1;
			break;

		case '6':
			ip6_flag = 1;
			break;

		case 'D':
			g_fork = 0;
			break;

		case 'd':
			g_debug = 1;
			break;

		case 'p':
			if (sscanf(optarg, "%d", & port_arg) != 1)
			{
				fprintf(stderr, "Invalid port specified: %s\n", optarg);
				smartid_usage();
				if (exit) * exit = 1;
				return 1;
			}
			break;

		case 'h':
			smartid_usage();
			if (exit) * exit = 1;
			return 0;

		case 'v':
			smartid_version();
			if (exit) * exit = 1;
			return 0;

		case '?':
			/* getopt_long already printed an error message */
			smartid_usage();
			if (exit) * exit = 1;
			return 1;

		default:
			/* something weird happened */
			fprintf(stderr, "Error in getopt_long()\n");
			return -1;

		}
	}

	/* Check for Unexpected Arguments */
	if (optind < argc)
	{
		fprintf(stderr, "Unexpected arguments: ");
		while (optind < argc)
			fprintf(stderr, "%s ", argv[optind++]);
		fprintf(stderr, "\n");

		smartid_usage();
		if (exit) * exit = 1;
		return 1;
	}

	/* Handle Arguments */
	if (ip4_flag && ip6_flag)
	{
		fprintf(stderr, "Error: Both -4 and -6 may not be specified\n");
		smartid_usage();
		if (exit) * exit = 1;
		return 1;
	}

	if (ip4_flag)
	{
		g_ipv4 = 1;
		g_ipv6 = 0;
	}

	if (ip6_flag)
	{
		g_ipv4 = 0;
		g_ipv6 = 1;
	}

	if (port_arg != -1)
	{
		if (port_arg < 0 || port_arg > 65535)
		{
			fprintf(stderr, "Invalid port specified: %d", port_arg);
		}

		g_port = port_arg;
	}

	/* Setup Logging */
	smartid_log_use_syslog(  g_fork);
	smartid_log_use_stderr(! g_fork);
	smartid_log_debug_lvl(g_debug);

	/* Success */
	if (exit) * exit = 0;
	return 0;
}

int smartid_run()
{
	int rv;

	char port_str[10];
	char addr_str[INET6_ADDRSTRLEN];
	void * addr_ptr;

	struct addrinfo hints;
	struct addrinfo * servinfo;
	struct addrinfo * it;
	static int yes = 1;

	int listenfd;

	/* Log Initialization */
	smartid_log_info("%s is initializing", SMARTID_APP_TITLE);

	/* Setup Signal Handlers */
	if (smartid_setup_signals())
		return -1;

	/* Setup LibEvent */
	smartid_log_debug("Using libevent version %s", event_get_version());

	/* Initialize the Daemon */
	rv = smartid_init();
	if (rv != 0)
	{
		smartid_log_error("smartid_init() failed with code %d", rv);
		return rv;
	}


	/* Get Host Addresses */
	memset(& hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	snprintf(port_str, 10, "%d", g_port);

	smartid_log_debug("Using port %s", port_str);
	if ((rv = getaddrinfo(NULL, port_str, & hints, & servinfo)) != 0)
	{
		smartid_log_error("getaddrinfo() failed (code %d: %s)", rv, gai_strerror(rv));
		return -1;
	}

	/* Walk Addresses and Bind the First Available */
	for (it = servinfo; it != NULL; it = it->ai_next)
	{
		/* Check Address Family */
		if ((it->ai_family == AF_INET) && ! g_ipv4)
			continue;
		if ((it->ai_family == AF_INET6) && ! g_ipv6)
			continue;

		/* Translate the Address into Text (for messages) */
		if (it->ai_family == AF_INET)
			addr_ptr = & ((struct sockaddr_in *)it->ai_addr)->sin_addr;
		else
			addr_ptr = & ((struct sockaddr_in6 *)it->ai_addr)->sin6_addr;

		inet_ntop(it->ai_family, addr_ptr, addr_str, INET6_ADDRSTRLEN);

		/* Create the Socket */
		listenfd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (listenfd == -1)
		{
			smartid_log_warning("Failed to open socket for %s (code %d: %s)", addr_str, errno, strerror(errno));
			continue;
		}

		/* Set the Socket Options */
		rv = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, & yes, sizeof(int));
		if (rv == -1)
		{
			smartid_log_error("Failed to set socket option SO_REUSEADDR for %s (code %d: %s)", addr_str, errno, strerror(errno));
			close(listenfd);
			freeaddrinfo(servinfo);
			smartid_cleanup();
			return -1;
		}

		/* Bind the Socket to the IP Address and TCP Port */
		rv = bind(listenfd, it->ai_addr, it->ai_addrlen);
		if (rv == -1)
		{
			smartid_log_warning("Failed to bind to %s (code %d: %s)", addr_str, errno, strerror(errno));
			close(listenfd);
			continue;
		}

		/* Successfully Bound Socket */
		smartid_log_info("Bound to %s on port %s", addr_str, port_str);
		break;
	}

	if (! it)
	{
		smartid_log_error("Unable to bind to a service port");
		freeaddrinfo(servinfo);
		smartid_cleanup();
		return -1;
	}

	freeaddrinfo(servinfo); // free the linked list

	/* Start the Server */
	rv = smartid_start(listenfd, addr_str, g_port);

	/* Shut Down */
	smartid_log_info("%s is shutting down", SMARTID_APP_TITLE);

	smartid_cleanup();
	close(listenfd);

	return 0;
}

int main(int argc, char ** argv)
{
	int exit = 0;
	int rv;

	/* Setup Logging */
	smartid_log_open();

	/* Process Options */
	rv = smartid_parse_args(argc, argv, & exit);
	if (exit || (rv != 0))
	{
		return rv;
	}

	/* Daemonize */
	if (g_fork)
	{
		FILE * fp_pid;
		pid_t pid = 0;
		pid_t sid = 0;

		/* Fork the Process */
		pid = fork();
		if (pid < 0)
		{
			fprintf(stderr, "Error: fork() failed (code %d: %s)", errno, strerror(errno));
			return 1;
		}

		if (pid > 0)
		{
			/* Kill the Parent Process */
			return 0;
		}

		/* Setup the Daemon */
		umask(0);
		chdir("/");
		pid = getpid();
		sid = setsid();
		if (sid < 0)
		{
			smartid_log_syserror("setsid() failed in child");
			return 1;
		}

		/* Write the PID to disk */
		fp_pid = fopen(g_pidfile, "w");
		if (! fp_pid)
		{
			smartid_log_syserror("fopen() failed for pid file '%s'", g_pidfile);
			return 1;
		}

		fprintf(fp_pid, "%lu", pid);
		fflush(fp_pid);
		fclose(fp_pid);

		/* Close Standard I/O */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

	/* Setup IrDA */
	rv = irda_init();
	if (rv != 0)
	{
		smartid_log_error("Failed to initialize IrDA (code %d: %s)", irda_errcode(), irda_errmsg());
		return rv;
	}

	/* Initialize and Run Smart-I Daemon */
	smartid_log_info("%s/%s (%s)", SMARTID_APP_NAME, SMARTID_VERSION_STRING, SMARTID_MACHINE);
	rv = smartid_run();
	irda_cleanup();
	smartid_log_info("%s is exiting (code %d)", SMARTID_APP_TITLE, rv);

	return rv;
}
