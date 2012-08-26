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


#include "socket_util.h"
#include "error.h"
#include "canbus.h"

int init_sockets()
{
/*	if (!canbus_init())
		err_log("init_all: canbus_init() failed"); */
#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,1), &wsaData) != 0)
		err_sys("could not initialize Winsock");
#endif
	return 1;
}

void cleanup_all()
{
	if (!canbus_close())
		err_log("cleanup_all: canbus_close() failed");
#ifdef WIN32
	WSACleanup();
#endif
}

#ifdef WIN32
int inet_pton(int address_family, const char* address_string, void* destination)
{
	if (address_family == AF_INET) {
		unsigned long in_val;

		if ( (in_val = inet_addr(address_string)) != INADDR_NONE) {
			memcpy(destination, &in_val, sizeof(struct in_addr));
			return 1;
		}
		return 0;
	}
	return -1;
}
#endif

unsigned short parse_port(const char* port_string)
{
	long sl;
	char* end_ptr;

	sl = strtol(port_string, &end_ptr, 10);

	if ((errno == ERANGE) || (sl > 65535) || (sl < 0))
		err_quit("illegal value for listen port: %s", port_string);

	return (unsigned short) sl;
}

SOCKET create_tcp_server_socket(const char* listen_address,
                                const unsigned short listen_port,
                                struct sockaddr_in* servaddr)
{
	if (!listen_address || !servaddr)
		err_quit("create_tcp_server_socket: got null pointer in listen_address or servaddr");

	SOCKET sockfd;

	memset(servaddr, 0, sizeof(*servaddr));
	servaddr->sin_family = AF_INET;
	servaddr->sin_port = htons(listen_port);

	if ("" == listen_address)
	{
		inet_pton(AF_INET, "127.0.0.1", &servaddr->sin_addr);
	} else
	{
		if (inet_pton(AF_INET, listen_address, &servaddr->sin_addr) <= 0)
			err_sys("create_tcp_server_socket: could not parse ip address %s", listen_address);
	}

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys("create_tcp_server_socket: could not create socket");

	if (bind(sockfd, (SOCKADDR*) servaddr, sizeof(*servaddr)) < 0)
		err_sys("create_tcp_server_socket: could not bind socket to port %u", listen_port);

	if (listen(sockfd, LISTENQ) < 0)
		err_sys("create_tcp_server_socket: could not listen to port %u", listen_port);

	return sockfd;
}

void Send(SOCKET s, const char* data, size_t len)
{
	size_t bytes_remaining = len;
	size_t offset = 0;
	ssize_t res = 0;
	while (bytes_remaining) {
		res = send(s, data+offset, bytes_remaining, 0);
		if (-1 == res) {
			err_sys("Send: send() failed");
			return;
		}
		offset += res;
		bytes_remaining -= res;
	}
}

void Close(SOCKET s)
{
#ifdef WIN32
	if (SOCKET_ERROR == closesocket(s))
		err_sys("Close: closesocket failed");
#else
	if (-1 == close(s))
		err_sys("Close: close failed");
#endif
}
