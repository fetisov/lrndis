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

#include "usbd_rndis_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"

/*********************************************
   RNDIS Device library callbacks
 *********************************************/
static uint8_t  usbd_rndis_init          (void *pdev, uint8_t cfgidx);
static uint8_t  usbd_rndis_deinit        (void *pdev, uint8_t cfgidx);
static uint8_t  usbd_rndis_setup         (void *pdev, USB_SETUP_REQ *req);
static uint8_t  usbd_rndis_ep0_recv      (void *pdev);
static uint8_t  usbd_rndis_data_in       (void *pdev, uint8_t epnum);
static uint8_t  usbd_rndis_data_out      (void *pdev, uint8_t epnum);
static uint8_t  usbd_rndis_sof           (void *pdev);
static uint8_t  rndis_iso_in_incomplete  (void *pdev);
static uint8_t  rndis_iso_out_incomplete (void *pdev);
static uint8_t *usbd_rndis_get_cfg       (uint8_t speed, uint16_t *length);

/*********************************************
   RNDIS specific management functions
 *********************************************/

#define ETH_HEADER_SIZE             14
#define ETH_MAX_PACKET_SIZE         ETH_HEADER_SIZE + RNDIS_MTU
#define ETH_MIN_PACKET_SIZE         60
#define RNDIS_RX_BUFFER_SIZE        (ETH_MAX_PACKET_SIZE + sizeof(rndis_data_packet_t))

static const uint8_t station_hwaddr[6] = { RNDIS_HWADDR };
static const uint8_t permanent_hwaddr[6] = { RNDIS_HWADDR };

usb_eth_stat_t usb_eth_stat = { 0, 0, 0, 0 };
uint32_t oid_packet_filter = 0x0000000;
__ALIGN_BEGIN char rndis_rx_buffer[RNDIS_RX_BUFFER_SIZE] __ALIGN_END;
__ALIGN_BEGIN uint8_t usb_rx_buffer[RNDIS_DATA_OUT_SZ] __ALIGN_END ;
rndis_rxproc_t rndis_rxproc = NULL;
uint8_t *rndis_tx_ptr = NULL;
bool rndis_first_tx = true;
int rndis_tx_size = 0;
int rndis_sended = 0;
rndis_state_t rndis_state;

extern USB_OTG_CORE_HANDLE USB_OTG_dev;

USBD_Class_cb_TypeDef usbd_rndis_cb =
{
    usbd_rndis_init,
    usbd_rndis_deinit,
    usbd_rndis_setup,
    NULL,
    usbd_rndis_ep0_recv,
    usbd_rndis_data_in,
    usbd_rndis_data_out,
    usbd_rndis_sof,
    rndis_iso_in_incomplete,
    rndis_iso_out_incomplete,
    usbd_rndis_get_cfg
};

__ALIGN_BEGIN static __IO uint32_t  usbd_cdc_AltSet  __ALIGN_END = 0;

#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

