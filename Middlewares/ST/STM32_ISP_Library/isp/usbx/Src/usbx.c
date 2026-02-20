/**
 ******************************************************************************
 * @file    usbx.c
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


#include <assert.h>
#include "usbx.h"
#include "usbx_codes.h"
#include "stm32n6xx_hal.h"
#include "usb_desc.h"
#include "usb_desc_internal.h"
#include "ux_api.h"
#include "ux_dcd_stm32.h"
#include "ux_device_class_video.h"
#include "ux_device_class_cdc_acm.h"

#include "isp_tool_com.h"

static uint8_t usbx_mem_pool[USBX_MEM_SIZE] USBX_ALIGN_32;
#if defined(USBX_USE_DMA)
static uint8_t usbx_mem_pool_uncached[USBX_MEM_SIZE] USBX_ATTR;
#endif

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

static uint8_t usb_desc_fs[USBX_MAX_CONF_LEN] USBX_ATTR;
static uint8_t usb_desc_hs[USBX_MAX_CONF_LEN] USBX_ATTR;
static uint8_t usb_dev_strings[USBX_MAX_STRING_LEN];
static uint8_t usb_dev_langid[USBX_MAX_LANGID_LEN];

/* Use read global buffer instead of local (memory issue) */
#define RX_PACKET_SIZE 512
uint8_t rx_buffer[RX_PACKET_SIZE];


/*
 * UVC functions when UVC is activated
 */
#if defined (ISP_ENABLE_UVC)

static uvc_ctx_t uvc_ctx = {0};
static uint8_t uvc_packet[UVC_ISO_HS_MPS] USBX_ATTR;

static int uvc_fps_ok(uvc_ctx_t *p_ctx)
{
  if (HAL_GetTick() - p_ctx->frame_start >= p_ctx->frame_period_in_ms)
  {
    return 1;
  }

  return 0;
}

static void uvc_fill_sent_data(uvc_ctx_t *p_ctx, uvc_on_fly_ctx *on_fly_ctx, uint8_t *p_frame, int fsize,
                             int packet_size)
{
  on_fly_ctx->packet_nb = (fsize + packet_size - 1) / (packet_size - 2);
  on_fly_ctx->last_packet_size = fsize % (packet_size - 2);
  if (!on_fly_ctx->last_packet_size)
  {
    on_fly_ctx->packet_nb--;
    on_fly_ctx->last_packet_size = packet_size - 2;
  }
  on_fly_ctx->cursor = p_frame;
  p_ctx->packet[1] ^= 1;

  p_ctx->is_starting = 0;
  p_ctx->frame_start = HAL_GetTick();
}

static uvc_on_fly_ctx *uvc_start_selected_raw(uvc_ctx_t *p_ctx, int packet_size)
{
  uvc_on_fly_ctx *on_fly_ctx = &p_ctx->on_fly_storage_ctx;

  on_fly_ctx->frame_index = -1;
  uvc_fill_sent_data(p_ctx, on_fly_ctx, p_ctx->p_frame, p_ctx->frame_size, packet_size);

  __DMB();
  p_ctx->p_frame = NULL;

  return on_fly_ctx;
}

static int uvc_handle_set_itf_setup_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  uint16_t wIndex = req->wIndex;
  uint16_t wValue = req->wValue;
  int ret;

  /* Only handle control on vs itf */
  if (wIndex != 1)
    return -1;

  switch (wValue) {
  case 0:
    /* set alternate itf to zero => stop streaming */
    ret = req->stop_streaming(req->ctx);
    break;
  case 1:
    /* set alternate itf to one => start streaming */
    ret = req->start_streaming(req->ctx);
    break;
  default:
    assert(0);
    return -1;
  }

  return ret;
}

static int uvc_handle_std_itf_setup_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  int ret;

  switch (req->bRequest) {
  case USB_REQ_SET_INTERFACE:
    ret = uvc_handle_set_itf_setup_request(p_ctx, req);
    break;
  default:
    assert(0);
    return -1;
  }

  return ret;
}

