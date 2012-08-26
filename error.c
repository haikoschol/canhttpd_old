/*
 * Copyright (c) 2006 Haiko Schol (http://www.haikoschol.com/)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "error.h"
#include "socket_util.h"

static enum loglevel app_loglevel = LOG_INFO;

static void err_doit(int errnoflag, enum loglevel l, const char* fmt, va_list ap);

void set_loglevel(enum loglevel l)
{
	app_loglevel = l;
}

void err_quit(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, LOG_FATAL, fmt, ap);
	va_end(ap);
	exit(ERR_QUIT);
}

void err_sys(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, LOG_SYSTEM, fmt, ap);
	va_end(ap);
	exit(ERR_SYSTEM);
}

void err_log(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, LOG_NONFATAL, fmt, ap);
	va_end(ap);
}

void info_log(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, LOG_INFO, fmt, ap);
	va_end(ap);
}

static void err_doit(int errnoflag, enum loglevel l, const char* fmt, va_list ap)
{
	if (l > app_loglevel)
		return;

	int saved_errno, buflen;
	char buf[MAX_ERROR_LINE];

	saved_errno = errno;
	vsnprintf(buf, sizeof(buf), fmt, ap);
	buflen = strlen(buf);

	if (errnoflag) {
#ifdef WIN32
		snprintf(buf+buflen, sizeof(buf)-buflen, ": %d", WSAGetLastError());
#else
		snprintf(buf+buflen, sizeof(buf)-buflen, ": %s", strerror(saved_errno));
#endif
	}

	strcat(buf, "\n");
	fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
}

