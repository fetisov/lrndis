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

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usbd_conf.h"
#include "usb_regs.h"

#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_SIZ_STRING_LANGID                   4
#define USB_SIZ_DEVICE_DESC                     0x12

uint8_t *USBD_USR_DeviceDescriptor( uint8_t speed , uint16_t *length);
uint8_t *USBD_USR_LangIDStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *USBD_USR_ManufacturerStrDescriptor ( uint8_t speed , uint16_t *length);
uint8_t *USBD_USR_ProductStrDescriptor ( uint8_t speed , uint16_t *length);
uint8_t *USBD_USR_SerialStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *USBD_USR_ConfigStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *USBD_USR_InterfaceStrDescriptor( uint8_t speed , uint16_t *length);
#ifdef USB_SUPPORT_USER_STRING_DESC
uint8_t *USBD_USR_USRStringDesc (uint8_t speed, uint8_t idx , uint16_t *length);  
#endif

USBD_DEVICE USR_desc =
{
  USBD_USR_DeviceDescriptor,
  USBD_USR_LangIDStrDescriptor, 
  USBD_USR_ManufacturerStrDescriptor,
  USBD_USR_ProductStrDescriptor,
  USBD_USR_SerialStrDescriptor,
  USBD_USR_ConfigStrDescriptor,
  USBD_USR_InterfaceStrDescriptor,
};

extern  uint8_t USBD_StrDesc[USB_MAX_STR_DESC_SIZ];
extern  uint8_t USBD_OtherSpeedCfgDesc[USB_LEN_CFG_DESC]; 
extern  uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC];
extern  uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID];

__ALIGN_BEGIN uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END =
{
    18,                                 /* bLength = 18 bytes */
    USB_DEVICE_DESCRIPTOR_TYPE,         /* bDescriptorType = DEVICE */
    0x00, 0x02,                         /* bcdUSB          = 1.1 0x10,0x01  2.0 0x00,0x02 */
    0xE0,                               /* bDeviceClass    = Wireless Controller */
    0x00,                               /* bDeviceSubClass = Unused at this time */
    0x00,                               /* bDeviceProtocol = Unused at this time */
    0x40,                               /* bMaxPacketSize0 = EP0 buffer size */
    LOBYTE(USBD_VID), HIBYTE(USBD_VID), /* Vendor ID */
    LOBYTE(USBD_PID), HIBYTE(USBD_PID), /* Product ID */
    0xFF, 0xFF,                         /* bcdDevice */
    USBD_IDX_MFC_STR,
    USBD_IDX_PRODUCT_STR,
    USBD_IDX_SERIAL_STR,
    1
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END =
{
     USB_SIZ_STRING_LANGID,         
     USB_DESC_TYPE_STRING,       
     LOBYTE(USBD_LANGID_STRING),
     HIBYTE(USBD_LANGID_STRING), 
};

uint8_t *USBD_USR_DeviceDescriptor(uint8_t speed , uint16_t *length)
{
    *length = sizeof(USBD_DeviceDesc);
    return USBD_DeviceDesc;
}

uint8_t *USBD_USR_LangIDStrDescriptor(uint8_t speed , uint16_t *length)
{
    *length =  sizeof(USBD_LangIDDesc);  
    return USBD_LangIDDesc;
}

uint8_t *USBD_USR_ProductStrDescriptor(uint8_t speed , uint16_t *length)
{
    if(speed == 0)
    {   
        USBD_GetString((uint8_t *)USBD_PRODUCT_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString((uint8_t *)USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);    
    }
    return USBD_StrDesc;
}

uint8_t *USBD_USR_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

uint8_t *USBD_USR_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
    if(speed  == USB_OTG_SPEED_HIGH)
    {    
        USBD_GetString((uint8_t *)USBD_SERIALNUMBER_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString((uint8_t *)USBD_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);    
    }
    return USBD_StrDesc;
}

uint8_t *USBD_USR_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
    if(speed  == USB_OTG_SPEED_HIGH)
    {  
        USBD_GetString((uint8_t *)USBD_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString((uint8_t *)USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length); 
    }
    return USBD_StrDesc;  
}

uint8_t *USBD_USR_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
    if(speed == 0)
    {
        USBD_GetString((uint8_t *)USBD_INTERFACE_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString((uint8_t *)USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;  
}
