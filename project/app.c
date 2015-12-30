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
#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lis302dl.h"
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
#include "time.h"
#include "httpd.h"

__ALIGN_BEGIN
USB_OTG_CORE_HANDLE USB_OTG_dev
__ALIGN_END;

//USB_OTG_CORE_HANDLE USB_OTG_dev; // usb device struct

static uint8_t hwaddr[6]  = {0x20,0x89,0x84,0x6A,0x96,00};
static uint8_t ipaddr[4]  = {192, 168, 7, 1};
static uint8_t netmask[4] = {255, 255, 255, 0};
static uint8_t gateway[4] = {0, 0, 0, 0};

#define LED_ORANGE LED3
#define LED_GREEN  LED4
#define LED_RED    LED5
#define LED_BLUE   LED6
#define LINK_LED   LED_BLUE

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

static uint8_t received[ETH_MTU + 14];
static int recvSize = 0;

void on_packet(const char *data, int size)
{
	memcpy(received, data, size);
	recvSize = size;
}

TIMER_PROC(link_led_off, 50 * 1000, 1, NULL)
{
	STM_EVAL_LEDOff(LINK_LED);
}

TIMER_PROC(tcp_timer, TCP_TMR_INTERVAL * 1000, 1, NULL)
{
	tcp_tmr();
}

void usb_polling()
{
	__disable_irq();
	if (recvSize == 0) 
	{
		__enable_irq();
		return;
	}
	struct pbuf *frame;
	frame = pbuf_alloc(PBUF_RAW, recvSize, PBUF_POOL);
	if (frame == NULL) 
	{
		__enable_irq();
		return;
	}
	memcpy(frame->payload, received, recvSize);
	frame->len = recvSize;
	recvSize = 0;
	__enable_irq();
	ethernet_input(frame, &netif_data);
	pbuf_free(frame);

	STM_EVAL_LEDOn(LINK_LED);
	stmr_run(&link_led_off);
}

static int outputs = 0;

err_t output_fn(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr)
{
	return etharp_output(netif, p, ipaddr);
}

err_t linkoutput_fn(struct netif *netif, struct pbuf *p)
{
	int i;
	struct pbuf *q;
	static char data[ETH_MTU + 14 + 4];
	int size = 0;
	for (i = 0; i < 200; i++)
	{
		if (rndis_can_send()) break;
		msleep(1);
	}
  for(q = p; q != NULL; q = q->next)
	{
		if (size + q->len > ETH_MTU + 14)
			return ERR_ARG;
		memcpy(data + size, (char *)q->payload, q->len);
		size += q->len;
	}
	if (!rndis_can_send())
		return ERR_USE;
	rndis_send(data, size);
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

void init_lwip()
{
	struct netif  *netif = &netif_data;

	lwip_init();
	netif->hwaddr_len = 6;
	memcpy(netif->hwaddr, hwaddr, 6);

	netif = netif_add(netif, PADDR(ipaddr), PADDR(netmask), PADDR(gateway), NULL, netif_init_cb, ip_input);
	netif_set_default(netif);

	stmr_add(&tcp_timer);
	stmr_add(&link_led_off);
}

void init_periph(void)
{
	time_init();
	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &usbd_rndis_cb, &USR_cb);
	rndis_rxproc = on_packet;
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	STM_EVAL_LEDInit(LED_ORANGE);
	STM_EVAL_LEDInit(LED_GREEN);
	STM_EVAL_LEDInit(LED_RED);
	STM_EVAL_LEDInit(LED_BLUE);

	static LIS302DL_InitTypeDef	accInit =
	{
		LIS302DL_LOWPOWERMODE_ACTIVE,
		LIS302DL_DATARATE_100,
		LIS302DL_XYZ_ENABLE,
		LIS302DL_FULLSCALE_2_3,
		LIS302DL_SELFTEST_NORMAL
	};
	LIS302DL_Init(&accInit);
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

const char *state_cgi_handler(int index, int n_params, char *params[], char *values[])
{
	return "/state.shtml";
}

bool led_g = false;
bool led_o = false;
bool led_r = false;

const char *ctl_cgi_handler(int index, int n_params, char *params[], char *values[])
{
	int i;
	for (i = 0; i < n_params; i++)
	{
		if (strcmp(params[i], "g") == 0) led_g = *values[i] == '1';
		if (strcmp(params[i], "o") == 0) led_o = *values[i] == '1';
		if (strcmp(params[i], "r") == 0) led_r = *values[i] == '1';
	}

	if (led_g)
		STM_EVAL_LEDOn(LED_GREEN); else
		STM_EVAL_LEDOff(LED_GREEN);
	if (led_o)
		STM_EVAL_LEDOn(LED_ORANGE); else
		STM_EVAL_LEDOff(LED_ORANGE);
	if (led_r)
		STM_EVAL_LEDOn(LED_RED); else
		STM_EVAL_LEDOff(LED_RED);

	return "/state.shtml";
}

static const char *ssi_tags_table[] =
{
     "systick", // 0
		 "btn",     // 1
		 "acc",     // 2
		 "ledg",    // 3
		 "ledo",    // 4
		 "ledr"     // 5
};

static const tCGI cgi_uri_table[] =
{
	{ "/state.cgi", state_cgi_handler },
	{ "/ctl.cgi",   ctl_cgi_handler },
};

static u16_t ssi_handler(int index, char *insert, int ins_len)
{
	int res;

	if (ins_len < 32) return 0;

	switch (index)
	{
	case 0: // systick
		res = snprintf(insert, ins_len, "%u", (unsigned)mtime());
		break;
	case 1: // btn
		res = snprintf(insert, ins_len, "%i", STM_EVAL_PBGetState(BUTTON_USER) & 1);
		break;
	case 2: // acc
	{
		int32_t acc[3];
		LIS302DL_ReadACC(acc);
		res = snprintf(insert, ins_len, "%i, %i, %i", acc[0], acc[1], acc[2]);
		break;
	}
	case 3: // ledg
		*insert = '0' + (led_g & 1);
		res = 1;
		break;
	case 4: // ledo
		*insert = '0' + (led_o & 1);
		res = 1;
		break;
	case 5: // ledr
		*insert = '0' + (led_r & 1);
		res = 1;
		break;
	}

	return res;
}

int main(void)
{
	init_periph();

	while (rndis_state != rndis_initialized) ;

	init_lwip();

	while (!netif_is_up(&netif_data)) ;

	while (dhserv_init(&dhcp_config) != ERR_OK) ;

	while (dnserv_init(PADDR(ipaddr), 53, dns_query_proc) != ERR_OK) ;

	http_set_cgi_handlers(cgi_uri_table, sizeof(cgi_uri_table) / sizeof(tCGI));
	http_set_ssi_handler(ssi_handler, ssi_tags_table, sizeof(ssi_tags_table) / sizeof(char *));
	httpd_init();

	while (1)
	{
		usb_polling();     // usb device polling
		stmr();            // call software timers
	}
}