__ALIGN_BEGIN uint8_t usbd_cdc_CfgDesc[] __ALIGN_END =
{
    /* Configuration descriptor */

    9,                                 /* bLength         = 9 bytes. */
    USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType = CONFIGURATION */
    0xDE, 0xAD,                        /* wTotalLength    = sizeof(usbd_cdc_CfgDesc) */
    0x02,                              /* bNumInterfaces  = 2 (RNDIS spec). */
    0x01,                              /* bConfValue      = 1 */
    0x00,                              /* iConfiguration  = unused. */
    0x40,                              /* bmAttributes    = Self-Powered. */
    0x01,                              /* MaxPower        = x2mA */

    /* IAD descriptor */

    0x08, /* bLength */
    0x0B, /* bDescriptorType */
    0x00, /* bFirstInterface */
    0x02, /* bInterfaceCount */
    0xE0, /* bFunctionClass (Wireless Controller) */
    0x01, /* bFunctionSubClass */
    0x03, /* bFunctionProtocol */
    0x00, /* iFunction */

    /* Interface 0 descriptor */
    
    9,                             /* bLength */
    USB_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType = INTERFACE */
    0x00,                          /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    1,                             /* bNumEndpoints */
    0xE0,                          /* bInterfaceClass: Wireless Controller */
    0x01,                          /* bInterfaceSubClass */
    0x03,                          /* bInterfaceProtocol */
    0,                             /* iInterface */

    /* Interface 0 functional descriptor */

    /* Header Functional Descriptor */
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType = CS Interface */
    0x00, /* bDescriptorSubtype */
    0x10, /* bcdCDC = 1.10 */
    0x01, /* bcdCDC = 1.10 */

    /* Call Management Functional Descriptor */
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType = CS Interface */
    0x01, /* bDescriptorSubtype = Call Management */
    0x00, /* bmCapabilities */
    0x01, /* bDataInterface */

    /* Abstract Control Management Functional Descriptor */
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType = CS Interface */
    0x02, /* bDescriptorSubtype = Abstract Control Management */
    0x00, /* bmCapabilities = Device supports the notification Network_Connection */

    /* Union Functional Descriptor */
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType = CS Interface */
    0x06, /* bDescriptorSubtype = Union */
    0x00, /* bControlInterface = "RNDIS Communications Control" */
    0x01, /* bSubordinateInterface0 = "RNDIS Ethernet Data" */

    /* Endpoint descriptors for Communication Class Interface */

    7,                            /* bLength         = 7 bytes */
    USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType = ENDPOINT */
    RNDIS_NOTIFICATION_IN_EP,     /* bEndpointAddr   = IN - EP3 */
    0x03,                         /* bmAttributes    = Interrupt endpoint */
    8, 0,                         /* wMaxPacketSize */
    0x01,                         /* bInterval       = 1 ms polling from host */

    /* Interface 1 descriptor */

    9,                             /* bLength */
    USB_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    0x01,                          /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    2,                             /* bNumEndpoints */
    0x0A,                          /* bInterfaceClass: CDC */
    0x00,                          /* bInterfaceSubClass */
    0x00,                          /* bInterfaceProtocol */
    0x00,                          /* uint8  iInterface */

    /* Endpoint descriptors for Data Class Interface */

    7,                            /* bLength         = 7 bytes */
    USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType = ENDPOINT [IN] */
    RNDIS_DATA_IN_EP,             /* bEndpointAddr   = IN EP */
    0x02,                         /* bmAttributes    = BULK */
    RNDIS_DATA_IN_SZ, 0,          /* wMaxPacketSize */
    0,                            /* bInterval       = ignored for BULK */

    7,                            /* bLength         = 7 bytes */
    USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType = ENDPOINT [OUT] */
    RNDIS_DATA_OUT_EP,            /* bEndpointAddr   = OUT EP */
    0x02,                         /* bmAttributes    = BULK */
    RNDIS_DATA_OUT_SZ, 0,         /* wMaxPacketSize */
    0                             /* bInterval       = ignored for BULK */
};

static uint8_t usbd_rndis_init(void  *pdev, uint8_t cfgidx)
{
  DCD_EP_Open(pdev, RNDIS_NOTIFICATION_IN_EP, RNDIS_NOTIFICATION_IN_SZ, USB_OTG_EP_INT);
  DCD_EP_Open(pdev, RNDIS_DATA_IN_EP, RNDIS_DATA_IN_SZ, USB_OTG_EP_BULK);
  DCD_EP_Open(pdev, RNDIS_DATA_OUT_EP, RNDIS_DATA_OUT_SZ, USB_OTG_EP_BULK);
  DCD_EP_PrepareRx(pdev, RNDIS_DATA_OUT_EP, (uint8_t*)usb_rx_buffer, RNDIS_DATA_OUT_SZ);
  return USBD_OK;
}