uint32_t uvc_compute_dwMaxVideoFrameSize(uvc_desc_conf *conf)
{
	/* We only support UNCOMPRESSED YUY2 */
	uint32_t res = conf->width * conf->height * 2;
	return res;
}

static int uvc_handle_probe_control_get_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  p_ctx->UVC_VideoProbeControl.bmHint = 0;
  p_ctx->UVC_VideoProbeControl.dwFrameInterval = UVC_INTERVAL(p_ctx->conf.fps);
  p_ctx->UVC_VideoProbeControl.dwMaxVideoFrameSize = uvc_compute_dwMaxVideoFrameSize(&p_ctx->conf);
  p_ctx->UVC_VideoProbeControl.dwMaxPayloadTransferSize = req->dwMaxPayloadTransferSize;
  p_ctx->UVC_VideoProbeControl.dwClockFrequency = 48000000;
  /* should not zero but not clear what value is possible for uncompressed format */
  p_ctx->UVC_VideoProbeControl.bPreferedVersion = 0x00U;
  p_ctx->UVC_VideoProbeControl.bMinVersion = 0x00U;
  p_ctx->UVC_VideoProbeControl.bMaxVersion = 0x00U;

  return req->send_data(req->ctx, (uint8_t *)&p_ctx->UVC_VideoProbeControl,
                        MIN(req->wLength, sizeof(p_ctx->UVC_VideoProbeControl)));
}

static int uvc_handle_probe_control_set_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  return req->receive_data(req->ctx, (uint8_t *)&p_ctx->UVC_VideoProbeControl,
                           MIN(req->wLength, sizeof(p_ctx->UVC_VideoProbeControl)));
}

static int uvc_handle_probe_control_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  int ret;

  switch (req->bRequest) {
  case UVC_GET_DEF:
  case UVC_GET_MIN:
  case UVC_GET_MAX:
  case UVC_GET_CUR:
    ret = uvc_handle_probe_control_get_request(p_ctx, req);
    break;
  case UVC_SET_CUR:
    ret = uvc_handle_probe_control_set_request(p_ctx, req);
    break;
  default:
    assert(0);
    return -1;
  }

  return ret;
}

static int uvc_handle_probe_commit_set_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  return req->receive_data(req->ctx, (uint8_t *)&p_ctx->UVC_VideoCommitControl,
                           MIN(req->wLength, sizeof(p_ctx->UVC_VideoCommitControl)));
}

static int uvc_handle_commit_control_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  int ret;

  switch (req->bRequest) {
  case UVC_SET_CUR:
    ret = uvc_handle_probe_commit_set_request(p_ctx, req);
    break;
  default:
    assert(0);
    return -1;
  }

  return ret;
}

static int uvc_handle_class_itf_setup_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  int itf_nb = req->wIndex;
  int cs = req->wValue >> 8;
  int ret;

  /* no control for vc itf */
  if (!itf_nb)
    return -1;

  switch (cs) {
  case VS_PROBE_CONTROL_CS:
    ret = uvc_handle_probe_control_request(p_ctx, req);
    break;
  case VS_COMMIT_CONTROL_CS:
    ret = uvc_handle_commit_control_request(p_ctx, req);
    break;
  default:
    assert(0);
    return -1;
  }

  return ret;
}

static int uvc_handle_itf_setup_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  int ret;

  switch (req->bmRequestType & USB_REQ_TYPE_MASK) {
  case USB_REQ_TYPE_STANDARD:
    ret = uvc_handle_std_itf_setup_request(p_ctx, req);
    break;
  case USB_REQ_TYPE_CLASS:
    ret = uvc_handle_class_itf_setup_request(p_ctx, req);
    break;
  default:
    assert(0);
    return -1;
  }

  return ret;
}

