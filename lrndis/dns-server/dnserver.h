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
 * brief:   tiny dns ipv4 server using lwip (pcb)
 */

#ifndef DNSERVER
#define DNSERVER

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lwip/def.h"
#include "lwip/err.h"
#include "lwip/udp.h"
#include "netif/etharp.h"

typedef bool (*dns_query_proc_t)(const char *name, ip_addr_t *addr);

err_t dnserv_init(ip_addr_t *bind, uint16_t port, dns_query_proc_t query_proc);
void  dnserv_free(void);

#endif
