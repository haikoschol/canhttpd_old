#ifndef _UNP_H
#define _UNP_H
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


/* header fuer die socket api auf unix systemen */
#ifndef WIN32
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <unistd.h>

#define SOCKET int
#define SOCKADDR struct sockaddr
#else
#define __USE_W32_SOCKETS 1
#include <windows.h>
#define socklen_t int
#define ssize_t int
#endif

#define MAXLINE 4096
#define MAXCLIENTS 5
#define LISTENQ 5

#ifdef SPRITZE
#define TIMEOUT_MSEC 700
#else
#define TIMEOUT_MSEC 50
#endif

int init_sockets();
void cleanup_all();
#ifdef WIN32
int inet_pton(int address_family, const char* address_string, void* destination);
#endif
unsigned short parse_port(const char* port_string);
SOCKET create_tcp_server_socket(const char* listen_address,
                                const unsigned short listen_port,
                                struct sockaddr_in* addr);
void Send(SOCKET s, const char* data, size_t len);
void Close(SOCKET s);
#endif // header guard




