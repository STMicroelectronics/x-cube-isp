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
#include <stdbool.h>
#include "cmw_camera.h"
#include "cmw_sensors_if.h"
#include "cmw_errno.h"
#include "vd1943.h"
#include "stm32n6xx_hal_dcmipp.h"
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_api.h"
#endif

#define VD1943_CUT1_3_CHIP_ID   0x53393430
#define VD1943_CUT1_4_CHIP_ID   0x53393431
#define VD1943_NAME             "VD1943"
#define CAMERA_VD1943_ADDRESS   0x20U

#ifndef BIT
#define BIT(n) (1u << (n))
#endif

 typedef enum {
    CMW_VD1943_GLOBAL_SHUTTER = 0,
    CMW_VD1943_ROLLING_SHUTTER = 1,
} cmw_vd1943_shutter_type_t;

typedef enum {
    CMW_VD1943_DYNAMIC_RANGE_SDR = 0,
    CMW_VD1943_DYNAMIC_RANGE_HDR = 1,
} cmw_vd1943_dynamic_range_t;

/**
 * @brief Pixel colour-filter-array (CFA) pattern.
 *
 * The upper 16 bits encode the pattern size (1×1, 2×2, 4×4) while the lower
 * bits identify the specific arrangement.  This encoding allows quick
 * pattern-size checks via a bitmask.
 */
typedef enum {
    CMW_VD1943_SENSOR_PIXEL_PATTERN_UNKNOWN = 0,                                      /**< Unknown pattern */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_11 = 0x10000,                                     /**< 1×1 base pattern */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_22 = 0x20000,                                     /**< 2×2 base pattern */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_44 = 0x40000,                                     /**< 4×4 base pattern */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_MONO = CMW_VD1943_SENSOR_PIXEL_PATTERN_11 | (1 << 1),    /**< Monochrome */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_IR = CMW_VD1943_SENSOR_PIXEL_PATTERN_11 | (1 << 2),      /**< Monochrome IR */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_RGGB = CMW_VD1943_SENSOR_PIXEL_PATTERN_22 | (1 << 1),    /**< Bayer RGGB */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_GRBG = CMW_VD1943_SENSOR_PIXEL_PATTERN_22 | (1 << 2),    /**< Bayer GRBG */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG = CMW_VD1943_SENSOR_PIXEL_PATTERN_22 | (1 << 3),    /**< Bayer GBRG */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_BGGR = CMW_VD1943_SENSOR_PIXEL_PATTERN_22 | (1 << 4),    /**< Bayer BGGR */
    CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR = CMW_VD1943_SENSOR_PIXEL_PATTERN_44 | (1 << 1),  /**< 4×4 RGB + NIR */
} cmw_vd1943_sensor_pixel_pattern_t;


/**
 * @brief Pixel depth (bits per pixel) as bit-flags.
 *
 * Values are encoded as single-bit flags so they can be OR-ed together to
 * describe the set of depths supported by a sensor (see @ref cmw_vd1943_sensor_capabilities_t).
 */
typedef enum {
    CMW_VD1943_SENSOR_PIXEL_DEPTH_NONE = 0,           /**< No depth / unset */
    CMW_VD1943_SENSOR_PIXEL_DEPTH_8 = BIT(0),         /**< 8 bits per pixel */
    CMW_VD1943_SENSOR_PIXEL_DEPTH_10 = BIT(1),        /**< 10 bits per pixel */
    CMW_VD1943_SENSOR_PIXEL_DEPTH_12 = BIT(2),        /**< 12 bits per pixel */
    CMW_VD1943_SENSOR_PIXEL_DEPTH_14 = BIT(3),        /**< 14 bits per pixel */
    CMW_VD1943_SENSOR_PIXEL_DEPTH_16 = BIT(4),        /**< 16 bits per pixel */
    CMW_VD1943_SENSOR_PIXEL_DEPTH_UNKNOWN = BIT(5)    /**< Unknown / unsupported depth */
} cmw_vd1943_sensor_pixel_depth_t;


/**
 * @brief Pixel sub-sampling modes (bit-flags).
 *
 * Multiple values can be OR-ed together to express the set of sub-sampling
 * modes supported by a sensor.
 */
typedef enum {
    CMW_VD1943_SENSOR_SUBSAMPLING_OFF = BIT(0),     /**< No sub-sampling (1×1) */
    CMW_VD1943_SENSOR_SUBSAMPLING_2x2 = BIT(1),     /**< 2×2 sub-sampling */
    CMW_VD1943_SENSOR_SUBSAMPLING_4x4 = BIT(2),     /**< 4×4 sub-sampling */
    CMW_VD1943_SENSOR_SUBSAMPLING_8x8 = BIT(3),     /**< 8×8 sub-sampling */
    CMW_VD1943_SENSOR_SUBSAMPLING_32x32 = BIT(4),   /**< 32×32 sub-sampling */
} cmw_vd1943_sensor_subsampling_t;



typedef struct {
    cmw_vd1943_shutter_type_t shutter_type;
    cmw_vd1943_sensor_pixel_depth_t pixel_depth;
    cmw_vd1943_sensor_pixel_pattern_t pixel_pattern;
    cmw_vd1943_sensor_subsampling_t subsampling;
    cmw_vd1943_dynamic_range_t dynamic_range;
    bool upscaling;
    bool split_exposure;
} cmw_vd1943_stream_mode_config_t;

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
} CMW_VD1943_t;

typedef struct
{
  uint32_t CSI_PHYBitrate;
  cmw_vd1943_stream_mode_config_t sensor_stream_mode; /* to select different image processing settings in the driver, if supported by the sensor */
} CMW_VD1943_config_t;

int32_t CMW_CAMERA_VD1943_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx,  DCMIPP_HandleTypeDef *hdcmipp,
                               CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP);
void CMW_VD1943_SetDefaultSensorValues(void *sensor_config);

#ifdef __cplusplus
}
#endif

#endif
