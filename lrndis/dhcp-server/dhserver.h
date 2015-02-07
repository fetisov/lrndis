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
 * brief:   tiny dhcp ipv4 server using lwip (pcb)
 * ref:     https://lists.gnu.org/archive/html/lwip-users/2012-12/msg00016.html
 */

#ifndef DHSERVER_H
#define DHSERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lwip/err.h"
#include "lwip/udp.h"
#include "netif/etharp.h"

//#define DHCP_SERVER_PORT 67
//#define DHCP_CLIENT_PORT 68

typedef struct dhcp_entry
{
	uint8_t  mac[6];
	uint8_t  addr[4];
	uint8_t  subnet[4];
	uint32_t lease;
} dhcp_entry_t;

typedef struct dhcp_config
{
	uint8_t       addr[4];
	uint16_t      port;
	uint8_t       dns[4];
	const char   *domain;
	int           num_entry;
	dhcp_entry_t *entries;
} dhcp_config_t;

err_t dhserv_init(dhcp_config_t *config);
void dhserv_free(void);

#endif // DHSERVER_H