/* internal API for common code */
uvc_on_fly_ctx *uvs_start_new_frame_transmission(uvc_ctx_t *p_ctx, int packet_size)
{
  int is_fps_ok = uvc_fps_ok(p_ctx);

  if (p_ctx->is_starting == 0 && !is_fps_ok)
    return NULL;

  if (!p_ctx->p_frame)
    return NULL;

  return uvc_start_selected_raw(p_ctx, packet_size);
}

int uvc_handle_setup_request(uvc_ctx_t *p_ctx, uvc_setup_req_t *req)
{
  int ret;

  switch (req->bmRequestType & USB_REQ_RECIPIENT_MASK) {
  case USB_REQ_RECIPIENT_INTERFACE:
    ret = uvc_handle_itf_setup_request(p_ctx, req);
    break;
  default:
    assert(0);
    return -1;
  }

  return ret;
}

void uvc_update_on_fly_ctx(uvc_ctx_t *p_ctx, int len)
{
  uvc_on_fly_ctx *on_fly_ctx = p_ctx->on_fly_ctx;

  assert(on_fly_ctx);

  on_fly_ctx->packet_index = (on_fly_ctx->packet_index + 1) % on_fly_ctx->packet_nb;
  on_fly_ctx->cursor += len - 2;
  on_fly_ctx->prev_len = len;

  if (on_fly_ctx->packet_index)
    return ;

  /* We reach last packet */
  p_ctx->on_fly_ctx = NULL;
}

void uvc_abort_on_fly_ctx(uvc_ctx_t *p_ctx)
{
  uvc_on_fly_ctx *on_fly_ctx = p_ctx->on_fly_ctx;

  assert(on_fly_ctx);

  p_ctx->on_fly_ctx = NULL;
}

static void uvc_vc_init(uvc_VideoControlTypeDef *vc)
{
  vc->bmHint = 0x0000U;
  vc->bFormatIndex = 0x01U;
  vc->bFrameIndex = 0x01U;
  vc->dwFrameInterval = UVC_INTERVAL(30);
  vc->wKeyFrameRate = 0x0000U;
  vc->wPFrameRate = 0x0000U;
  vc->wCompQuality = 0x0000U;
  vc->wCompWindowSize = 0x0000U;
  vc->wDelay = 0x0000U;
  vc->dwMaxVideoFrameSize = 0x0000U;
  vc->dwMaxPayloadTransferSize = 0x00000000U;
  vc->dwClockFrequency = 0x00000000U;
  vc->bmFramingInfo = 0x00U;
  vc->bPreferedVersion = 0x00U;
  vc->bMinVersion = 0x00U;
  vc->bMaxVersion = 0x00U;
}

static int uvc_is_hs()
{
  assert(_ux_system_slave);

  return _ux_system_slave->ux_system_slave_speed == UX_HIGH_SPEED_DEVICE;
}

static uvc_ctx_t *uvc_usbx_get_ctx_from_video_instance(UX_DEVICE_CLASS_VIDEO *video_instance)
{
  /* Should use ux_device_class_video_ioctl + UX_DEVICE_CLASS_VIDEO_IOCTL_GET_ARG. But direct access is faster */

  return video_instance->ux_device_class_video_callbacks.ux_device_class_video_arg;
}

static uvc_ctx_t *uvc_usbx_get_ctx_from_stream(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream)
{
  return uvc_usbx_get_ctx_from_video_instance(stream->ux_device_class_video_stream_video);
}

static void uvc_send_packet(uvc_ctx_t *p_ctx, UX_DEVICE_CLASS_VIDEO_STREAM *stream, int len)
{
  ULONG buffer_length;
  UCHAR *buffer;
  int ret;

  ret = ux_device_class_video_write_payload_get(stream, &buffer, &buffer_length);
  assert(ret == UX_SUCCESS);
  assert(buffer_length >= len);

  memcpy(buffer, p_ctx->packet, len);
  ret = ux_device_class_video_write_payload_commit(stream, len);
  assert(ret == UX_SUCCESS);
}

