#ifndef _CANBUS_STREUER_H_
#define _CANBUS_STREUER_H_
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
#include "Pcan_usb.h"
#include "error.h"
#include "http.h"

#define sections_000000 0x81
#define sections_111111 0xFF
#define sections_111110 0x83
#define sections_111100 0x87
#define sections_011111 0xC1
#define sections_001111 0xE1
#define sections_000111 0xF1
#define sections_000011 0xF9

int canbus_init();
int canbus_modify_message(http_request* request);
int canbus_send_message();
int canbus_close();
int check_sync();
#endif // header guard