static uint8_t  usbd_rndis_deinit(void  *pdev, uint8_t cfgidx)
{
  DCD_EP_Close(pdev, RNDIS_NOTIFICATION_IN_EP);
  DCD_EP_Close(pdev, RNDIS_DATA_IN_EP);
  DCD_EP_Close(pdev, RNDIS_DATA_OUT_EP);
  return USBD_OK;
}

const uint32_t OIDSupportedList[] = 
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_MAC_OPTIONS
};

#define OID_LIST_LENGTH (sizeof(OIDSupportedList) / sizeof(*OIDSupportedList))
#define ENC_BUF_SIZE    (OID_LIST_LENGTH * 4 + 32)

uint8_t encapsulated_buffer[ENC_BUF_SIZE];

static uint8_t usbd_rndis_setup(void  *pdev, USB_SETUP_REQ *req)
{
  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
      if (req->wLength != 0) /* is data setup packet? */
      {
        /* Check if the request is Device-to-Host */
        if (req->bmRequest & 0x80)
        {
					USBD_CtlSendData(pdev, encapsulated_buffer, ((rndis_generic_msg_t *)encapsulated_buffer)->MessageLength);
        }
        else /* Host-to-Device requeset */
        {
          USBD_CtlPrepareRx(pdev, encapsulated_buffer, req->wLength);          
        }
      }
      return USBD_OK;
      
    default:
			return USBD_OK;
  }
}

void rndis_query_cmplt32(int status, uint32_t data)
{
	rndis_query_cmplt_t *c;
	c = (rndis_query_cmplt_t *)encapsulated_buffer;
	c->MessageType = REMOTE_NDIS_QUERY_CMPLT;
	c->MessageLength = sizeof(rndis_query_cmplt_t) + 4;
	c->InformationBufferLength = 4;
	c->InformationBufferOffset = 16;
	c->Status = status;
	*(uint32_t *)(c + 1) = data;
	DCD_EP_Tx(&USB_OTG_dev, RNDIS_NOTIFICATION_IN_EP, (uint8_t *)"\x01\x00\x00\x00\x00\x00\x00\x00", 8);
}

void rndis_query_cmplt(int status, const void *data, int size)
{
	rndis_query_cmplt_t *c;
	c = (rndis_query_cmplt_t *)encapsulated_buffer;
	c->MessageType = REMOTE_NDIS_QUERY_CMPLT;
	c->MessageLength = sizeof(rndis_query_cmplt_t) + size;
	c->InformationBufferLength = size;
	c->InformationBufferOffset = 16;
	c->Status = status;
	memcpy(c + 1, data, size);
	DCD_EP_Tx(&USB_OTG_dev, RNDIS_NOTIFICATION_IN_EP, (uint8_t *)"\x01\x00\x00\x00\x00\x00\x00\x00", 8);
}

#define MAC_OPT NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | \
			NDIS_MAC_OPTION_RECEIVE_SERIALIZED  | \
			NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  | \
			NDIS_MAC_OPTION_NO_LOOPBACK

static const char *rndis_vendor = RNDIS_VENDOR;

