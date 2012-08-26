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


#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include "http.h"
#include "client_list.h"
#include "error.h"
#include "canbus.h"

int parse_http_request_method(char** request_line, http_request* request)
{
	if ((!request_line) || (!*request_line) || (!request))
		return 0;

	char* p = *request_line;
	char* method = p;

	while (p && !isspace(*p))
		p++;

	if (!p)
		return 0;

	unsigned int len = p - method;
	*request_line += len + 1;                /* skip blank after method */

	if (0 == strncmp(method, "GET", len)) {
		request->method = GET;
		return 1;
	}
	if (0 == strncmp(method, "POST", len)) {
		request->method = POST;
		return 1;
	}
	if (0 == strncmp(method, "HEAD", len)) {
		request->method = HEAD;
		return 1;
	}
	if (0 == strncmp(method, "PUT", len)) {
		request->method = PUT;
		return 1;
	}
	if (0 == strncmp(method, "DELETE", len)) {
		request->method = METHOD_DELETE;
		return 1;
	}
	if (0 == strncmp(method, "TRACE", len)) {
		request->method = TRACE;
		return 1;
	}
	if (0 == strncmp(method, "CONNECT", len)) {
		request->method = CONNECT;
		return 1;
	}
	if (0 == strncmp(method, "OPTIONS", len)) {
		request->method = OPTIONS;
		return 1;
	}
	request->method = UNKNOWN;
	return 1;
}

int parse_http_request_path(char** recvbuf, http_request* request)
{
	if ((!recvbuf) || (!*recvbuf) || (!request))
		return 0;

	char* p = *recvbuf;
	char* path = p;

	while (p && (*p != '?') && !isspace(*p))
		p++;

	if (!p)
		return 0;

	unsigned int len = p - path;
	request->path = malloc(sizeof(char) * (len+1));
	if (!request->path) {
		err_sys("parse_http_request_path: malloc() failed");
		return 0;
	}
	strncpy(request->path, path, len);
	request->path[len] = '\0';

	*recvbuf += len; /* _don't_ skip the '?', that's how presence of request variables is detected later on */
	return 1;
}

void http_request_add_variable(http_request* request, char* name, char* value)
{
	if ((!request) || (!name) || (!value)) {
		err_log("http_request_add_variable: got null pointer in request, name or value");
		return;
	}

	http_request_vars_list* new_entry = malloc(sizeof(http_request_vars_list));
	if (!new_entry) {
		err_sys("http_request_add_variable: malloc() failed");
		return;
	}

	new_entry->name = name;
	new_entry->value = value;
	new_entry->next = 0;

	if (!request->variables) {
		request->variables = new_entry;
		return;
	}

	http_request_vars_list* node = request->variables;
	while (node->next)
		node = node->next;
	node->next = new_entry;
	return;
}

int parse_http_request_variables(char** recvbuf, http_request* request)
{
	if ((!recvbuf) || (!*recvbuf) || (!request))
		return 0;

	if (**recvbuf != '?')
		return 1;       /* no request variables, but also no error */

	char* p = *recvbuf + 1; /* skip '?' */
	char* name = p;
	char* value;

	if (!p)
		return 0;       /* premature end of request line (no http version string) */

	if (isspace(*p))	/* i.e. "GET /index.html? HTTP/1.1" is considered a valid request line */
		return 1;

	while (p) {
		if ('=' == *p) {
			unsigned int len = (p > name) ? (p - name) : 0;
			char* tmpname = malloc(sizeof(char) * (len+1));
			if (!tmpname) {
				err_sys("parse_http_request_variables: malloc() failed");
				return 0;
			}
			strncpy(tmpname, name, len);
			tmpname[len] = '\0';
			value = p + 1;
			while (p && (*p != '&') && !isspace(*p))
				p++;
			if (!p) {                /* premature end of request line (no http version string) */
				free(tmpname);
				return 0;
			}
			len = (p > value) ? (p - value) : 0;
			char* tmpvalue = malloc(sizeof(char) * (len+1));
			if (!tmpvalue) {
				err_sys("parse_http_request_variables: malloc() failed");
				return 0;
			}
			strncpy(tmpvalue, value, len);
			tmpvalue[len] = '\0';
			http_request_add_variable(request, tmpname, tmpvalue);
			if (isspace(*p))
				break;
			name = p + 1;
		} else if (isspace(*p)) {
			break;
		}
		p++;
	}
	if (!p)
		return 0;
	*recvbuf = p;
	return 1;
}

