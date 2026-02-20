/**
 ******************************************************************************
 * @file    usb_uvc.h
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

#ifndef _USB_UVC_H_
#define _USV_UVC_H_

#include "usb_desc.h"

#ifndef USBL_PACKET_PER_MICRO_FRAME
#define USBL_PACKET_PER_MICRO_FRAME 1
#endif

#define UVC_BUFFER_NB                                  4

#define UVC_ISO_FS_MPS                                 1023
#define UVC_ISO_HS_MPS                                 (USBL_PACKET_PER_MICRO_FRAME * 1024)

#define UVC_INTERVAL(n)                               (10000000U/(n))

typedef struct
{
  uint16_t bmHint;
  uint8_t bFormatIndex;
  uint8_t bFrameIndex;
  uint32_t dwFrameInterval;
  uint16_t wKeyFrameRate;
  uint16_t wPFrameRate;
  uint16_t wCompQuality;
  uint16_t wCompWindowSize;
  uint16_t wDelay;
  uint32_t dwMaxVideoFrameSize;
  uint32_t dwMaxPayloadTransferSize;
  uint32_t dwClockFrequency;
  uint8_t bmFramingInfo;
  uint8_t bPreferedVersion;
  uint8_t bMinVersion;
  uint8_t bMaxVersion;
  uint8_t bUsage;
  uint8_t bBitDepthLuma;
  uint8_t bmSettings;
  uint8_t bMaxNumberOfRefFramesPlus1;
  uint16_t bmRateControlModes;
  uint64_t bmLayoutPerStream;
} __PACKED uvc_VideoControlTypeDef;

typedef enum {
  UVC_STATUS_STOP,
  UVC_STATUS_STREAMING,
} uvc_state_t;

typedef struct {
  /* usb request packet */
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
  /* info use by common code */
  int dwMaxPayloadTransferSize;
  /* ctx argument for cb functions */
  void *ctx;
  /* impl must implement following functions */
  int (*stop_streaming)(void *);
  int (*start_streaming)(void *);
  int (*send_data)(void *, uint8_t *, int);
  int (*receive_data)(void *, uint8_t *, int);
} uvc_setup_req_t;

typedef struct {
  int frame_index;
  uint8_t *cursor;
  int packet_nb;
  int packet_index;
  int last_packet_size;
  int prev_len;
  uint8_t *p_frame;
} uvc_on_fly_ctx;

typedef struct {
  uvc_desc_conf conf;
  int buffer_nb;
  uvc_state_t state;
  uint8_t *packet;
  uint32_t frame_start;
  int frame_period_in_ms;
  int is_starting;
  uint8_t *p_frame;
  int frame_size;
  uvc_on_fly_ctx on_fly_storage_ctx;
  uvc_on_fly_ctx *on_fly_ctx;
  uvc_VideoControlTypeDef UVC_VideoCommitControl;
  uvc_VideoControlTypeDef UVC_VideoProbeControl;
} uvc_ctx_t;

#endif
