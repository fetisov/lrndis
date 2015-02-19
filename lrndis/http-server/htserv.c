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

#include "htserv.h"

static struct tcp_pcb *http_pcb = NULL;

htserv_on_req_t htserv_on_req = NULL;
htserv_on_con_t htserv_on_con = NULL;
htserv_on_err_t htserv_on_err = NULL;

typedef enum htcon_state_priv
{
	CSP_ACTIVE,  // as CON_ACTIVE
	CSP_CLOSING, // as CON_CLOSING
	CSP_CLOSED,  // as CON_CLOSED
	CSP_RD_REQ,
	CSP_WR_RESP,
	CSP_NONE,
} htcon_state_priv_t;

typedef struct htcon_priv
{
	htcon_t            htcon;      // user connection struct
	char buff[HTTP_CON_BUFF_SIZE]; // request/response buffer
	htcon_state_priv_t state;      // connection state
	struct tcp_pcb    *pcb;        // connection pcb
	http_reqb_t        reqb;       // request buffer
	http_resp_t        resp;       // responce
	// writing managment
	char              *wbuff;      // writing buffer
	int                wsize;      // writing buffer size
	int                writed;     // writed to buffer
	int                wsended;    // sended by tcp
	int                wacksize;   // acknowledged
} htcon_priv_t;

htcon_priv_t htcons[HTTP_SERVER_MAX_CON];

void htserv_io(void);

void http_prepare_resp(http_resp_t *resp, int code)
{
	resp->code = code;
	resp->server = HTTP_SERVER_NAME;
	resp->cont_lang = HTTP_DEF_CONT_LANG;
	resp->mime = MIME_TEXT_HTML;
	resp->cont_len = 0;
	resp->conn_type = CT_CLOSE;
}

void htcon_req_finished(htcon_priv_t *con)
{
	// 1. check request
	if (con->reqb.req.method != METHOD_GET)
	{
		//http_prepare_resp(&con->resp, 405); // Method Not Allowed
		//con->state = CSP_WR_RESP;
		con->state = CSP_CLOSING;
		htserv_io();
		return;
	}
	// 2. ask user
	if (htserv_on_req == NULL)
	{
		con->state = CSP_CLOSING;
		htserv_io();
		return;
	}

	http_prepare_resp(&con->resp, 200);

	if (htserv_on_req(&con->reqb.req, &con->resp, &con->htcon.arg))
	{
		int len;
		con->wbuff = con->buff + con->reqb.size;
		con->wsize = HTTP_CON_BUFF_SIZE - con->reqb.size;
		len = http_resp_str(&con->resp, con->wbuff, con->wsize);
		if (len >= con->wsize)
		{
			if (htserv_on_err != NULL)
				htserv_on_err(HTTP_RESP_NOMEM);
			con->state = CSP_CLOSING;
		}
		else
		{
			con->state = CSP_WR_RESP;
			con->wacksize = 0;
			con->writed = len;
			con->wsended = 0;
		}
	}
	else
		con->state = CSP_CLOSING;

	htserv_io();
}

void htcon_received(htcon_priv_t *con, const char *data, int size)
{
	htserv_err_t err;
	err = HTTP_NO_ERROR;

	if (con->state == CSP_RD_REQ)
	{
		http_reqb_push(&con->reqb, data, size);
		switch (con->reqb.state)
		{
		case REQB_UNFINISHED:
			return;
		case REQB_FINISHED:
			htcon_req_finished(con);
			return;
		case REQB_REQ_TO_BIG:
			err = HTTP_BIG_REQUEST;
			break;
		case REQB_SYNT_ERROR:
			err = HTTP_REQ_SYNT_ERR;
			break;
		}
	}
	if (err != HTTP_NO_ERROR)
	{
		if (htserv_on_err != NULL)
			htserv_on_err(err);
		con->htcon.state = CON_CLOSING;
		con->state = CSP_CLOSING;
		htserv_io();
	}
}

err_t htcon_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) 
{
	htcon_priv_t *con;

	con = (htcon_priv_t *)arg;

	if (err == ERR_OK && p != NULL)
	{
		tcp_recved(pcb, p->tot_len);
		if (con != NULL)
			htcon_received(con, p->payload, p->tot_len);
	}

	if (err == ERR_OK && p == NULL)
	{
		if (con != NULL)
			if (con->state != CSP_CLOSED)
			{
				con->state = CSP_CLOSING;
				con->htcon.state = CON_CLOSING;
			}
	}

	if (p != NULL)
		pbuf_free(p);

	return ERR_OK;
}

static int find_free_con(void)
{
	int i;
	for (i = 0; i < HTTP_SERVER_MAX_CON; i++)
		if (htcons[i].state == CSP_NONE) return i;
	return -1;
}

err_t htcon_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	htcon_priv_t *con;
	con = (htcon_priv_t *)arg;
	if (con == NULL) return ERR_OK;
	con->wacksize += len;
//	if (con->state == CSP_ACTIVE)
//		con->htcon.writed += len;
	return ERR_OK;
}

void htcon_err(void *arg, err_t err)
{
	htcon_priv_t *con;
	con = (htcon_priv_t *)arg;
	if (con == NULL) return;
	if (htserv_on_err != NULL)
		htserv_on_err(HTTP_TRANSP_ERR);
	con->htcon.state = CON_CLOSING;
	con->state = CSP_CLOSING;
}