int parse_http_request_header(char** recvbuf, http_request* request)
{
	/* ***FIXME*** not really necessary, we only need the request vars */
	return 1;
}

int parse_http_request_uri(char** recvbuf, http_request* request)
{
	if (!parse_http_request_method(recvbuf, request)) {
		info_log("parse_http_request_method failed");
		return 0;
	}

	if (!parse_http_request_path(recvbuf, request)) {
		info_log("parse_http_request_path failed");
		return 0;
	}

	if (!parse_http_request_variables(recvbuf, request))
		return 0;

	return 1;
}

int delete_http_name_value_pair_list(struct http_name_value_pair_list_t* list)
{
	if (!list)
		return 0;

	int valid = 1;

	struct http_name_value_pair_list_t* node = list;
	struct http_name_value_pair_list_t* delete_me;

	while (node) {
		if (node->name) {
			free(node->name);
		} else {
			valid = 0;
		}
		if (node->value) {
			free(node->value);
		} else {
			valid = 0;
		}
		delete_me = node;
		node = node->next;
		free(delete_me);
	}

	return valid;
}

int delete_http_request_vars(http_request_vars_list* request_vars)
{
	return delete_http_name_value_pair_list(request_vars);
}

int delete_http_request_header(http_header_list* request_header)
{
	return delete_http_name_value_pair_list(request_header);
}

void delete_http_request(http_request* request)
{
	int valid = 1;

	if (!request) {
		err_log("delete_http_request: got null pointer in request");
		return;
	}

	if ((!request->path) || (!request->version)) {
		valid = 0;
		info_log("request->path or request->version is null");
	}

	if (request->variables != NULL) {
		if (!delete_http_request_vars(request->variables)) {
			valid = 0;
			info_log("request->variables were screwed up");
		}
	}

	if (request->headers != NULL) {
		if (!delete_http_request_header(request->headers)) {
			valid = 0;
		}
	}

	if (!valid) {
		err_log("delete_http_request: request structure was malformed. got illegal null pointer");
	}
}

int parse_http_request(http_client* client, http_request** request)
{
	if ((!client) || (!client->recvbuf) || (!request)) {
		err_log("parse_http_request: got null pointer in client, client->recvbuf or request");
		return 0;
	}

	*request = malloc(sizeof(struct http_request_t));
	if (!*request) {
		err_sys("parse_http_request: malloc() failed");
		return 0;
	}

	(*request)->path = 0;
	(*request)->version = 0;
	(*request)->variables = 0;
	(*request)->headers = 0;

	client->currpos = client->recvbuf;
	char* unparsed_recvbuf = client->currpos;

	if (!parse_http_request_uri(&unparsed_recvbuf, *request)) {
		goto cleanup;
	}

	if (!parse_http_request_header(&unparsed_recvbuf, *request)) {
		info_log("parse_http_request_header failed");
		goto cleanup;
	}

	/* ***FIXME*** */
	char* version = malloc(sizeof(char) * (strlen("HTTP/1.1")+1));
	strcpy(version, "HTTP/1.1");
	(*request)->version = version;
	return 1;

cleanup:
	delete_http_request(*request);
	return 0;
}