static void uvc_data_in(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream)
{
  int packet_size = uvc_is_hs() ? UVC_ISO_HS_MPS : UVC_ISO_FS_MPS;
  uvc_ctx_t *p_ctx = uvc_usbx_get_ctx_from_stream(stream);
  uvc_on_fly_ctx *on_fly_ctx;
  int len;

  if (p_ctx->state != UVC_STATUS_STREAMING)
  {
    return ;
  }

  /* select new frame */
  if (!p_ctx->on_fly_ctx)
    p_ctx->on_fly_ctx = uvs_start_new_frame_transmission(p_ctx, packet_size);

  if (!p_ctx->on_fly_ctx) {
    uvc_send_packet(p_ctx, stream, 2);
    return ;
  }

  /* Send next frame packet */
  on_fly_ctx = p_ctx->on_fly_ctx;
  len = on_fly_ctx->packet_index == (on_fly_ctx->packet_nb - 1) ? on_fly_ctx->last_packet_size + 2 : packet_size;
  memcpy(&p_ctx->packet[2], on_fly_ctx->cursor, len - 2);
  uvc_send_packet(p_ctx, stream, len);

  uvc_update_on_fly_ctx(p_ctx, len);
}

static void uvc_stop_streaming(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream)
{
  uvc_ctx_t *p_ctx = uvc_usbx_get_ctx_from_stream(stream);

  p_ctx->state = UVC_STATUS_STOP;
  if (p_ctx->on_fly_ctx)
    uvc_abort_on_fly_ctx(p_ctx);

  p_ctx->p_frame = NULL;
  p_ctx->frame_size = 0;
  p_ctx->is_starting = 0;
}

static void uvc_start_streaming(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream)
{
    uvc_ctx_t *p_ctx = uvc_usbx_get_ctx_from_stream(stream);
    int i;

    if (!p_ctx)
        return;

    /* Initialize packet header (UVC payload header) */
    if (p_ctx->packet) {
        p_ctx->packet[0] = 2; /* Example: header length or flags */
        p_ctx->packet[1] = 0; /* Example: frame ID or flags */
    }

    /* Initialize timing and state */
    p_ctx->frame_start = HAL_GetTick() - p_ctx->frame_period_in_ms;
    p_ctx->is_starting = 1;
    p_ctx->state = UVC_STATUS_STREAMING;

    /* Start data-in process for each buffer */
    for (i = 0; i < p_ctx->buffer_nb; i++)
        uvc_data_in(stream);
}

static uint8_t uvc_is_streaming = 0;
static void uvc_stream_change(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream, ULONG alternate_setting)
{
  int ret;

  if (alternate_setting == 0) {
    uvc_stop_streaming(stream);
    uvc_is_streaming = 0;
    return ;
  }

  uvc_start_streaming(stream);
  uvc_is_streaming = 1;

  ret = ux_device_class_video_transmission_start(stream);
  assert(ret == UX_SUCCESS);
}

static int uvc_usbx_stop_streaming(void *ctx)
{
  /* we should never reach this function */
  assert(0);

  return -1;
}

static int uvc_usbx_start_streaming(void *ctx)
{
  /* we should never reach this function */
  assert(0);

  return -1;
}

static int uvc_usbx_send_data(void *ctx, uint8_t *data, int length)
{
  UX_SLAVE_TRANSFER *transfer = ctx;
  uint8_t *buffer = transfer->ux_slave_transfer_request_data_pointer;
  int ret;

  memcpy(buffer, data, length);
  ret = ux_device_stack_transfer_request(transfer, length, length);

  return ret;
}

static int uvc_usbx_receive_data(void *ctx, uint8_t *data, int length)
{
  UX_SLAVE_TRANSFER *transfer = ctx;

  if (transfer->ux_slave_transfer_request_actual_length != length)
    return UX_ERROR;

  memcpy(data, transfer->ux_slave_transfer_request_data_pointer, length);

  return UX_SUCCESS;
}


