/**
  ******************************************************************************
  * @file    cmw_imx335.h
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

#ifndef CMW_IMX335
#define CMW_IMX335

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include "cmw_camera.h"
#include "cmw_sensors_if.h"
#include "cmw_errno.h"
#include "imx335.h"
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_api.h"
#endif

#define CAMERA_IMX335_ADDRESS          0x34U

typedef struct
{
  uint16_t Address;
  uint32_t ClockInHz;
  IMX335_Object_t ctx_driver;
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
} CMW_IMX335_t;

typedef struct
{
  CMW_PixelFormat_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
} CMW_IMX335_config_t;

int32_t CMW_CAMERA_IMX335_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx,  DCMIPP_HandleTypeDef *hdcmipp, CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP);
void CMW_IMX335_SetDefaultSensorValues(void *sensor_config);

#ifdef __cplusplus
}
#endif

#endif