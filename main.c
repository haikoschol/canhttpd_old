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
#include "server.h"
#include "error.h"

int main(int argc, char* argv[])
{
	SOCKET server_socket;
	char* listen_address = "";
	unsigned short listen_port;
	struct sockaddr_in servaddr;

	if (argc < 2) {
		listen_port = parse_port("80");
	} else {
		listen_port = parse_port(argv[1]);
	}

	if (argc > 2)
		listen_address = argv[2];

	init_sockets();
	atexit(cleanup_all);
	server_socket = create_tcp_server_socket(listen_address, listen_port, &servaddr);
	run_server_loop(server_socket, servaddr);
	return 0;
}
