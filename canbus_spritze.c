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


#include <windows.h>
#include <stdio.h>
#include "error.h"
#include "http.h"
#include "canbus_spritze.h"

static TPCANMsg message;

int canbus_init()
{
	message.ID = 0x18E6FF33;
	message.MSGTYPE = MSGTYPE_EXTENDED;
	message.LEN = 8;
	message.DATA[0] = 0x21;
	message.DATA[1] = 0xFA;
	message.DATA[2] = 0x00;        /* contains state information */
	message.DATA[3] = 0x80;
	message.DATA[4] = 0x00;
	message.DATA[5] = 0x00;
	message.DATA[6] = 0x01;
	message.DATA[7] = 0x0D;

	if (CAN_Init(CAN_BAUD_250K, CAN_INIT_TYPE_EX) != CAN_ERR_OK) {
		err_quit("canbus_init: CAN_Init() failed");
		return 0;
	}
	return 1;
}

int canbus_modify_message(http_request* request)
{
	if ((!request) || (!request->variables)) {
		err_log("canbus_modify_message: got null pointer in request or request->variables");
		return 0;
	}

	http_request_vars_list* vars = request->variables;
	int current;
	unsigned int values_read = 0;
	message.DATA[2] = 0x00;

	do {
		if (0 == sscanf(vars->value, "%1d", &current)) {
			err_log("canbus_modify_message: sscanf() failed");
			return 0;
		}
		if (current > 1) {
			err_log("canbus_modify_message: unexpected value in http request variable");
			return 0;
		}
		current <<= values_read++;
		message.DATA[2] |= current;
		vars = vars->next;
	} while (vars && (values_read < 5));

	if (values_read < 5) {
		err_log("canbus_modify_message: got less http get variables than expected");
		return 0;
	}
	return 1;
}

int canbus_send_message()
{
	if (CAN_Write(&message) != CAN_ERR_OK) {
		err_log("canbus_send_message: CAN_Write() failed");
		return 0;
	}
	return 1;
}

int canbus_close()
{
	if (CAN_Close() != CAN_ERR_OK) {
		err_log("canbus_close: CAN_Close() failed");
		return 0;
	}
	return 1;
}
