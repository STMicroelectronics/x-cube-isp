/**
 ******************************************************************************
 * @file    usbx_conf.h
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

#ifndef _USBX_CONF_H_
#define _USBX_CONF_H_

#include "stm32n6xx_hal.h"

int usb_init(PCD_TypeDef *pcd_instance);
#if defined(ISP_ENABLE_UVC)
void usb_uvc_init(int widht, int height, int fps);
int usb_uvc_show_frame(void *frame, int frame_size);
#endif
#endif
