/*
 * Copyright (C) 2015 by Sergey Fetisov <fsenok@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
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
#define HTTP_SERVER_MAX_CON 2        // max connections number at one time
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
