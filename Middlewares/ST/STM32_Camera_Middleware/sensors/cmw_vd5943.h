/**
  ******************************************************************************
  * @file    cmw_vd5943.h
  * @author  MDG Application Team
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef CMW_VD5943_H
#define CMW_VD5943_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include "cmw_camera.h"
#include "cmw_sensors_if.h"
#include "cmw_errno.h"
#include "vd1943.h"
#include "stm32n6xx_hal_dcmipp.h"
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_api.h"
#endif

#define VD5943_CUT1_3_CHIP_ID    0x53393430
#define VD5943_CUT1_4_CHIP_ID    0x53393431
#define CAMERA_VD5943_ADDRESS   0x20U

typedef struct
{
  uint16_t Address;
  VD1943_Ctx_t  ctx_driver;
#if !defined (CMW_USE_WITHOUT_ISP)
  ISP_HandleTypeDef hIsp;
#endif
  DCMIPP_HandleTypeDef *hdcmipp;
  uint8_t IsInitialized;
  int32_t (*Init)(void);
  int32_t (*DeInit)(void);
  int32_t (*WriteReg)(uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*ReadReg) (uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*GetTick) (void);
  void (*Delay)(uint32_t delay_in_ms);
  void (*ShutdownPin)(int value);
  void (*EnablePin)(int value);
} CMW_VD5943_t;

typedef struct
{
  CMW_PixelFormat_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD5943_config_t;

int32_t CMW_CAMERA_VD5943_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx,  DCMIPP_HandleTypeDef *hdcmipp,
                               CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP);
void CMW_VD5943_SetDefaultSensorValues(void *sensor_config);

#ifdef __cplusplus
}
#endif

#endif