void rndis_query(void  *pdev)
{
	switch (((rndis_query_msg_t *)encapsulated_buffer)->Oid)
	{
		case OID_GEN_SUPPORTED_LIST:         rndis_query_cmplt(RNDIS_STATUS_SUCCESS, OIDSupportedList, 4 * OID_LIST_LENGTH); return;
		case OID_GEN_VENDOR_DRIVER_VERSION:  rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0x00001000);  return;
		case OID_802_3_CURRENT_ADDRESS:      rndis_query_cmplt(RNDIS_STATUS_SUCCESS, &station_hwaddr, 6); return;
		case OID_802_3_PERMANENT_ADDRESS:    rndis_query_cmplt(RNDIS_STATUS_SUCCESS, &permanent_hwaddr, 6); return;
		case OID_GEN_MEDIA_SUPPORTED:        rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, NDIS_MEDIUM_802_3); return;
		case OID_GEN_MEDIA_IN_USE:           rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, NDIS_MEDIUM_802_3); return;
		case OID_GEN_PHYSICAL_MEDIUM:        rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, NDIS_MEDIUM_802_3); return;
		case OID_GEN_HARDWARE_STATUS:        rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
		case OID_GEN_LINK_SPEED:             rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, RNDIS_LINK_SPEED / 100); return;
		case OID_GEN_VENDOR_ID:              rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0x00FFFFFF); return;
		case OID_GEN_VENDOR_DESCRIPTION:     rndis_query_cmplt(RNDIS_STATUS_SUCCESS, rndis_vendor, strlen(rndis_vendor) + 1); return;
		case OID_GEN_CURRENT_PACKET_FILTER:  rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, oid_packet_filter); return;
		case OID_GEN_MAXIMUM_FRAME_SIZE:     rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, ETH_MAX_PACKET_SIZE - ETH_HEADER_SIZE); return;
		case OID_GEN_MAXIMUM_TOTAL_SIZE:     rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, ETH_MAX_PACKET_SIZE); return;
		case OID_GEN_TRANSMIT_BLOCK_SIZE:    rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, ETH_MAX_PACKET_SIZE); return;
		case OID_GEN_RECEIVE_BLOCK_SIZE:     rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, ETH_MAX_PACKET_SIZE); return;
		case OID_GEN_MEDIA_CONNECT_STATUS:   rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, NDIS_MEDIA_STATE_CONNECTED); return;
		case OID_GEN_RNDIS_CONFIG_PARAMETER: rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
		case OID_802_3_MAXIMUM_LIST_SIZE:    rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 1); return;
		case OID_802_3_MULTICAST_LIST:       rndis_query_cmplt32(RNDIS_STATUS_NOT_SUPPORTED, 0); return;
		case OID_802_3_MAC_OPTIONS:          rndis_query_cmplt32(RNDIS_STATUS_NOT_SUPPORTED, 0); return;
		case OID_GEN_MAC_OPTIONS:            rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, /*MAC_OPT*/ 0); return;
		case OID_802_3_RCV_ERROR_ALIGNMENT:  rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
		case OID_802_3_XMIT_ONE_COLLISION:   rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
		case OID_802_3_XMIT_MORE_COLLISIONS: rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
		case OID_GEN_XMIT_OK:                rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, usb_eth_stat.txok); return;
		case OID_GEN_RCV_OK:                 rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, usb_eth_stat.rxok); return;
		case OID_GEN_RCV_ERROR:              rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, usb_eth_stat.rxbad); return;
		case OID_GEN_XMIT_ERROR:             rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, usb_eth_stat.txbad); return;
		case OID_GEN_RCV_NO_BUFFER:          rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
		default:                             rndis_query_cmplt(RNDIS_STATUS_FAILURE, NULL, 0); return;
	}
}

#define INFBUF ((uint32_t *)((uint8_t *)&(m->RequestId) + m->InformationBufferOffset))
#define CFGBUF ((rndis_config_parameter_t *) INFBUF)
#define PARMNAME  ((uint8_t *)CFGBUF + CFGBUF->ParameterNameOffset)
#define PARMVALUE ((uint8_t *)CFGBUF + CFGBUF->ParameterValueOffset)
#define PARMVALUELENGTH	CFGBUF->ParameterValueLength
#define PARM_NAME_LENGTH 25 /* Maximum parameter name length */

void rndis_handle_config_parm(const char *data, int keyoffset, int valoffset, int keylen, int vallen)
{
    (void)data;
    (void)keyoffset;
    (void)valoffset;
    (void)keylen;
    (void)vallen;
}

void rndis_packetFilter(uint32_t newfilter)
{
    (void)newfilter;
}