char* sanitize_path(const char* path)
{
	if (!path) {
		err_log("sanitize_path: got null pointer in path");
		return NULL;
	}

	char* sanitized_path = (char*) path;
	char* p;

	do {
		p = strstr(sanitized_path, "..");
		if (p && (p != sanitized_path)) {
			sanitized_path = p;
		}
	} while (p);

	if (!sanitized_path || (0 == strlen(sanitized_path)))
		return NULL;

	return sanitized_path;
}

void send_file(http_client* client, http_request* request, FILE* f)
{
	if ((!client) || (!request) || (!f)) {
		err_log("send_file: got null pointer in client, request or f");
		return;
	}

	char buf[FILE_READ_BUFFER_SIZE];
	size_t thingies_read = 0;

	while (!feof(f)) {
		if (ferror(f)) {
			err_log("send_file: error reading file");
			return;
		}
		thingies_read = fread(buf, sizeof(char), sizeof(buf)/sizeof(char) - 1, f);
		buf[thingies_read] = '\0';
		Send(client->sock, buf, strlen(buf));
	}
}

void handle_http_get_request(http_client* client, http_request* request)
{
	if ((!client) || (!request)) {
		err_log("handle_http_get_request: got null pointer in client or request");
		return;
	}

	char* sanitized_path = sanitize_path(request->path+1); /* skip '/' at beginning of path ***FIXME*** */
	if (!sanitized_path) {
		err_log("handle_http_get_request: illegal path: %s", request->path);
	}

	if (0 == strcmp(sanitized_path, MAGIC_FILE_NAME)) {
		if (!canbus_modify_message(request))
			err_log("handle_http_get_request: canbus_modify_message() failed");
		char response_header[] = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
		Send(client->sock, response_header, strlen(response_header));
#ifdef SPRITZE
		canbus_send_message();
#endif
		return;
	}

	/* ***FIXME*** use absolute path, webserver root directory, etc. */
	FILE* f = fopen(sanitized_path, "r");
	if (!f) {
		char msg[] = "HTTP/1.1 404 File Not Found\r\n\r\n"
			     "<html><title>404</title><body><h1>"
			     "404 File Not Found</h1></body></html>\r\n";
		Send(client->sock, msg, strlen(msg));
		return;
	}

	struct stat file_info;
	if (-1 == (fstat(fileno(f), &file_info))) {
		fclose(f);
		err_sys("handle_http_get_request: fstat() failed");
		return;
	}

	char response_header[MAX_HTTP_RESPONSE_LENGTH+1] = "HTTP/1.1 200 OK\r\nConnection: close\r\n";
	char length[255];

	snprintf(length, sizeof(length), "Content-Length: %d\r\n", (int)file_info.st_size);
	strncat(response_header, length, sizeof(response_header)-strlen(response_header)-1);
	strncat(response_header, "Content-Type: application/xml\r\n\r\n",
		sizeof(response_header)-strlen(response_header)-1);
	response_header[MAX_HTTP_RESPONSE_LENGTH] = '\0';

	Send(client->sock, response_header, strlen(response_header));
	send_file(client, request, f);
	fclose(f);		/* checking return value is pointless, since if fclose() fails, */
				/* we can't do anything useful about it. */
}

void handle_not_implemented_http_request(http_client* client, http_request* request)
{
	char msg[] = "HTTP/1.1 501 Method Not Implemented\r\n\r\n"
		     "<html><title>501</title><body><h1>"
		     "501 Method Not Implemented</h1></body></html>\r\n";
	Send(client->sock, msg, strlen(msg));
}

void handle_http_request(http_client* client, http_request* request)
{
	if (GET == request->method) {
		handle_http_get_request(client, request);
	} else if (UNKNOWN == request->method) {
		handle_not_implemented_http_request(client, request);
	} else {
		handle_not_implemented_http_request(client, request);
	}
/*	info_log("path: %s", request->path);
	if (!request->variables)
		return;
	http_request_vars_list* vars = request->variables;
	while (vars) {
		info_log("%s=%s", vars->name, vars->value);
		vars = vars->next;
	}*/
}
