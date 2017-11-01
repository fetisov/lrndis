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

#ifndef __USB_DESC_H
#define __USB_DESC_H

#include <stdint.h>
#include "usbd_def.h"

#define USBD_VID                        0x4E44
#define USBD_PID                        0x4953
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
