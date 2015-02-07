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
 * brief:   part of tiny http ipv4 server using lwip (pcb)
 */

#include "http_req.h"

#ifdef __cplusplus
extern "C" {
#endif

const char MIME_TEXT_HTML[] =   "text/html";
const char MIME_TEXT_JS[] =     "text/javascript";
const char MIME_TEXT_PLAIN[] =  "text/plain";
const char MIME_TEXT_XML[] =    "text/xml";
const char MIME_TEXT_CSS[] =    "text/css";
const char MIME_IMAGE_GIF[] =   "image/gif";
const char MIME_IMAGE_JPEG[] =  "image/jpeg";
const char MIME_IMAGE_PJPEG[] = "image/pjpeg";
const char MIME_IMAGE_PNG[] =   "image/png";
const char MIME_IMAGE_SVG[] =   "image/svg+xml";
const char MIME_IMAGE_TIFF[] =  "image/tiff";
const char MIME_IMAGE_ICON[] =  "image/vnd.microsoft.icon";
const char MIME_IMAGE_WBMP[] =  "image/vnd.wap.wbmp";

static const char *METHODS_STR[] =
{
	"",
	"GET",
	"POST",
	"HEAD",
	"PUT",
	"CONNECT",
	"OPTIONS",
	"DELETE",
	"TRACE",
	"PATCH"
};

static const char *CONN_TYPE_STR[] =
{
	"", "close", "keep-alive"
};

void http_reqb_init(http_reqb_t *reqb, void *buff, int size)
{
	memset(reqb, 0, sizeof(http_reqb_t));
	reqb->buff = buff;
	reqb->bsize = size;
}

int http_reqb_avail(const http_reqb_t *reqb)
{
	return reqb->bsize - reqb->size;
}

static chatset_t req_ident =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, // '%', '-', '.', '/'
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // '0'...'9'
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 'A'...'O'
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, // 'P'...'Z', '_'
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 'a'...'z'
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, // 'p'...'z'
};

static http_mt_t str_to_method(const char *str)
{
	http_mt_t res;
	for (res = METHOD_GET; res <= METHOD_PATCH; res++)
		if (strcmp(METHODS_STR[res], str) == 0)
			return res;
	return METHOD_NONE;
}

static http_ct_t str_to_ct(const char *str)
{
	if (strcmp(str, CONN_TYPE_STR[CT_CLOSE]) == 0) return CT_CLOSE;
	if (strcmp(str, CONN_TYPE_STR[CT_KEEP_ALIVE]) == 0) return CT_KEEP_ALIVE;
	return CT_NONE;
}

void http_req_set_param(http_req_t *req, char *name, char *val)
{
	if (strcmp(name, "Host") == 0)
		req->host = val;
	else
	if (strcmp(name, "User-Agent") == 0)
		req->user_agent = val;
	else
	if (strcmp(name, "Content-Type") == 0)
		req->mime = val;
	else
	if (strcmp(name, "Content-Length") == 0)
		req->cont_len = strtol(val, NULL, 10);
	else
	if (strcmp(name, "Accept") == 0)
		req->accept = val;
	else
	if (strcmp(name, "Accept-Language") == 0)
		req->accept_lang = val;
	else
	if (strcmp(name, "Accept-Encoding") == 0)
		req->accept_enc = val;
	else
	if (strcmp(name, "Cookie") == 0)
		req->cookie = val;
	else
	if (strcmp(name, "Connection") == 0)
		req->conn_type = str_to_ct(val);
	else
	if (strcmp(name, "Keep-Alive") == 0)
		req->keep_alive = strtol(val, NULL, 10);
}

