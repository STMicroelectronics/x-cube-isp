/**
  ******************************************************************************
  * @file    cmw_<sensor>.h
  * @brief   Template sensor adaptation layer for stm32-mw-camera.
  *
  * Replace:
  * - SENSORNAME  -> your sensor name in uppercase (example: IMX335)
  * - SENSORNAME  -> your sensor name in lowercase (example: imx335)
  * - SENSORNAME  -> your sensor name in CamelCase if needed by your vendor driver
  ******************************************************************************
  */

#ifndef CMW_SENSORNAME
#define CMW_SENSORNAME

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cmw_camera.h"
#include "cmw_sensors_if.h"
#include "cmw_errno.h"

typedef struct
{
  uint16_t Address;
  uint32_t ClockInHz;
#if !defined (CMW_USE_WITHOUT_ISP)
  ISP_HandleTypeDef hIsp;
#endif
  DCMIPP_HandleTypeDef *hdcmipp;
  uint8_t IsInitialized;
  int32_t (*Init)(void);
  int32_t (*DeInit)(void);
  int32_t (*WriteReg)(uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*ReadReg)(uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*GetTick)(void);
  void (*Delay)(uint32_t delay_in_ms);
  void (*ShutdownPin)(int value);
  void (*EnablePin)(int value);

  /* Vendor driver object goes here, for example:
   * SENSORNAME_Object_t ctx_driver;
   */
} CMW_SENSORNAME_t;

typedef struct
{
  CMW_PixelFormat_t pixel_format;
} CMW_SENSORNAME_config_t;

int32_t CMW_CAMERA_SENSORNAME_Init(CMW_Sensor_if_t *camera_drv,
                                  void *sensor_ctx,
                                  DCMIPP_HandleTypeDef *hdcmipp,
                                  CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP);

void CMW_SENSORNAME_SetDefaultSensorValues(void *sensor_config);

#ifdef __cplusplus
}
#endif

#endif
