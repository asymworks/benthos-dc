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
 * @file src/smartid/logging.c
 * @brief Smart-I Daemon Logging Utilities
 * @author Jonathan Krauss <jkrauss@asymworks.com>
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "smartid_logging.h"
#include "smartid_version.h"

#define LOG_BUFFER_LEN	2048
#define LOG_BUFFER_FORMAT(ARG) \
	char buf[LOG_BUFFER_LEN]; \
	va_list args; \
	va_start(args, ARG); \
	vsnprintf(buf, LOG_BUFFER_LEN, format, args); \
	va_end(args)

int g_log_use_syslog;
int g_log_use_stderr;
int g_log_debug_level;

void smartid_log_open(void)
{
	openlog(SMARTID_APP_NAME, LOG_NDELAY | LOG_PID, LOG_DAEMON);

	g_log_use_syslog = 1;
	g_log_use_stderr = 0;
	g_log_debug_level = 0;
}

void smartid_log_close(void)
{
	closelog();
}

void smartid_log_use_syslog(int use)
{
	g_log_use_syslog = (use ? 1 : 0);
}

void smartid_log_use_stderr(int use)
{
	g_log_use_stderr = (use ? 1 : 0);
}

void smartid_log_debug_lvl(int lvl)
{
	g_log_debug_level = (lvl ? 1 : 0);
}

void smartid_log_syserror(const char * format, ...)
{
	int ec = errno;

	LOG_BUFFER_FORMAT(format);

	if (g_log_use_syslog)
		syslog(LOG_ERR, "%s (code %d: %s)", buf, ec, strerror(ec));

	if (g_log_use_stderr)
		fprintf(stderr, "[ERROR] %s (code %d: %s)", buf, ec, strerror(ec));
}

void smartid_log_error(const char * format, ...)
{
	LOG_BUFFER_FORMAT(format);

	if (g_log_use_syslog)
		syslog(LOG_ERR, "%s", buf);

	if (g_log_use_stderr)
		fprintf(stderr, "[ERROR] %s\n", buf);
}

void smartid_log_warning(const char * format, ...)
{
	LOG_BUFFER_FORMAT(format);

	if (g_log_use_syslog)
		syslog(LOG_WARNING, "%s", buf);

	if (g_log_use_stderr)
		fprintf(stderr, "[WARNING] %s\n", buf);
}

void smartid_log_info(const char * format, ...)
{
	LOG_BUFFER_FORMAT(format);

	if (g_log_use_syslog)
		syslog(LOG_INFO, "%s", buf);

	if (g_log_use_stderr)
		fprintf(stderr, "[INFO] %s\n", buf);
}

void smartid_log_debug(const char * format, ...)
{
	if (g_log_debug_level)
	{
		LOG_BUFFER_FORMAT(format);

		if (g_log_use_syslog)
			syslog(LOG_DEBUG, "%s", buf);

		if (g_log_use_stderr)
			fprintf(stderr, "[DEBUG] %s\n", buf);
	}
}
