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
 * brief:   part of tiny http ipv4 server using lwip (pcb)
 */

#ifndef HTTP_REQ_H
#define HTTP_REQ_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cparser.h"

//#define HTTP_REQ_MAX_SIZE   512
#define HTTP_REQ_MAX_PARAMS 32

typedef enum http_method
{
	METHOD_NONE,
	METHOD_GET,
	METHOD_POST,
	METHOD_HEAD,
	METHOD_PUT,
	METHOD_CONNECT,
	METHOD_OPTIONS,
	METHOD_DELETE,
	METHOD_TRACE,
	METHOD_PATCH
} http_mt_t;

typedef enum http_conn_type
{
	CT_NONE,
	CT_CLOSE,
	CT_KEEP_ALIVE
} http_ct_t;

extern const char MIME_TEXT_HTML[];   // text/html
extern const char MIME_TEXT_HTML[];   // text/javascript
extern const char MIME_TEXT_PLAIN[];  // text/plain
extern const char MIME_TEXT_XML[];    // text/xml
extern const char MIME_TEXT_CSS[];    // text/css
extern const char MIME_IMAGE_GIF[];   // image/gif
extern const char MIME_IMAGE_JPEG[];  // image/jpeg
extern const char MIME_IMAGE_PJPEG[]; // image/pjpeg
extern const char MIME_IMAGE_PNG[];   // image/png
extern const char MIME_IMAGE_SVG[];   // image/svg+xml
extern const char MIME_IMAGE_TIFF[];  // image/tiff
extern const char MIME_IMAGE_ICON[];  // image/vnd.microsoft.icon
extern const char MIME_IMAGE_WBMP[];  // image/vnd.wap.wbmp

typedef struct http_req
{
	http_mt_t  method;                      // GET
	char      *uri;                         //      /path/resource
	int        num_params;                  //                      2
	char      *params[HTTP_REQ_MAX_PARAMS]; //                      param1, param2
	char      *values[HTTP_REQ_MAX_PARAMS]; //                      value1, value2
	char      *ver;                         //                                      HTTP/1.1
	char      *host;                        // Host:            [wikipedia.org]
	char      *user_agent;                  // User-Agent:      [Mozilla/5.0]
	char      *mime;                        // Content-Type:    [multipart/form-data; boundary="Asrf456BGe4h"]
	int        cont_len;                    // Content-Length:  [123]
	char      *accept;                      // Accept:          [text/html]
	char      *accept_lang;                 // Accept-Language: [en-US;q=0.5,en;q=0.3]
	char      *accept_enc;                  // Accept-Encoding: [gzip, deflate]
	char      *cookie;                      // Cookie:          [Cookie data]
	http_ct_t  conn_type;                   // Connection:      [keep-alive]
	int        keep_alive;                  // Keep-Alive:      [300]
} http_req_t;

typedef struct http_resp
{
	int         code;                   // HTTP/1.1 [200] OK
	const char *server;                 // Server: [lrndis]
	const char *cont_lang;              // Content-Language: [ru]
	const char *mime;                   // Content-Type: [text/html; charset=utf-8]
	int         cont_len;               // Content-Length: [1234]
	http_ct_t   conn_type;              // Connection: [close]
} http_resp_t;

// HTTP REQUEST BUFFER

typedef enum http_reqb_state
{
	REQB_UNFINISHED,
	REQB_FINISHED,
	REQB_SYNT_ERROR,
	REQB_REQ_TO_BIG
} http_reqb_state_t;

typedef struct http_reqb
{
	http_req_t req;
	http_reqb_state_t state;
	//char buff[HTTP_REQ_MAX_SIZE];
	char *buff;
	int bsize;
	int size;
	struct
	{
		int pos;
		int state;
	} parsing;
} http_reqb_t;

void http_reqb_init(http_reqb_t *reqb, void *buff, int size);
int  http_reqb_avail(const http_reqb_t *reqb);
void http_reqb_push(http_reqb_t *rb, const void *data, int size);
int  http_resp_len(const http_resp_t *resp);
int  http_resp_str(const http_resp_t *resp, char *str, int size);

#ifdef __cplusplus
}
#endif

#endif