void http_reqb_push(http_reqb_t *rb, const void *data, int size)
{
	parser_t p;
	int indeces[4];

	if (rb->size + size >= rb->bsize)
	{
		rb->state = REQB_REQ_TO_BIG;
		return;
	}
	if (rb->state != REQB_UNFINISHED) return;

	memcpy(&rb->buff[rb->size], data, size);
	rb->size += size;

	parser_init(&p, rb->buff, rb->size);
	parser_skip_n(&p, rb->parsing.pos);

	while (!parser_eof(&p))
	{
		int total;
		switch (rb->parsing.state)
		{
		case 0:
			parser_skip_ws(&p);
			total = p.total;
			parser_skip_before(&p, ' ');
			if (parser_eof(&p)) break;
			rb->buff[p.total] = 0;
			rb->req.method = str_to_method(&rb->buff[total]);
			parser_skip_n(&p, 1);
			rb->parsing.pos = p.total;
			rb->parsing.state = 1;
			break;
		case 1:
			parser_skip_ws(&p);
			total = p.total;
			parser_skip(&p, req_ident);
			if (parser_eof(&p)) break;
			switch (parser_curr(&p))
			{
			case '?': rb->parsing.state = 2; break;
			case ' ': rb->parsing.state = 4; break;
			default:  rb->parsing.state = 11; break;
			}
			if (rb->parsing.state >= 10) break; // error
			rb->req.uri = &rb->buff[total];
			rb->buff[p.total] = 0;
			parser_skip_n(&p, 1);
			rb->parsing.pos = p.total;
			break;
		case 2:
			total = p.total;
			parser_skip(&p, req_ident);
			if (parser_eof(&p)) break;
			switch (parser_curr(&p))
			{
			case '&': rb->parsing.state = 2; break;
			case '=': rb->parsing.state = 3; break;
			case ' ': rb->parsing.state = 4; break;
			default:  rb->parsing.state = 12; break;
			}
			if (rb->parsing.state >= 10) break; // error
			rb->req.num_params++;
			if (rb->req.num_params <= HTTP_REQ_MAX_PARAMS)
			{
				rb->req.params[rb->req.num_params - 1] = &rb->buff[total];
				rb->req.values[rb->req.num_params - 1] = NULL;
			}
			rb->buff[p.total] = 0;
			parser_skip_n(&p, 1);
			rb->parsing.pos = p.total;
			break;
		case 3:
			total = p.total;
			parser_skip(&p, req_ident);
			if (parser_eof(&p)) break;
			switch (parser_curr(&p))
			{
			case '&': rb->parsing.state = 2; break;
			case ' ': rb->parsing.state = 4; break;
			default:  rb->parsing.state = 13; break;
			}
			if (rb->parsing.state >= 10) break; // error
			if (rb->req.num_params <= HTTP_REQ_MAX_PARAMS)
				rb->req.values[rb->req.num_params - 1] = &rb->buff[total];
			rb->buff[p.total] = 0;
			parser_skip_n(&p, 1);
			rb->parsing.pos = p.total;
			break;
		case 4:
			parser_skip_ws(&p);
			total = p.total;
			if (!parser_skip_line(&p)) break;
			rb->req.ver = &rb->buff[total];
			rb->buff[p.total - 2] = 0;
			rb->parsing.pos = p.total;
			rb->parsing.state = 5;
			break;
		case 5:
			parser_skip_ws_in_line(&p);
			if (parser_skip_char(&p, '\r') || parser_skip_char(&p, '\n'))
			{
				// DONE
				rb->state = REQB_FINISHED;
				return;
			}
			// 0     1   2            3
			// Accept:   text/html\r\n...
			parser_skip_ws_in_line(&p);
			indeces[0] = p.total;
			parser_skip(&p, req_ident);
			if (parser_eof(&p)) break;
			if (parser_curr(&p) != ':')
			{
				rb->parsing.state = 15;
				break;
			}
			indeces[1] = p.total;
			parser_skip_n(&p, 1);
			parser_skip_ws_in_line(&p);
			indeces[2] = p.total;
			if (!parser_skip_line(&p)) break;
			indeces[3] = p.total;
			rb->buff[indeces[1]] = 0;
			rb->buff[indeces[3] - 2] = 0;
			http_req_set_param(&rb->req,
				&rb->buff[indeces[0]],
				&rb->buff[indeces[2]]);
			rb->parsing.pos = p.total;
			break;
		default:
			rb->state = REQB_SYNT_ERROR;
			return;
		}
	}
}

int http_resp_len(const http_resp_t *resp)
{
	char temp[1];
	return http_resp_str(resp, temp, 1);
}

int http_resp_str(const http_resp_t *resp, char *str, int size)
{
	return snprintf(str, size,
		"HTTP/1.1 %d\r\n"
		"Server: %s\r\n"
//		"Content-Type: text/html\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %d\r\n"
		"Connection: %s\r\n"
		"\r\n",
		resp->code,
		resp->server,
		resp->mime,
		resp->cont_len,
		CONN_TYPE_STR[resp->conn_type]);
}

#ifdef __cplusplus
}
#endif
