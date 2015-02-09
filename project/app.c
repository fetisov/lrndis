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
 * brief:   main file of LRNDIS lib demonstration project
 */

#include "dhserver.h"
#include "dnserver.h"
#include <stdlib.h>
#include <stdio.h>
#include "usbd_rndis_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usb_conf.h"
#include "netif/etharp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/icmp.h"
#include "lwip/udp.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
#include "lwip/tcp_impl.h"
#include "lwip/tcp.h"
#include "http_req.h"
#include "htserv.h"
#include "time.h"

__ALIGN_BEGIN
USB_OTG_CORE_HANDLE USB_OTG_dev
__ALIGN_END;

//USB_OTG_CORE_HANDLE USB_OTG_dev; // usb device struct

static uint8_t hwaddr[6]  = {0x20,0x89,0x84,0x6A,0x96,00};
static uint8_t ipaddr[4]  = {192, 168, 7, 1};
static uint8_t netmask[4] = {255, 255, 255, 0};
static uint8_t gateway[4] = {0, 0, 0, 0};

#define NUM_DHCP_ENTRY 3

static dhcp_entry_t entries[NUM_DHCP_ENTRY] =
{
	// mac    ip address        subnet mask        lease time
	{ {0}, {192, 168, 7, 2}, {255, 255, 255, 0}, 24 * 60 * 60 },
	{ {0}, {192, 168, 7, 3}, {255, 255, 255, 0}, 24 * 60 * 60 },
	{ {0}, {192, 168, 7, 4}, {255, 255, 255, 0}, 24 * 60 * 60 }
};

static dhcp_config_t dhcp_config =
{
	{192, 168, 7, 1}, 67, // server address, port
	{192, 168, 7, 1},     // dns server
	"stm",                // dns suffix
	NUM_DHCP_ENTRY,       // num entry
	entries               // entries
};

struct netif netif_data;

uint32_t sys_now()
{
	return (uint32_t)mtime();
}

static uint8_t received[ETH_MTU];
static int recvSize = 0;

void on_packet(const char *data, int size)
{
	memcpy(received, data, size);
	recvSize = size;
}

void usb_polling()
{
	if (recvSize == 0) return;
	struct pbuf *frame;
	frame = pbuf_alloc(PBUF_RAW, recvSize, PBUF_POOL);
	if(frame == NULL) return;
	memcpy(frame->payload, received, recvSize);
	frame->len = recvSize;
	ethernet_input(frame, &netif_data);
	pbuf_free(frame);
	recvSize = 0;
}


static int outputs = 0;

err_t output_fn(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr)
{
	return etharp_output(netif, p, ipaddr);
}

err_t linkoutput_fn(struct netif *netif, struct pbuf *p)
{
	int i;
	if (p->len > ETH_MTU)
		return ERR_ARG;
	for (i = 0; i < 500; i++)
	{
		if (rndis_can_send()) break;
		msleep(1);
	}
	if (!rndis_can_send())
		return ERR_USE;
	static char data[ETH_MTU];
	memcpy(data, (char *)p->payload, p->len);
	rndis_send(data, p->len);
	outputs++;
	return ERR_OK;
}

err_t netif_init_cb(struct netif *netif)
{
	LWIP_ASSERT("netif != NULL", (netif != NULL));
	netif->mtu = ETH_MTU;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
	netif->state = NULL;
	netif->name[0] = 'E';
	netif->name[1] = 'X';
	netif->linkoutput = linkoutput_fn;
	netif->output = output_fn;
	return ERR_OK;
}

#define PADDR(ptr) ((ip_addr_t *)ptr)

TIMER_PROC(tcp_timer, TCP_TMR_INTERVAL * 1000, 1, NULL)
{
	tcp_tmr();
}

TIMER_PROC(http_timer, 100, 1, NULL)
{
	htserv_tmr(mtime());
}

void init_lwip()
{
	struct netif  *netif = &netif_data;

	lwip_init();
	netif->hwaddr_len = 6;
	memcpy(netif->hwaddr, hwaddr, 6);

	netif = netif_add(netif, PADDR(ipaddr), PADDR(netmask), PADDR(gateway), NULL, netif_init_cb, ip_input);
	netif_set_default(netif);

	stmr_add(&tcp_timer);
	stmr_add(&http_timer);
}

void init_periph(void)
{
	time_init();
	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &usbd_rndis_cb, &USR_cb);
	rndis_rxproc = on_packet;
}

bool dns_query_proc(const char *name, ip_addr_t *addr)
{
	if (strcmp(name, "run.stm") == 0 || strcmp(name, "www.run.stm") == 0)
	{
		addr->addr = *(uint32_t *)ipaddr;
		return true;
	}
	return false;
}

typedef struct my_page
{
	const char *uri;
	int         code;
	const char *mime;
	const void *data;
	int         size;
} my_page_t;

#include "resources.inc"

static const my_page_t my_pages[] =
{
	{ "/",          200, MIME_TEXT_HTML, page1_html,     page1_html_size      },
	{ "/page2.htm", 200, MIME_TEXT_HTML, page2_html,     page2_html_size      },
	{ "/page3.htm", 200, MIME_TEXT_HTML, page3_html,     page3_html_size      },
	{ "/check.gif", 200, MIME_IMAGE_GIF, check_png,      check_png_size       },
	{ NULL,         404, MIME_TEXT_HTML, page_not_found, page_not_found_size  }
};

bool on_http_req(const http_req_t *req, http_resp_t *resp, void **arg)
{
	const my_page_t *page;
	for (page = my_pages; page->uri != NULL; page++)
		if (strcmp(page->uri, req->uri) == 0) break;
	resp->code = page->code;
	resp->cont_len = page->size;
	resp->mime = page->mime;
	resp->conn_type = CT_CLOSE;
	*arg = (void *)page;
	return true;
}

void http_write_data()
{
	for (int i = 0; i < HTTP_SERVER_MAX_CON; i++)
	{
		int n;
		const htcon_t *con;
		my_page_t *page;
		con = htcon(i);
		if (con == NULL) continue;
		page = (my_page_t *)con->arg;
		if (con->state == CON_CLOSED)
		{
			htcon_free(i);
			continue;
		}
		if (con->state != CON_ACTIVE) continue;
		n = page->size - con->writed;
		htcon_write(i, (char *)page->data + con->writed, n);
	}
}

int main(void)
{
	init_periph();
	while (rndis_state != rndis_initialized) ;

	init_lwip();

	while (!netif_is_up(&netif_data)) ;

	while (dhserv_init(&dhcp_config) != ERR_OK) ;

	while (dnserv_init(PADDR(ipaddr), 53, dns_query_proc) != ERR_OK) ;

	htserv_on_req = on_http_req;
	while (htserv_init(80) != ERR_OK) ;

	while (1)
	{
		stmr();            // call software timers
		usb_polling();     // usb device polling
		http_write_data(); // writes http response
	}
}
