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
#include "canbus_streuer.h"

static TPCANMsg message;
static int status = sections_000000;
static int border_spreading = 0;
static int sync = 1;
static int bad_sync_count = 0;
int canbus_init()
{
	message.ID = 0x1CE6FFF8;
	message.MSGTYPE = MSGTYPE_EXTENDED;
	message.LEN = 8;
	message.DATA[0] = 0x21;
	message.DATA[1] = 0x00;        /* key identifier */
	message.DATA[2] = 0xFF;
	message.DATA[3] = 0xFF;
	message.DATA[4] = 0xFF;
	message.DATA[5] = 0xFF;
	message.DATA[6] = 0x00;        /* 0x01 - key down, 0x00 - key up */
	message.DATA[7] = 0xFF;

	if (CAN_Init(CAN_BAUD_250K, CAN_INIT_TYPE_EX) != CAN_ERR_OK) {
		err_quit("canbus_init: CAN_Init() failed");
		return 0;
	}
	return 1;
}

void key(unsigned int key_number)
{
	message.DATA[1] = key_number-1;
	canbus_send_message();
}

int close_all_sections()
{
	key(10); // both shutters OFF
	key(3);  // section ON to left side
	key(3);  // section ON to left side
	key(5);  // section ON to right side
	key(5);  // section ON to right side
	return(sections_000000);
}

int open_all_sections()
{
	key(9); // both shutters ON
	key(3); // section ON to left side
	key(3); // section ON to left side
	key(5); // section ON to right side
	key(5); // section ON to right side
	return(sections_111111);
}

int section_1_off()
{
	key(4);
	return(sections_011111);
}

int section_2_off()
{
	key(4);
	return(sections_001111);
}

int left_hyd_shutter_off_sections()
{
	key(4);
	return(sections_000111);
}

int left_hyd_shutter_off()
{
	key(12);
	return(sections_000111);
}

int select_section_5_6_off()
{
	key(10); // close left and right hyd. shutter
	key(5);  // section ON to right side
	key(5);  // section ON to right side
	key(6);  // section OFF from right side
	key(6);  // section OFF from right side
	return(sections_000011);
}

int section_4_after_select()
{
	key(9); // open both hyd. shutters
	return(sections_111100);
}

int section_5_on_to_right()
{
	key(5); // section ON to right side
	return(sections_111110);
}

int parse_http_variables(http_request* request, char** re, char** li)
{
	if ((!request) || (!request->variables) || (!re) || (!li)) {
		err_log("parse_http_variables: got null pointer in request, request->variables, re or li");
		return 0;
	}

	http_request_vars_list* vars = request->variables;

	if (strcmp(vars->name, "Re") != 0) {
		err_log("parse_http_variables: expected http variable 'Re'. got '%s' instead", vars->name);
		return 0;
	}
	*re = vars->value;
	if (!vars->next) {
		err_log("parse_http_variables: expected another http variable");
		return 0;
	}
	vars = vars->next;
	if (strcmp(vars->name, "Li") != 0) {
		err_log("parse_http_variables: expected http variable 'Li'. got '%s' instead", vars->name);
		return 0;
	}
	*li = vars->value;
	return 1;
}

