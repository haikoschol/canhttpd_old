#ifndef _HTTP_H_
#define _HTTP_H_
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

#define MAX_HTTP_RESPONSE_LENGTH 4096
#define FILE_READ_BUFFER_SIZE 1024

/* if this file is requested and the right get variables are supplied, a message will be send over
 * canbus, instead of serving a file with that name over the tcp connection. */
#define MAGIC_FILE_NAME "proxy.html"

enum http_request_method { OPTIONS, GET, HEAD, POST, PUT, METHOD_DELETE, TRACE, CONNECT, UNKNOWN };

struct http_name_value_pair_list_t
{
	char* name;
	char* value;
	struct http_name_value_pair_list_t* next;
};

typedef struct http_name_value_pair_list_t http_request_vars_list;
typedef struct http_name_value_pair_list_t http_header_list;

struct http_request_t
{
	enum http_request_method method;
	char* path;
	char* version;
	http_request_vars_list* variables;
	http_header_list* headers;
};

typedef struct http_request_t http_request;

int parse_http_request(http_client* client, http_request** request);
void handle_http_request(http_client* client, http_request* request);
void delete_http_request(http_request* request);
#endif // header guard