err_t htserv_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	int index;
	htcon_priv_t *con;

	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	index = find_free_con();
	if (index < 0) return ERR_MEM;
  con = &htcons[index];

	con->pcb = newpcb;
	http_reqb_init(&con->reqb, con->buff, HTTP_REQ_MAX_SIZE);
	con->wbuff = NULL;
	con->wsize = 0;
	con->wsended = 0;
	con->wacksize = 0;
	con->state = CSP_RD_REQ;
	
	con->htcon.req = &con->reqb.req;
	con->htcon.resp = &con->resp;
	con->htcon.writed = 0;
	con->htcon.arg = NULL;


	tcp_setprio(newpcb, TCP_PRIO_MIN);
	tcp_arg(newpcb, con);
	tcp_recv(newpcb, htcon_recv);
	tcp_err(newpcb, htcon_err);
	tcp_sent(newpcb, htcon_sent);
	tcp_poll(newpcb, NULL, 4); // No polling here

	return ERR_OK;
}

void htserv_io(void) // state machine
{
	for (int i = 0; i < HTTP_SERVER_MAX_CON; i++)
	{
		htcon_priv_t *con;
		con = &htcons[i];
		if (con->state == CSP_NONE || con->state == CSP_CLOSED)
			continue;

		// have data to send?
		if (con->state != CSP_CLOSING && con->writed > con->wsended)
		{
			int count;
			err_t err;
			count = con->writed - con->wsended;
			if (count > tcp_sndbuf(con->pcb))
				count = tcp_sndbuf(con->pcb);
			err = tcp_write(con->pcb, con->wbuff + con->wsended, count, TCP_WRITE_FLAG_COPY);
			if (err == ERR_OK)
			{
				con->wsended += count;
				tcp_output(con->pcb);
			}
		}

		// data was successfully sended?
		if (con->state != CSP_CLOSING && con->wacksize >= con->wsended && con->wsended > 0)
		{
			con->writed = 0;
			con->wsended = 0;
			con->wacksize = 0;
			// is it http-response?
			if (con->state == CSP_WR_RESP)
			{
				con->htcon.state = CON_ACTIVE;
				con->state = CSP_ACTIVE;
				if (htserv_on_con != NULL)
					htserv_on_con(i);
			}
			else
			// is it document?
			if (con->state == CSP_ACTIVE && con->resp.conn_type != CT_KEEP_ALIVE)
				if (con->htcon.writed >= con->resp.cont_len)
				{
					con->htcon.state = CON_CLOSING;
					con->state = CSP_CLOSING;
				}
		}

		// closing connection?
		if (con->state == CSP_CLOSING)
			// if (tcp_close(con->pcb) == ERR_OK)
			{
				con->htcon.state = CON_CLOSED;
				con->state = CSP_CLOSED;
				con->pcb = NULL;
			}
	}
}

err_t htserv_init(uint16_t port)
{
	int i;
	err_t err;
	tcp_init();
	htserv_free();
	if (http_pcb != NULL)
		return ERR_USE;
	http_pcb = tcp_new();
	if (http_pcb == NULL) return ERR_MEM;
	err = tcp_bind(http_pcb, IP_ADDR_ANY, port);
	if (err != ERR_OK)
	{
		htserv_free();
		return err;
	}
	for (i = 0; i < HTTP_SERVER_MAX_CON; i++)
		htcons[i].state = CSP_NONE;
	return ERR_OK;
}

void htserv_free(void)
{
	if (http_pcb == NULL) return;
	if (tcp_close(http_pcb) == ERR_OK)
		http_pcb = NULL;
}

void htserv_tmr(uint32_t time_ms)
{
	http_pcb = tcp_listen(http_pcb);
	tcp_accept(http_pcb, htserv_accept);
	htserv_io();
}

const htcon_t *htcon(int index)
{
	if (index < 0 || index >= HTTP_SERVER_MAX_CON)
		return NULL;
	if (htcons[index].state > CSP_CLOSED) return NULL;
	return &htcons[index].htcon;
}

void htcon_close(int index)
{
	htcon_priv_t *con;
	con = (htcon_priv_t *)htcon(index);
	if (con == NULL) return;
	if (con->state == CSP_CLOSED) return;
	con->htcon.state = CON_CLOSING;
	con->state = CSP_CLOSING;
	htserv_io();
}

void htcon_free(int index)
{
	htcon_priv_t *con;
	con = (htcon_priv_t *)htcon(index);
	if (con == NULL) return;
	if (con->state != CSP_CLOSED) return;
	htcons[index].state = CSP_NONE;
}

int htcon_write_avail(int index)
{
	htcon_priv_t *con;
	con = (htcon_priv_t *)htcon(index);
	if (con == NULL) return 0;
	if (con->state != CSP_ACTIVE) return 0;
	return con->wsize - con->writed;
}

int htcon_write(int index, const char *data, int size)
{
	int res;
	htcon_priv_t *con;
	con = (htcon_priv_t *)htcon(index);
	if (con == NULL) return 0;
	if (con->state != CSP_ACTIVE) return 0;
	res = con->resp.cont_len - con->htcon.writed;
	if (size < res) res = size;
	if (con->wsize - con->writed < res)
		res = con->wsize - con->writed;
	if (res <= 0) return 0;
	memcpy(con->wbuff + con->writed, data, res);
	con->writed += res;
	con->htcon.writed += res;
	htserv_io();
	return res;
}
