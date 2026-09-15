/**
  ******************************************************************************
  * @file    cmw_sensors_if.h
  * @author  GPM Application Team
  * @brief   This header file contains the common defines and functions prototypes
  *          for the camera driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef CMW_SENSORS_IF
#define CMW_SENSORS_IF

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Bayer / Pattern */
#define CMW_BAYER_PATTERN_RGGB               0x00U
#define CMW_BAYER_PATTERN_GRBG               0x01U
#define CMW_BAYER_PATTERN_GBRG               0x02U
#define CMW_BAYER_PATTERN_BGGR               0x03U
#define CMW_BAYER_PATTERN_MONO               0x04U
#define CMW_BAYER_PATTERN_RGBNIR             0x05U
#define CMW_BAYER_PATTERN_RGBNIR_MIRROR      0x06U
#define CMW_BAYER_PATTERN_RGBNIR_FLIP        0x07U
#define CMW_BAYER_PATTERN_RGBNIR_FLIP_MIRROR 0x08U

#define CMW_SENSOR_INFO_MAX_LENGTH      (32U)

typedef struct
{
  char name[CMW_SENSOR_INFO_MAX_LENGTH];
  uint8_t bayer_pattern;
  uint8_t color_depth;
  uint32_t width;
  uint32_t height;
  uint32_t gain_min;
  uint32_t gain_max;
  uint32_t again_max;
  uint32_t exposure_min;
  uint32_t exposure_max;
} CMW_Sensor_Info_t;

typedef struct
{
  uint32_t width;
  uint32_t height;
  int fps;
  uint32_t mirrorFlip;
  void *sensor_config; /* to pass specific config from application side*/
  int isp_decimation_ratio_h;
  int isp_decimation_ratio_v;
} CMW_Sensor_Init_t;

typedef struct
{
  int32_t (*DeInit)(void *);
  int32_t (*Start)(void *);
  int32_t (*Run)(void *);
  void    (*VsyncEventCallback)(void *, uint32_t);
  void    (*FrameEventCallback)(void *, uint32_t);
    int32_t (*Stop)(void *);
  int32_t (*SetMirrorFlip)(void *, uint32_t);
  int32_t (*SetFramerate)(void*, int32_t);
  int32_t (*SetGain)(void *, int32_t);
  int32_t (*SetExposure)(void *, int32_t);
  int32_t (*SetExposureMode)(void *, int32_t);
  int32_t (*SetWBRefMode)(void *, uint8_t, uint32_t);
  int32_t (*ListWBRefModes)(void *, uint32_t[], uint32_t);
  int32_t (*GetSensorInfo)(void *, CMW_Sensor_Info_t *);
  int32_t (*SetTestPattern)(void *, int32_t);
  int32_t (*GetIspDecimationRatio)(void *, int32_t *, int32_t*);
} CMW_Sensor_if_t;


// Each sensor driver should provide an instance of this struct
typedef struct {
  const char *name;
  int32_t (*init)(CMW_Sensor_if_t *camera_drv, void *sensor_ctx,  DCMIPP_HandleTypeDef *hdcmipp, CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP);
  void  (*set_defaultSensorValues)(void *sensor_config);
  void *sensor_ctx; // Sensor-specific context
} camera_sensor_t;



#ifdef __cplusplus
}
#endif

#endif /* CMW_SENSORS_IF */
