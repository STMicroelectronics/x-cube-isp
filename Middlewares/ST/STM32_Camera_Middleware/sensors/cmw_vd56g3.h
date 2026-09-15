/**
  ******************************************************************************
  * @file    cmw_vd56g3.h
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

#ifndef CMW_VD56G3_H
#define CMW_VD56G3_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include "cmw_camera.h"
#include "cmw_sensors_if.h"
#include "cmw_errno.h"
#include "vd6g.h"

#define VD56G3_NAME              "VD56G3"
#define CAMERA_VD56G3_ADDRESS    0x20U
#define CAMERA_VD56G3_FREQ_IN_HZ 12000000U

typedef struct
{
  uint16_t Address;
  VD6G_Ctx_t ctx_driver;
  uint8_t IsInitialized;
  int32_t (*Init)(void);
  int32_t (*DeInit)(void);
  int32_t (*WriteReg)(uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*ReadReg)(uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*GetTick)(void);
  void (*Delay)(uint32_t delay_in_ms);
  void (*ShutdownPin)(int value);
  void (*EnablePin)(int value);
} CMW_VD56G3_t;

typedef struct
{
  CMW_PixelFormat_t pixel_format;  /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  int line_len;
} CMW_VD56G3_config_t;

int32_t CMW_CAMERA_VD56G3_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx,  DCMIPP_HandleTypeDef *hdcmipp,
                               CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP);
void CMW_VD56G3_SetDefaultSensorValues(void *sensor_config);

#ifdef __cplusplus
}
#endif

#endif
