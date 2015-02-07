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