void rndis_handle_set_msg(void  *pdev)
{
	rndis_set_cmplt_t *c;
	rndis_set_msg_t *m;
	rndis_Oid_t oid;

	c = (rndis_set_cmplt_t *)encapsulated_buffer;
	m = (rndis_set_msg_t *)encapsulated_buffer;

	/* Never have longer parameter names than PARM_NAME_LENGTH */
	/*
	char parmname[PARM_NAME_LENGTH+1];

	uint8_t i;
	int8_t parmlength;
	*/

	/* The parameter name seems to be transmitted in uint16_t, but */
	/* we want this in uint8_t. Hence have to throw out some info... */

	/*
	if (CFGBUF->ParameterNameLength > (PARM_NAME_LENGTH*2))
	{
		parmlength = PARM_NAME_LENGTH * 2;
	}
	else
	{
		parmlength = CFGBUF->ParameterNameLength;
	}

	i = 0;
	while (parmlength > 0)
	{
		// Convert from uint16_t to char array. 
		parmname[i] = (char)*(PARMNAME + 2*i); // FSE! FIX IT!
		parmlength -= 2;
		i++;
	}
	*/

	oid = m->Oid;
	c->MessageType = REMOTE_NDIS_SET_CMPLT;
	c->MessageLength = sizeof(rndis_set_cmplt_t);
	c->Status = RNDIS_STATUS_SUCCESS;

	switch (oid)
	{
		/* Parameters set up in 'Advanced' tab */
		case OID_GEN_RNDIS_CONFIG_PARAMETER:
			{
                rndis_config_parameter_t *p;
				char *ptr = (char *)m;
				ptr += sizeof(rndis_generic_msg_t);
				ptr += m->InformationBufferOffset;
				p = (rndis_config_parameter_t *)ptr;
				rndis_handle_config_parm(ptr, p->ParameterNameOffset, p->ParameterValueOffset, p->ParameterNameLength, p->ParameterValueLength);
			}
			break;

		/* Mandatory general OIDs */
		case OID_GEN_CURRENT_PACKET_FILTER:
			oid_packet_filter = *INFBUF;
			if (oid_packet_filter)
			{
				rndis_packetFilter(oid_packet_filter);
				rndis_state = rndis_data_initialized;
			} 
			else 
			{
				rndis_state = rndis_initialized;
			}
			break;

		case OID_GEN_CURRENT_LOOKAHEAD:
			break;

		case OID_GEN_PROTOCOL_OPTIONS:
			break;

		/* Mandatory 802_3 OIDs */
		case OID_802_3_MULTICAST_LIST:
			break;

		/* Power Managment: fails for now */
		case OID_PNP_ADD_WAKE_UP_PATTERN:
		case OID_PNP_REMOVE_WAKE_UP_PATTERN:
		case OID_PNP_ENABLE_WAKE_UP:
		default:
			c->Status = RNDIS_STATUS_FAILURE;
			break;
	}

	/* c->MessageID is same as before */
	DCD_EP_Tx(pdev, RNDIS_NOTIFICATION_IN_EP, (uint8_t *)"\x01\x00\x00\x00\x00\x00\x00\x00", 8);
	return;
}

