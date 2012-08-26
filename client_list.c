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


#include "client_list.h"
#include "error.h"

client_list* create_client_list()
{
	http_client* head = malloc(sizeof(struct client_list_node_t));
	if (!head)
		err_sys("create_list: malloc failed");

	client_list* result = malloc(sizeof(struct client_list_t));
	if (!result)
		err_sys("create_list: malloc failed");

	result->head = head;
	result->count = 0;
	result->head->next = 0;
	return result;
}

http_client* add_client(client_list* clients, SOCKET sock)
{
	if ((!clients) || (!clients->head)) {
		err_log("add_client: got null pointer in clients or clients->head");
		return NULL;
	}

	http_client* node = clients->head;

	while (node->next)
		node = node->next;

	http_client* new_node = malloc(sizeof(struct client_list_node_t));
	if (!new_node)
		err_sys("add_client: malloc failed");

	new_node->recvbuf = malloc(sizeof(char) * (MAXLINE+1));
	if (!new_node->recvbuf)
		err_sys("add_client: malloc failed");

	new_node->currpos = new_node->recvbuf;
	new_node->sock = sock;
	new_node->next = 0;
	node->next = new_node;
	clients->count++;
	return new_node;
}

http_client* get_client(client_list* clients, SOCKET sock)
{
	if ((!clients) || (!clients->head)) {
		err_log("get_client: got null pointer in clients or clients->head");
		return NULL;
	}

	http_client* node = clients->head;

	while (node->next) {
		if (node->sock == sock) {
			return node;
		}
	}

	return NULL;
}

int del_client(client_list* clients, SOCKET sock)
{
	if ((!clients) || (!clients->head)) {
		err_log("del_client: got null pointer in clients or clients->head");
		return 0;
	}

	if (0 == clients->count)
		return 0;

	http_client* node = clients->head->next;
	http_client* prev = clients->head;

	do {
		if (node->sock == sock) {
			http_client* delete_me = node;
			prev->next = node->next;
			clients->count--;

			if (delete_me->recvbuf != NULL)
				free(delete_me->recvbuf);

			free(delete_me);
			return 1;
		}
		node = node->next;
		prev = prev->next;
	} while (node);
	return 0;
}
