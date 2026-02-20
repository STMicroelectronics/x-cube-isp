/**
 ******************************************************************************
 * @file    usbx_conf.c
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

#ifdef ISP_ENABLE_UVC
#define DEFAULT_UVC_PREVIEW_WIDTH 640
#define DEFAULT_UVC_PREVIEW_HEIGHT 480
uvc_config_t uvc_conf = {0};
#endif

/* We want to be the owner so we can decide about location of this structure */
#if defined(USBX_USE_DMA)
PCD_HandleTypeDef usbx_pcd_handle USBX_ATTR;
#else
PCD_HandleTypeDef usbx_pcd_handle;
#endif

static int usb_instance_init(PCD_HandleTypeDef *pcd_handle, PCD_TypeDef *pcd_instance)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
  int ret;

  /* First be sure HSE is turn on */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if (ret)
    return ret;

  /* configure usb */
  pcd_handle->Instance = pcd_instance;
  pcd_handle->Init.dev_endpoints = 9;
  pcd_handle->Init.speed = PCD_SPEED_HIGH;

#if defined(USBX_USE_DMA)
  pcd_handle->Init.dma_enable = ENABLE;
#else
  pcd_handle->Init.dma_enable = DISABLE;
#endif
  pcd_handle->Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
  pcd_handle->Init.Sof_enable = DISABLE;
  pcd_handle->Init.low_power_enable = DISABLE;
  pcd_handle->Init.lpm_enable = DISABLE;
  pcd_handle->Init.vbus_sensing_enable = DISABLE;
  pcd_handle->Init.use_dedicated_ep1 = DISABLE;
  pcd_handle->Init.use_external_vbus = DISABLE;
  ret = HAL_PCD_Init(pcd_handle);
  if (ret)
    return ret;

  /*
   * How to configure the USB FIFO:
   * https://community.st.com/t5/stm32-mcus/how-to-configure-usb-fifo-over-usb-otg-controller-on-stm32-in/ta-p/834987
   * STM32N6 has 4Kbytes of dedicated RAM for USB FiFo. Value set in the setYxFiFo are value in word (4bytes)
   */
#if defined(USBX_USE_DMA)
  HAL_PCDEx_SetRxFiFo(pcd_handle,     0xB3); /* RxFifo (w/ DMA descriptor) */
#else
  HAL_PCDEx_SetRxFiFo(pcd_handle,     0xA1); /* RxFifo (w/o DMA descriptor) */
#endif
  HAL_PCDEx_SetTxFiFo(pcd_handle, 0,  0x10); /* endpoint 0 => control */
#if defined(ISP_ENABLE_UVC)
#if defined(USBX_USE_DMA)
  HAL_PCDEx_SetTxFiFo(pcd_handle, 1, 0x28D); /* endpoint 1 */
  HAL_PCDEx_SetTxFiFo(pcd_handle, 2,  0xA0); /* endpoint 2 */
#else
  HAL_PCDEx_SetTxFiFo(pcd_handle, 1, 0x2BF); /* endpoint 1 */
  HAL_PCDEx_SetTxFiFo(pcd_handle, 2,  0x80); /* endpoint 2 */
#endif
  HAL_PCDEx_SetTxFiFo(pcd_handle, 3,  0x10); /* endpoint 3 */
#else
  HAL_PCDEx_SetTxFiFo(pcd_handle, 1, 0x100); /* endpoint 1 */
  HAL_PCDEx_SetTxFiFo(pcd_handle, 2,  0x10); /* endpoint 2 */
#endif

  return 0;
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
  assert(hpcd->Instance == USB1_OTG_HS);

  __HAL_RCC_PWR_CLK_ENABLE();

  /* Enable the VDD33USB independent USB 33 voltage monitor */
  HAL_PWREx_EnableVddUSBVMEN();

  /* Wait until VDD33USB is ready */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_USB33RDY) == 0U);

  /* Enable VDDUSB supply */
  HAL_PWREx_EnableVddUSB();

  /* Enable USB1 OTG clock */
  __HAL_RCC_USB1_OTG_HS_CLK_ENABLE();

  /* Set FSEL to 24 Mhz */
  USB1_HS_PHYC->USBPHYC_CR &= ~(0x7U << 0x4U);
  USB1_HS_PHYC->USBPHYC_CR |= (0x2U << 0x4U);

  /* Enable USB1 OTG PHY clock */
  __HAL_RCC_USB1_OTG_HS_PHY_CLK_ENABLE();

  HAL_NVIC_SetPriority(USB1_OTG_HS_IRQn, 6U, 0U);

  /* Enable USB OTG interrupt */
  HAL_NVIC_EnableIRQ(USB1_OTG_HS_IRQn);
}

#if defined(ISP_ENABLE_UVC)
void usb_uvc_init(int width, int height, int fps)
{
  uvc_conf.width = width;
  uvc_conf.height = height;
  uvc_conf.fps = fps;
}

int usb_uvc_show_frame(void *frame, int frame_size)
{
  return usbx_uvc_show_frame(frame, frame_size);
}
#endif /* ISP_ENABLE_UVC */

int usb_init(PCD_TypeDef *pcd_instance)
{
  int ret;

  ret = usb_instance_init(&usbx_pcd_handle, pcd_instance);
  if (ret)
    return ret;

#if defined(ISP_ENABLE_UVC)
  if (uvc_conf.width == 0 && uvc_conf.height == 0)
  {
    uvc_conf.width = DEFAULT_UVC_PREVIEW_WIDTH;
    uvc_conf.height = DEFAULT_UVC_PREVIEW_HEIGHT;
    uvc_conf.fps = 30;
  }

  ret = usbx_init(&usbx_pcd_handle, pcd_instance, &uvc_conf);
  if (ret)
    return ret;
#else
  ret = usbx_init(&usbx_pcd_handle, pcd_instance, NULL);
  if (ret)
    return ret;
#endif /* ISP_ENABLE_UVC */

  return HAL_PCD_Start(&usbx_pcd_handle);
}