static uint8_t usbd_rndis_ep0_recv(void  *pdev)
{
	switch (((rndis_generic_msg_t *)encapsulated_buffer)->MessageType)
	{
		case REMOTE_NDIS_INITIALIZE_MSG:
			{
				rndis_initialize_cmplt_t *m;
				m = ((rndis_initialize_cmplt_t *)encapsulated_buffer);
				/* m->MessageID is same as before */
				m->MessageType = REMOTE_NDIS_INITIALIZE_CMPLT;
				m->MessageLength = sizeof(rndis_initialize_cmplt_t);
				m->MajorVersion = RNDIS_MAJOR_VERSION;
				m->MinorVersion = RNDIS_MINOR_VERSION;
				m->Status = RNDIS_STATUS_SUCCESS;
				m->DeviceFlags = RNDIS_DF_CONNECTIONLESS;
				m->Medium = RNDIS_MEDIUM_802_3;
				m->MaxPacketsPerTransfer = 1;
				m->MaxTransferSize = RNDIS_RX_BUFFER_SIZE;
				m->PacketAlignmentFactor = 0;
				m->AfListOffset = 0;
				m->AfListSize = 0;
				rndis_state = rndis_initialized;
				DCD_EP_Tx(pdev, RNDIS_NOTIFICATION_IN_EP, (uint8_t *)"\x01\x00\x00\x00\x00\x00\x00\x00", 8);
			}
			break;

		case REMOTE_NDIS_QUERY_MSG:
			rndis_query(pdev);
			break;
			
		case REMOTE_NDIS_SET_MSG:
			rndis_handle_set_msg(pdev);
			break;

		case REMOTE_NDIS_RESET_MSG:
			{
				rndis_reset_cmplt_t * m;
				m = ((rndis_reset_cmplt_t *)encapsulated_buffer);
				rndis_state = rndis_uninitialized;
				m->MessageType = REMOTE_NDIS_RESET_CMPLT;
				m->MessageLength = sizeof(rndis_reset_cmplt_t);
				m->Status = RNDIS_STATUS_SUCCESS;
				m->AddressingReset = 1; /* Make it look like we did something */
			    /* m->AddressingReset = 0; - Windows halts if set to 1 for some reason */
				DCD_EP_Tx(pdev, RNDIS_NOTIFICATION_IN_EP, (uint8_t *)"\x01\x00\x00\x00\x00\x00\x00\x00", 8);
			}
			break;

		case REMOTE_NDIS_KEEPALIVE_MSG:
			{
				rndis_keepalive_cmplt_t * m;
				m = (rndis_keepalive_cmplt_t *)encapsulated_buffer;
				m->MessageType = REMOTE_NDIS_KEEPALIVE_CMPLT;
				m->MessageLength = sizeof(rndis_keepalive_cmplt_t);
				m->Status = RNDIS_STATUS_SUCCESS;
			}
			/* We have data to send back */
			DCD_EP_Tx(pdev, RNDIS_NOTIFICATION_IN_EP, (uint8_t *)"\x01\x00\x00\x00\x00\x00\x00\x00", 8);
			break;

		default:
			break;
	}
  return USBD_OK;
}

int sended = 0;

static __inline uint8_t usbd_cdc_transfer(void *pdev)
{
	if (sended != 0 || rndis_tx_ptr == NULL || rndis_tx_size <= 0) return USBD_OK;
	if (rndis_first_tx)
	{
		static uint8_t first[RNDIS_DATA_IN_SZ];
		rndis_data_packet_t *hdr;

		hdr = (rndis_data_packet_t *)first;
		memset(hdr, 0, sizeof(rndis_data_packet_t));
		hdr->MessageType = REMOTE_NDIS_PACKET_MSG;
		hdr->MessageLength = sizeof(rndis_data_packet_t) + rndis_tx_size;
		hdr->DataOffset = sizeof(rndis_data_packet_t) - offsetof(rndis_data_packet_t, DataOffset);
		hdr->DataLength = rndis_tx_size;

		sended = RNDIS_DATA_IN_SZ - sizeof(rndis_data_packet_t);
		if (sended > rndis_tx_size) sended = rndis_tx_size;
		memcpy(first + sizeof(rndis_data_packet_t), rndis_tx_ptr, sended);

		DCD_EP_Tx(pdev, RNDIS_DATA_IN_EP, (uint8_t *)&first, sizeof(rndis_data_packet_t) + sended);
	}
	else
	{
		int n = rndis_tx_size;
		if (n > RNDIS_DATA_IN_SZ) n = RNDIS_DATA_IN_SZ;
		DCD_EP_Tx(pdev, RNDIS_DATA_IN_EP, rndis_tx_ptr, n);
		sended = n;
	}
	return USBD_OK;
}

