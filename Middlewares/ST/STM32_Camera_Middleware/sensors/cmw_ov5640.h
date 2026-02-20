/**
  ******************************************************************************
  * @file    cmw_ov5640.h
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

#ifndef CMW_OV5640
#define CMW_OV5640

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include "cmw_sensors_if.h"
#include "cmw_errno.h"
#include "ov5640.h"
#include "cmw_camera.h"

typedef struct
{
  uint16_t Address;
  uint32_t ClockInHz;
  OV5640_Object_t ctx_driver;
  uint8_t IsInitialized;
  int32_t (*Init)(void);
  int32_t (*DeInit)(void);
  int32_t (*WriteReg)(uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*ReadReg) (uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*GetTick) (void);
  void (*Delay)(uint32_t delay_in_ms);
  void (*ShutdownPin)(int value);
  void (*EnablePin)(int value);
} CMW_OV5640_t;

int CMW_OV5640_Probe(CMW_OV5640_t *io_ctx, CMW_Sensor_if_t *vd55g1_if);
void CMW_OV5640_SetDefaultSensorValues(CMW_OV5640_config_t *ov5640_config);

#ifdef __cplusplus
}
#endif

#endif