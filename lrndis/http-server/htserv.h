/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 by Sergey Fetisov <fsenok@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * version: 1.0 demo (7.02.2015)
 * brief:   tiny http ipv4 server using lwip (pcb)
 */

#ifndef HTSERV_H
#define HTSERV_H

#include <stdlib.h>
#include <stdio.h>
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/tcp_impl.h"
#include "lwip/tcp.h"
#include "http_req.h"

// server configuration

#define HTTP_SERVER_NAME    "lrndis" // http server name
#define HTTP_SERVER_MAX_CON 3        // max connections number at one time
#define HTTP_CON_BUFF_SIZE  2048     // size of con buffer, used for request/response data storing
#define HTTP_REQ_MAX_SIZE   1024     // max part of con buffer for a request storing
#define HTTP_DEF_CONT_LANG  "en"     // content language which will be sent to client by default

// http server types

typedef enum htserv_err
{
	HTTP_NO_ERROR,
	HTTP_REQ_SYNT_ERR, // http request synt error
	HTTP_BIG_REQUEST,  // http request size more then HTTP_CON_BUFF
	HTTP_RESP_NOMEM,   // http response size more then HTTP_CON_BUFF
	HTTP_TRANSP_ERR    // http transport error (client disconnected?)
} htserv_err_t;

typedef bool (*htserv_on_req_t)(const http_req_t *req, http_resp_t *resp, void **arg);
typedef void (*htserv_on_con_t)(int index);
typedef void (*htserv_on_err_t)(htserv_err_t err);

typedef enum htcon_state
{
	CON_ACTIVE,
	CON_CLOSING,
	CON_CLOSED
} htcon_state_t;

typedef struct htcon
{
	void              *arg;
	http_req_t        *req;
	http_resp_t       *resp;
	htcon_state_t      state;
	int                writed;
} htcon_t;

extern htserv_on_req_t htserv_on_req;
extern htserv_on_con_t htserv_on_con;
extern htserv_on_err_t htserv_on_err;

err_t          htserv_init(uint16_t port);
void           htserv_free(void);
void           htserv_tmr(uint32_t time_ms);
const htcon_t *htcon(int index);
void           htcon_close(int con);
void           htcon_free(int con);
int            htcon_write_avail(int con);
int            htcon_write(int con, const char *data, int size);

#endif
