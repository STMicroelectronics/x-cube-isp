/**
  ******************************************************************************
  * @file    cmw_vd65g4.h
  * @author  MDG Application Team
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

#ifndef CMW_VD65G4
#define CMW_VD65G4

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include "cmw_sensors_if.h"
#include "cmw_errno.h"
#include "vd55g1.h"
#include "cmw_camera.h"

#define VD65G4_NAME    "VD65G4"

typedef struct
{
  uint16_t Address;
  VD55G1_Ctx_t ctx_driver;
  uint8_t IsInitialized;
  int32_t (*Init)(void);
  int32_t (*DeInit)(void);
  int32_t (*WriteReg)(uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*ReadReg) (uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*GetTick) (void);
  void (*Delay)(uint32_t delay_in_ms);
  void (*ShutdownPin)(int value);
  void (*EnablePin)(int value);
} CMW_VD65G4_t;

int CMW_VD65G4_Probe(CMW_VD65G4_t *io_ctx, CMW_Sensor_if_t *vd65g4_if);
void CMW_VD65G4_SetDefaultSensorValues(CMW_VD65G4_config_t *vd65g4_config);

#ifdef __cplusplus
}
#endif

#endif