static UX_THREAD *UVC_thread;

static UINT uvc_stream_request(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream, UX_SLAVE_TRANSFER *transfer)
{
  uvc_ctx_t *p_ctx = uvc_usbx_get_ctx_from_stream(stream);
  uvc_setup_req_t req;
  int ret;

  UVC_thread = &stream -> ux_device_class_video_stream_thread;

  req.bmRequestType = transfer->ux_slave_transfer_request_setup[UX_SETUP_REQUEST_TYPE];
  req.bRequest = transfer->ux_slave_transfer_request_setup[UX_SETUP_REQUEST];
  req.wValue = ux_utility_short_get(transfer->ux_slave_transfer_request_setup + UX_SETUP_VALUE);
  req.wIndex = ux_utility_short_get(transfer->ux_slave_transfer_request_setup + UX_SETUP_INDEX);
  req.wLength = ux_utility_short_get(transfer->ux_slave_transfer_request_setup + UX_SETUP_LENGTH);
  req.dwMaxPayloadTransferSize = uvc_is_hs() ? UVC_ISO_HS_MPS : UVC_ISO_FS_MPS;
  req.ctx = transfer;
  req.stop_streaming = uvc_usbx_stop_streaming;
  req.start_streaming = uvc_usbx_start_streaming;
  req.send_data = uvc_usbx_send_data;
  req.receive_data = uvc_usbx_receive_data;

  ret = uvc_handle_setup_request(p_ctx, &req);

  return ret ? UX_ERROR : UX_SUCCESS;
}

static VOID uvc_stream_payload_done(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream, ULONG length)
{
  uvc_data_in(stream);
}

static VOID uvc_instance_activate(VOID *video_instance)
{
  uvc_ctx_t *p_ctx = uvc_usbx_get_ctx_from_video_instance(video_instance);

  p_ctx->state = UVC_STATUS_STOP;
}

static VOID uvc_instance_deactivate(VOID *video_instance)
{
  ;
}
#endif /* ISP_ENABLE_UVC */


/*
 * CDC ACM functions
 */
static UX_SLAVE_CLASS_CDC_ACM *cdc_acm;

VOID cdc_acm_activate(VOID *cdc_acm_instance)
{
  printf("%s %p\r\n", __func__, cdc_acm_instance);
  cdc_acm = cdc_acm_instance;
}

VOID cdc_acm_deactivate(VOID *cdc_acm_instance)
{
  cdc_acm = NULL;
  printf("%s %p\r\n", __func__, cdc_acm_instance);
}


VOID cdc_acm_parameter_change(VOID *cdc_acm_instance)
{
  UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARAMETER line_coding;
  UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER line_state;
  UX_SLAVE_TRANSFER *transfer_request;
  UX_SLAVE_DEVICE *device;
  ULONG request;
  int ret;

  device = &_ux_system_slave -> ux_system_slave_device;
  transfer_request = &device -> ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;
  request = *(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_REQUEST);

  switch (request) {
  case UX_SLAVE_CLASS_CDC_ACM_SET_LINE_CODING:
    ret = ux_device_class_cdc_acm_ioctl(cdc_acm_instance, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING, &line_coding);
    assert(ret == 0);
    printf("Set %d bps\r\n", (int) line_coding.ux_slave_class_cdc_acm_parameter_baudrate);
    break;
  case UX_SLAVE_CLASS_CDC_ACM_SET_CONTROL_LINE_STATE:
    ret = ux_device_class_cdc_acm_ioctl(cdc_acm_instance, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE, &line_state);
    assert(ret == 0);
    /* printf("Set line state rts = %d / dtr = %d\r\n", (int) line_state.ux_slave_class_cdc_acm_parameter_rts,
                                                   (int) line_state.ux_slave_class_cdc_acm_parameter_dtr);*/
    break;
  default:
    printf("Unsupported request 0x%08x (%d)\r\n", (int)request, (int)request);
  }
}