static uint8_t usbd_rndis_data_in(void *pdev, uint8_t epnum)
{
	epnum &= 0x0F;
	if (epnum == (RNDIS_DATA_IN_EP & 0x0F))
	{
		rndis_first_tx = false;
		rndis_sended += sended;
		rndis_tx_size -= sended;
		rndis_tx_ptr += sended;
		sended = 0;
		usbd_cdc_transfer(pdev);
	}
	return USBD_OK;
}

static void handle_packet(const char *data, int size)
{
	rndis_data_packet_t *p;
	p = (rndis_data_packet_t *)data;
	if (size < sizeof(rndis_data_packet_t)) return;
	if (p->MessageType != REMOTE_NDIS_PACKET_MSG || p->MessageLength != size) return;
	if (p->DataOffset + offsetof(rndis_data_packet_t, DataOffset) + p->DataLength != size)
	{
		usb_eth_stat.rxbad++;
		return;
	}
	usb_eth_stat.rxok++;
	if (rndis_rxproc != NULL)
		rndis_rxproc(&rndis_rx_buffer[p->DataOffset + offsetof(rndis_data_packet_t, DataOffset)], p->DataLength);
}

/* Data received on non-control Out endpoint */
static uint8_t usbd_rndis_data_out(void *pdev, uint8_t epnum)
{
	static int rndis_received = 0;
	if (epnum == RNDIS_DATA_OUT_EP)
	{
		PUSB_OTG_EP ep = &((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum];
		if (rndis_received + ep->xfer_count > RNDIS_RX_BUFFER_SIZE)
		{
			usb_eth_stat.rxbad++;
			rndis_received = 0;
		}
		else
		{
			if (rndis_received + ep->xfer_count <= RNDIS_RX_BUFFER_SIZE)
			{
				memcpy(&rndis_rx_buffer[rndis_received], usb_rx_buffer, ep->xfer_count);
				rndis_received += ep->xfer_count;
				if (ep->xfer_count != RNDIS_DATA_OUT_SZ)
				{
					handle_packet(rndis_rx_buffer, rndis_received);
					rndis_received = 0;
				}
			}
			else
			{
					rndis_received = 0;
					usb_eth_stat.rxbad++;
			}
		}
		DCD_EP_PrepareRx(pdev, RNDIS_DATA_OUT_EP, (uint8_t*)usb_rx_buffer, RNDIS_DATA_OUT_SZ);
	}
  return USBD_OK;
}

/* Start Of Frame event management */
static uint8_t usbd_rndis_sof(void *pdev)
{
	return usbd_cdc_transfer(pdev);
}

static uint8_t rndis_iso_in_incomplete(void *pdev)
{
	return usbd_cdc_transfer(pdev);
}

static uint8_t rndis_iso_out_incomplete(void *pdev)
{
	DCD_EP_PrepareRx(pdev, RNDIS_DATA_OUT_EP, (uint8_t*)usb_rx_buffer, RNDIS_DATA_OUT_SZ);
	return USBD_OK;
}

static uint8_t *usbd_rndis_get_cfg(uint8_t speed, uint16_t *length)
{
    *length = sizeof(usbd_cdc_CfgDesc);
    usbd_cdc_CfgDesc[2] = sizeof(usbd_cdc_CfgDesc) & 0xFF;
    usbd_cdc_CfgDesc[3] = (sizeof(usbd_cdc_CfgDesc) >> 8) & 0xFF;
    return usbd_cdc_CfgDesc;
}

bool rndis_can_send(void)
{
	return rndis_tx_size <= 0;
}

bool rndis_send(const void *data, int size)
{
	if (size <= 0 ||
		size > ETH_MAX_PACKET_SIZE ||
		rndis_tx_size > 0) return false;

	__disable_irq();
	rndis_first_tx = true;
	rndis_tx_ptr = (uint8_t *)data;
	rndis_tx_size = size;
	rndis_sended = 0;
	__enable_irq();

	return true;
}
