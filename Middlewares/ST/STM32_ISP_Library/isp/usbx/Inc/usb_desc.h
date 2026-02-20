/**
 ******************************************************************************
 * @file    usb_desc.h
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

#ifndef _USB_DESC_H_
#define _USB_DESC_H_

#include <stdint.h>

typedef struct {
  int is_hs;
  int width;
  int height;
  int fps;
  int payload_type;
  uint32_t dwMaxVideoFrameSize;
} uvc_desc_conf;

typedef struct {
  int is_hs;
} usb_desc_conf;

int usb_get_device_desc(void *p_dst, int dst_len, int idx_manufacturer, int idx_product, int idx_serial);
int usb_get_device_qualifier_desc(void *p_dst, int dst_len);
int usb_get_lang_string_desc(void *p_dst, int dst_len);
int usb_get_manufacturer_string_desc(void *p_dst, int dst_len);
int usb_get_product_string_desc(void *p_dst, int dst_len);
int usb_get_serial_string_desc(void *p_dst, int dst_len);
int usb_get_configuration_desc(void *p_dst, int dst_len, usb_desc_conf *p_conf);
int uvc_get_configuration_desc(void *p_dst, int dst_len, uvc_desc_conf *p_conf);
int uvc_get_device_desc(void *p_dst, int dst_len, int idx_manufacturer, int idx_product, int idx_serial);
#endif