/*
 * USBX functions
 */
static int usbx_extract_string(uint8_t langid[2], int index, uint8_t *string_desc, uint8_t *p_dst, int dst_len)
{
  int str_len = (string_desc[0] - 2) / 2;
  int i;

  if (dst_len < str_len + 4)
    return -1;

  p_dst[0] = langid[0];
  p_dst[1] = langid[1];
  p_dst[2] = index;
  p_dst[3] = str_len;
  for (i = 0; i < str_len; i++)
    p_dst[4 + i] = string_desc[2 + 2 * i];

  return str_len + 4;
}

static int usbx_build_dev_strings(uint8_t langid[2], uint8_t *p_dst, int dst_len)
{
  uint8_t string_desc[128];
  int res = 0;
  int len;

  len = usb_get_manufacturer_string_desc(string_desc, sizeof(string_desc));
  if (len < 0)
    return len;
  res += usbx_extract_string(langid, 1, string_desc, &p_dst[res], dst_len - res);
  if (res < 0)
    return 0;

  len = usb_get_product_string_desc(string_desc, sizeof(string_desc));
  if (len < 0)
    return len;
  res += usbx_extract_string(langid, 2, string_desc, &p_dst[res], dst_len - res);
  if (res < 0)
    return 0;

  len = usb_get_serial_string_desc(string_desc, sizeof(string_desc));
  if (len < 0)
    return len;
  res += usbx_extract_string(langid, 3, string_desc, &p_dst[res], dst_len - res);
  if (res < 0)
    return 0;

  return res;
}

static TX_THREAD usbx_read_thread;
static uint8_t ubsx_read_thread_stack[4096];

static void usbx_read_thread_fct(ULONG arg)
{
  int ret;
  ULONG rx_len;

  while (1) {
    if (!cdc_acm)
      goto wait_few_ms;

    ret = ux_device_class_cdc_acm_read(cdc_acm, rx_buffer, RX_PACKET_SIZE, &rx_len);
    if (ret != UX_SUCCESS)
      goto wait_few_ms;
    if (!rx_len)
      goto wait_few_ms;

    ISP_ToolCom_ReceivedCb(rx_buffer, rx_len);
    continue;

    wait_few_ms:
      HAL_Delay(1);
  }

  assert(0);
}

void usbx_warn_dump_status(uint8_t on_going)
{
#if defined (ISP_ENABLE_UVC)
  if (on_going)
  {
    if (uvc_is_streaming)
    {
      /* In case UVC is streaming */
      /* If dump is just started, suspend the UVC thread */
      _ux_utility_thread_suspend(UVC_thread);
    }
  }
  else
  {
    if (uvc_is_streaming)
    {
      /* If UVC was streaming before the dump procedure*/
      /* Once dump is finished, resume the UVC thread */
      _ux_utility_thread_resume(UVC_thread);
    }
  }
#endif /* ISP_ENABLE_UVC */
}

void usbx_write(unsigned char *msg, uint32_t len)
{
  UX_SLAVE_DEVICE *device = &_ux_system_slave->ux_system_slave_device;
  UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER line_state;
  ULONG len_send;
  int ret;

  if (device->ux_slave_device_state != UX_DEVICE_CONFIGURED)
    return ;

  if (!cdc_acm)
    return ;

  ret = ux_device_class_cdc_acm_ioctl(cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE, &line_state);
  assert(ret == 0);
  if (!line_state.ux_slave_class_cdc_acm_parameter_dtr)
    return ;

  ret = ux_device_class_cdc_acm_write(cdc_acm, msg, len, &len_send);
  assert(ret == 0);
  assert(len_send == len);
}

