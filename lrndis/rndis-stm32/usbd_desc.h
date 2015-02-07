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

#ifndef __USB_DESC_H
#define __USB_DESC_H

#include <stdint.h>
#include "usbd_def.h"

#define USBD_VID                        0x0483
#define USBD_PID                        0x0123
#define USBD_LANGID_STRING              0x409
#define USBD_MANUFACTURER_STRING        "Fetisov Sergey"
#define USBD_PRODUCT_HS_STRING          "STM32F4 RNDIS"
#define USBD_SERIALNUMBER_HS_STRING     "00000000123B"
#define USBD_PRODUCT_FS_STRING          "STM32F4 RNDIS"
#define USBD_SERIALNUMBER_FS_STRING     "00000000123C"
#define USBD_CONFIGURATION_HS_STRING    "RNDIS Config"
#define USBD_INTERFACE_HS_STRING        "RNDIS Interface"
#define USBD_CONFIGURATION_FS_STRING    "RNDIS Config"
#define USBD_INTERFACE_FS_STRING        "RNDIS Interface"

extern  USBD_DEVICE USR_desc;

#endif /* __USBD_DESC_H */
