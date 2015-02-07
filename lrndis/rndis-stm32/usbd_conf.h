/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   USB Device configuration file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

#include "usb_conf.h"

#define RNDIS_NOTIFICATION_IN_EP 0x81
#define RNDIS_DATA_IN_EP         0x82
#define RNDIS_DATA_OUT_EP        0x03

#define RNDIS_NOTIFICATION_IN_SZ 0x08
#define RNDIS_DATA_IN_SZ         0x40
#define RNDIS_DATA_OUT_SZ        0x40

#define USBD_CFG_MAX_NUM         1
#define USBD_ITF_MAX_NUM         1
#define USB_MAX_STR_DESC_SIZ     64

#define USBD_SELF_POWERED               

#endif