#if defined (ISP_ENABLE_UVC)
int usbx_uvc_show_frame(void *frame, int frame_size)
{
  if (uvc_ctx.state == UVC_STATUS_STOP) {
	uvc_ctx.p_frame = NULL;
    return -1;
  }

  if (uvc_ctx.state != UVC_STATUS_STREAMING)
    return -2;
  if (uvc_ctx.p_frame)
    return -1;
  if (!frame)
    return -1;
  if (!frame_size)
    return -1;

  uvc_ctx.frame_size = frame_size;
  __DMB();
  uvc_ctx.p_frame = frame;

  return 0;
}
#endif /* ISP_ENABLE_UVC */

int usbx_init(PCD_HandleTypeDef *pcd_handle, PCD_TypeDef *pcd_instance, uvc_config_t *p_uvc_conf)
{
  uint8_t lang_string_desc[4];
  int usb_dev_strings_len;
  int usb_dev_langid_len;
  int usb_desc_hs_len;
  int usb_desc_fs_len;
  int len;
  int ret;

#if defined(USBX_USE_DMA)
  ret = ux_system_initialize(usbx_mem_pool, USBX_MEM_SIZE, usbx_mem_pool_uncached, USBX_MEM_SIZE);
#else
  ret = ux_system_initialize(usbx_mem_pool, USBX_MEM_SIZE, UX_NULL, 0);
#endif
  if (ret)
    return ret;

#if defined (ISP_ENABLE_UVC)
  /* Init uvc context with */
  uvc_ctx.conf.width = p_uvc_conf->width;
  uvc_ctx.conf.height= p_uvc_conf->height;
  uvc_ctx.conf.fps = p_uvc_conf->fps;
  uvc_ctx.buffer_nb = UVC_BUFFER_NB;
  uvc_ctx.packet = uvc_packet;
  uvc_ctx.frame_period_in_ms = 1000 / p_uvc_conf->fps;
  uvc_vc_init(&uvc_ctx.UVC_VideoCommitControl);
  uvc_vc_init(&uvc_ctx.UVC_VideoProbeControl);

  uvc_desc_conf desc_conf_uvc = { 0 };
  UX_DEVICE_CLASS_VIDEO_STREAM_PARAMETER vsp[1] = { 0 };
  UX_DEVICE_CLASS_VIDEO_PARAMETER vp = { 0 };
  desc_conf_uvc.width = uvc_ctx.conf.width;
  desc_conf_uvc.height = uvc_ctx.conf.height;
  desc_conf_uvc.fps = uvc_ctx.conf.fps;

  /* Build High Speed configuration descriptor */
  desc_conf_uvc.is_hs = 1;
  usb_desc_hs_len = uvc_get_device_desc(usb_desc_hs, sizeof(usb_desc_hs), 1, 2, 3);
  assert(usb_desc_hs_len > 0);

  len = uvc_get_configuration_desc(&usb_desc_hs[usb_desc_hs_len], sizeof(usb_desc_hs) - usb_desc_hs_len, &desc_conf_uvc);
  assert(len > 0);
  usb_desc_hs_len += len;

  /* Build Full Speed configuration descriptor */
  desc_conf_uvc.is_hs = 0;
  usb_desc_fs_len = uvc_get_device_desc(usb_desc_fs, sizeof(usb_desc_fs), 1, 2, 3);
  assert(usb_desc_fs_len > 0);

  len = uvc_get_configuration_desc(&usb_desc_fs[usb_desc_fs_len], sizeof(usb_desc_fs) - usb_desc_fs_len, &desc_conf_uvc);
  assert(len > 0);
  usb_desc_fs_len += len;
#else
  usb_desc_conf desc_conf_usb = { 0 };
  /* Build High Speed configuration descriptor */
  desc_conf_usb.is_hs = 1;
  usb_desc_hs_len = usb_get_device_desc(usb_desc_hs, sizeof(usb_desc_hs), 1, 2, 3);
  assert(usb_desc_hs_len > 0);

  len = usb_get_configuration_desc(&usb_desc_hs[usb_desc_hs_len], sizeof(usb_desc_hs) - usb_desc_hs_len, &desc_conf_usb);
  assert(len > 0);
  usb_desc_hs_len += len;

  /* Build Full Speed configuration descriptor */
  desc_conf_usb.is_hs = 0;
  usb_desc_fs_len = usb_get_device_desc(usb_desc_fs, sizeof(usb_desc_fs), 1, 2, 3);
  assert(usb_desc_fs_len > 0);

  len = usb_get_configuration_desc(&usb_desc_fs[usb_desc_fs_len], sizeof(usb_desc_fs) - usb_desc_fs_len, &desc_conf_usb);
  assert(len > 0);
  usb_desc_fs_len += len;
#endif /* ISP_ENABLE_UVC */

  len = usb_get_lang_string_desc(lang_string_desc, sizeof(lang_string_desc));
  assert(len == sizeof(lang_string_desc));
  usb_dev_langid[0] = lang_string_desc[2];
  usb_dev_langid[1] = lang_string_desc[3];
  usb_dev_langid_len = 2;

  usb_dev_strings_len = usbx_build_dev_strings(usb_dev_langid, usb_dev_strings, sizeof(usb_dev_strings));
  assert(usb_dev_strings_len > 0);

  ret = ux_device_stack_initialize(usb_desc_hs, usb_desc_hs_len,
                                   usb_desc_fs, usb_desc_fs_len,
                                   usb_dev_strings, usb_dev_strings_len,
                                   usb_dev_langid, usb_dev_langid_len, NULL);
  if (ret)
    return ret;

  UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_parameter;
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_activate   = cdc_acm_activate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_deactivate = cdc_acm_deactivate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_parameter_change    = cdc_acm_parameter_change;

#if defined (ISP_ENABLE_UVC)
  vsp[0].ux_device_class_video_stream_parameter_thread_stack_size = 0;
  vsp[0].ux_device_class_video_stream_parameter_thread_entry = ux_device_class_video_write_thread_entry;
  vsp[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_change = uvc_stream_change;
  vsp[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_request = uvc_stream_request;
  vsp[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_payload_done = uvc_stream_payload_done;
  vsp[0].ux_device_class_video_stream_parameter_max_payload_buffer_nb = UVC_BUFFER_NB;
  vsp[0].ux_device_class_video_stream_parameter_max_payload_buffer_size = UVC_ISO_HS_MPS;
  vp.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_activate = uvc_instance_activate;
  vp.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_deactivate = uvc_instance_deactivate;
  vp.ux_device_class_video_parameter_callbacks.ux_device_class_video_request = NULL;
  vp.ux_device_class_video_parameter_callbacks.ux_device_class_video_arg = &uvc_ctx;
  vp.ux_device_class_video_parameter_streams_nb = 1;
  vp.ux_device_class_video_parameter_streams = vsp;
  /* Register first Video instance corresponding to Interface 0 */
  ret = ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry, 1, 0, &vp);
  if (ret)
    return ret;
  /* Register first CDC instance corresponding to Interface 2 */
  ret = ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry, 1, 2, &cdc_acm_parameter);
  assert(ret == 0);

#else
  /* Register first CDC instance corresponding to Interface 0 */
  ret = ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry, 1, 0, &cdc_acm_parameter);
  assert(ret == 0);
#endif

  /* Create a thread to read incoming USB data */
  const UINT priority = TX_MAX_PRIORITIES / 2 - 1;
  const ULONG time_slice = 10;
  ret = tx_thread_create(&usbx_read_thread, "read", usbx_read_thread_fct, 0, ubsx_read_thread_stack,
                         sizeof(ubsx_read_thread_stack), priority, priority, time_slice, TX_AUTO_START);
  assert(ret == TX_SUCCESS);

  return ux_dcd_stm32_initialize((ULONG)pcd_instance, (ULONG)pcd_handle);
}