int canbus_modify_message(http_request* request)
{
	if ((!request) || (!request->variables)) {
		err_log("canbus_modify_message: got null pointer in request or request->variables");
		return 0;
	}

	char* re;
	char* li;
	if (!parse_http_variables(request, &re, &li)) {
		err_log("canbus_modify_message: parse_http_variables() failed");
		return 0;
	}

	if (!sync) {
		err_log("canbus_modify_message: out of sync");
	}
	/* border spreading */
	if (sync && (0 == strcmp(li, "%2D1")) && (0 == strcmp(re, "100"))) {   /* %2D1 = -1 urlencoded */
		if (!border_spreading) {
			border_spreading = 1;
			status = open_all_sections();
			key(23);
		}
	/* all sections on */
	} else if (sync && (0 == strcmp(li, "100")) && (0 == strcmp(re, "100"))) {
		if (status != sections_111111) {
			status = open_all_sections();
		}
		if (border_spreading) {
			key(23);
			border_spreading = 0;
		}
	/* section 1 off, rest on */
	} else if (sync && (0 == strcmp(li, "66")) && (0 == strcmp(re, "100"))) {
		if (status != sections_011111) {
			status = section_1_off();
		}
	/* section 1, 2 off, rest on */
	} else if (sync && (0 == strcmp(li, "33")) && (0 == strcmp(re, "100"))) {
		if ((status != sections_001111) && (status == sections_011111)) {
			status = section_2_off();
		}
	/* section 5, 6 select as off */
	} else if (sync && (0 == strcmp(li, "%2D100")) && (0 == strcmp(re, "%2D33"))) {
		if (status != sections_000011) {
			status = select_section_5_6_off();
		}
	/* section 6 off, rest on */
	} else if (sync && (0 == strcmp(li, "100")) && (0 == strcmp(re, "66"))) {
		if (status == sections_111100) {
			status = section_5_on_to_right();
		}
	/* sections 5, 6 off, rest on */
	} else if (sync && (0 == strcmp(li, "100")) && (0 == strcmp(re, "33"))) {
		if (status == sections_000011) {
			status = section_4_after_select();
		}
	/* left hyd. shutter closed, section 1, 2 3 OFF */
	} else if (sync && (0 == strcmp(li, "0")) && (0 == strcmp(re, "100"))) {
		if (status == sections_001111) {
			status = left_hyd_shutter_off_sections();
		}
		if (status == sections_111111) {
			status = left_hyd_shutter_off();
		}
	/* all sections off */
	} else if ((0 == strcmp(li, "0")) && (0 == strcmp(re, "0"))) {
		if (!sync && check_sync()) {
			sync = 1;
			bad_sync_count = 0;
			err_log("canbus_modify_message: back in sync");
		}
		if (status != sections_000000 && status != sections_000011) {
			status = close_all_sections();
		}
		if (border_spreading) {
			key(23);
			border_spreading = 0;
		}
	}
	return 1;
}

void wait_between_messages()
{
	static DWORD last_send = 0;
	if (0 >= last_send) {
		last_send = GetTickCount();
		return;
	}
	DWORD delta = GetTickCount() - last_send;
	if (delta > TIMEOUT_MSEC) {
		last_send = GetTickCount();
		return;
	}
	if (delta < 0) {
		delta = 0;
	}
	DWORD sleep_time = TIMEOUT_MSEC - delta;
	if (sleep_time > 0) {
		Sleep(sleep_time);
	}
	last_send = GetTickCount();
}

int check_sync()
{
	DWORD rc;
	TPCANMsg msg;
	rc = CAN_Read(&msg);
	CAN_ResetClient();
	if (CAN_ERR_OK == rc) {
		return 1;
	}
	bad_sync_count++;
	/* we assume the amatron is switched off and return status and border_spreading to a
	   defined state. when the next message to switch off all sections from the flash
	   animation is received, we are in sync again. */
	if (bad_sync_count > 2) {
		status = sections_000000;
		border_spreading = 0;
		sync = 0;
	}
	return 0;
}

int canbus_send_message()
{
	DWORD rc;
	if (!sync)
		return 1;
	wait_between_messages();
	message.DATA[6] = 0x01;  /* key down */
	rc = CAN_Write(&message);
	if (rc != CAN_ERR_OK) {
		goto error;
	}
	wait_between_messages();
	message.DATA[6] = 0x00;  /* key up */
	rc = CAN_Write(&message);
	if (rc != CAN_ERR_OK) {
		goto error;
	}
	return 1;

error:
	err_log("canbus_send_message: CAN_Write() failed.");
	status = sections_000000;
	border_spreading = 0;
	sync = 0;
	if (CAN_ERR_BUSOFF == rc) {
		canbus_init();
	}
	return 0;
}

int canbus_close()
{
	if (CAN_Close() != CAN_ERR_OK) {
		err_log("canbus_close: CAN_Close() failed");
		return 0;
	}
	return 1;
}
