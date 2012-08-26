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


#include <string.h>
#include <time.h>
#include "socket_util.h"
#include "server.h"
#include "error.h"
#include "client_list.h"
#include "http.h"
#include "canbus.h"

static SOCKET global_server_socket;
static client_list* clients;
static fd_set read_set;
static fd_set test_set;
static unsigned int maxfdp1;

void set_timeout(struct timeval* timeout, unsigned int delta)
{
	timeout->tv_sec = 0;
	timeout->tv_usec = (TIMEOUT_MSEC * 1000) - delta;
	//info_log("timeout is set to: %d", timeout->tv_usec / 1000);
}

void update_maxfdp1()
{
	if ((!clients) || (!clients->head)) {
		err_log("update_maxfdp1: got null pointer in clients or clients->head");
		return;
	}

	maxfdp1 = global_server_socket + 1;

	if (0 == clients->count)
		return;

	http_client* node = clients->head->next;

	while (node) {
		if (node->sock+1 > maxfdp1) {
			maxfdp1 = node->sock + 1;
		}
		node = node->next;
	}
}

void accept_connection(SOCKET server_socket)
{
	SOCKADDR addr;
	socklen_t len = sizeof(addr);
	SOCKET new_sock;

#ifdef WIN32
	new_sock = SOCKET_ERROR;
#endif
	new_sock = accept(server_socket, &addr, &len);
#ifdef WIN32
	if (SOCKET_ERROR == new_sock)
		err_sys("accept_connection: accept() failed");
#else
	if (-1 == new_sock)
		err_sys("accept_connection: accept() failed");
#endif
	if (clients->count == MAXCLIENTS) {
		Close(new_sock);
		info_log("Maximum number of client connections reached. closing new connection.");
		return;
	}

	http_client* new_client = add_client(clients, new_sock);
	if (!new_client)
		err_quit("accept_connection: add_client() failed");

	FD_SET(new_sock, &read_set);
	if (new_sock+1 > maxfdp1) {
		maxfdp1 = new_sock + 1;
	}
}

void close_connection(http_client* client)
{
	if (!client) {
		err_log("close_connection: got null pointer in client");
		return;
	}

	FD_CLR(client->sock, &read_set);
	Close(client->sock);

	if (!del_client(clients, client->sock))
		err_log("close_connection: del_client() failed");

	update_maxfdp1();
}

void add_data_to_client_recvbuf(http_client* client, const char* recvbuf)
{
	if ((!client) || (!recvbuf)) {
		err_log("add_data_to_client_recvbuf: got null pointer in client or recvbuf");
		return;
	}

	unsigned int buflen = strlen(recvbuf);
	unsigned int client_buflen = strlen(client->recvbuf);

	if (buflen > strlen(client->currpos)) {
		unsigned int offset = client->currpos - client->recvbuf;
		char* new_recvbuf = realloc(client->recvbuf, 2 * sizeof(char) * (client_buflen + buflen));
		if (!new_recvbuf)
			err_sys("add_data_to_client_recvbuf: realloc() failed");

		client->recvbuf = new_recvbuf;
		client->currpos = client->recvbuf + offset;
	}

	strcpy(client->currpos, recvbuf);
	client->currpos += buflen;
}

void receive_data()
{
	if ((!clients) || (!clients->head)) {
		err_log("receive_data: got null pointer in clients or clients->head or clients->head->next");
		return;
	}
	if (0 == clients->count) {
		err_log("receive_data: called but clients->count == 0");
		return;
	}
	http_client* node = clients->head->next;
	char recvbuf[MAXLINE+1];
	ssize_t bytes_received;

	while (1) {
		if (!FD_ISSET(node->sock, &test_set)) {
			goto next;
		}
		bytes_received = recv(node->sock, recvbuf, MAXLINE, 0);

		if (-1 == bytes_received)
			err_sys("receive_data: recv() failed");
		if (0 == bytes_received) {
			close_connection(node);
			goto next;
		}
		recvbuf[bytes_received] = '\0';
		add_data_to_client_recvbuf(node, recvbuf);
		goto next;

next:
		if (!node->next) {
			break;
		} else {
			node = node->next;
		}
	}
}

void handle_http()
{
	if ((!clients) || (!clients->head)) {
		err_log("handle_http: got null pointer in clients or clients->head or clients->head->next");
		return;
	}
	if (0 == clients->count) {
		err_log("handle_http: called but clients->count == 0");
		return;
	}
	http_client* node = clients->head->next;
	while (node) {
		if (strlen(node->recvbuf) < 4) {
			node = node->next;
			continue;
		}
		if (0 == strcmp(node->currpos-4, "\r\n\r\n")) {
			http_request* request;
			if (parse_http_request(node, &request)) {
				handle_http_request(node, request);
				delete_http_request(request);
			}
			close_connection(node);
		}
		node = node->next;
	}
}

void run_server_loop(SOCKET server_socket, struct sockaddr_in servaddr)
{
	struct timeval timeout;
	DWORD delta;
	DWORD before;
	DWORD after;
	int num_ready;

	global_server_socket = server_socket;
	clients = create_client_list();
	maxfdp1 = server_socket + 1;

	FD_ZERO(&read_set);
	FD_ZERO(&test_set);
	FD_SET(server_socket, &read_set);

	if (!canbus_init())
		err_log("run_server_loop: canbus_init() failed");

	do
	{
		before = GetTickCount();
		set_timeout(&timeout, 0);
		test_set = read_set;
		num_ready = select(maxfdp1, &test_set, NULL, NULL, &timeout);

		if (-1 == num_ready)
			err_sys("select failed");

		if (FD_ISSET(server_socket, &test_set)) {
			accept_connection(server_socket);
			num_ready--;
		}

		after = GetTickCount();
#ifdef SPRITZE
		delta = after - before;
		if ((delta > 0) && (delta < TIMEOUT_MSEC)) {
			Sleep(TIMEOUT_MSEC - delta);
		}
		canbus_send_message();
# else
		delta += after - before;
		if (delta >= 600) {  /* check every x ms, whether amatron is switched off */
			check_sync();
			delta = 0;
		}
#endif
		if (0 == num_ready) {
			continue;
		}

		receive_data();
		handle_http();
	} while (1);
}
