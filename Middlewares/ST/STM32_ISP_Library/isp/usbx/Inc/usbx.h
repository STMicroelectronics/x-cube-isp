/**
 ******************************************************************************
 * @file    usbx.h
 * @author  AIS Application Team
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef _USBX_H_
#define _USBX_H_

#include "usbx_conf.h"
#include "usb_uvc.h"

#define USBX_ALIGN_32 __attribute__ ((aligned (32)))
#define USBX_UNCACHED __attribute__ ((section (".uncached_bss")))
#if defined(USBX_USE_DMA)
#define USBX_ATTR USBX_UNCACHED USBX_ALIGN_32
#else
#define USBX_ATTR
#endif


#define USBX_MEM_SIZE                                  (32 * 1024)
#define USBX_MAX_CONF_LEN                              512
#define USBX_MAX_STRING_LEN                            512
#define USBX_MAX_LANGID_LEN                            2

#define USB_REQ_TYPE_STANDARD                          0x00U
#define USB_REQ_TYPE_CLASS                             0x20U
#define USB_REQ_TYPE_VENDOR                            0x40U
#define USB_REQ_TYPE_MASK                              0x60U

#define USB_REQ_RECIPIENT_DEVICE                       0x00U
#define USB_REQ_RECIPIENT_INTERFACE                    0x01U
#define USB_REQ_RECIPIENT_ENDPOINT                     0x02U
#define USB_REQ_RECIPIENT_MASK                         0x03U

#ifndef USB_REQ_SET_INTERFACE
#define USB_REQ_SET_INTERFACE                          0x0BU
#endif

typedef struct {
  int width;
  int height;
  int fps;
} uvc_config_t;

int usbx_init(PCD_HandleTypeDef *pcd_handle, PCD_TypeDef *pcd_instance, uvc_config_t *p_uvc_conf);
void usbx_warn_dump_status(uint8_t on_going);
void usbx_write(unsigned char *msg, uint32_t len);
#if defined(ISP_ENABLE_UVC)
int usbx_uvc_show_frame(void *frame, int frame_size);
#endif

#endif
