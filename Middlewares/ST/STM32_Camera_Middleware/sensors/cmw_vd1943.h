/**
  ******************************************************************************
  * @file    cmw_vd1943.h
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

#ifndef CMW_VD1943_H
#define CMW_VD1943_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include "cmw_sensors_if.h"
#include "cmw_errno.h"
#include "vd1943.h"
#include "stm32n6xx_hal_dcmipp.h"
#include "cmw_camera.h"
#include "isp_api.h"

#define VD1943_CUT1_3_CHIP_ID   0x53393430
#define VD1943_CUT1_4_CHIP_ID   0x53393431
#define VD1943_NAME             "VD1943"

typedef struct
{
  uint16_t Address;
  VD1943_Ctx_t  ctx_driver;
  ISP_HandleTypeDef hIsp;
  ISP_AppliHelpersTypeDef appliHelpers;
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
} CMW_VD1943_t;

int CMW_VD1943_Probe(CMW_VD1943_t *io_ctx, CMW_Sensor_if_t *vd1943_if);
void CMW_VD1943_SetDefaultSensorValues(CMW_VD1943_config_t *vd1943_config);

#ifdef __cplusplus
}
#endif

#endif
