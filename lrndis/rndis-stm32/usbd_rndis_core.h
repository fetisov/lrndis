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
 */

#ifndef __USB_CDC_CORE_H_
#define __USB_CDC_CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include "usbd_ioreq.h"
#include "usbd_rndis_core.h"
#include "rndis_protocol.h"

#define ETH_MTU          1500                           // MTU value
#define ETH_LINK_SPEED   250000                         // bits per sec
#define RNDIS_VENDOR     "fetisov"                      // NIC vendor name
#define STATION_HWADDR   0x20,0x89,0x84,0x6A,0x96,0xAA  // station MAC
#define PERMANENT_HWADDR 0x20,0x89,0x84,0x6A,0x96,0xAA  // permanent MAC

typedef void (*rndis_rxproc_t)(const char *data, int size);

extern USBD_Class_cb_TypeDef usbd_rndis_cb;

extern usb_eth_stat_t usb_eth_stat;
extern rndis_state_t rndis_state;

extern rndis_rxproc_t rndis_rxproc;

bool   rndis_can_send(void);
bool   rndis_send(const void *data, int size);

#endif
