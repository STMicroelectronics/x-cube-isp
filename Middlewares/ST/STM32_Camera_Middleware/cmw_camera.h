 /**
 ******************************************************************************
 * @file    cmw_camera.h
 * @author  GPM Application Team
 *
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CMW_CAMERA_H
#define CMW_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmw_errno.h"
#include "cmw_camera_conf.h"
#include "cmw_sensors_if.h"

/* Camera capture mode */
#define CMW_MODE_CONTINUOUS          DCMIPP_MODE_CONTINUOUS
#define CMW_MODE_SNAPSHOT            DCMIPP_MODE_SNAPSHOT

#define CMW_PIXEL_FORMAT_DEFAULT     0x00                 /*!< Default Data Type chosen by cmw */
#define CMW_PIXEL_FORMAT_YUV420_8    DCMIPP_DT_YUV420_8   /*!< Data Type YUV420 8bit  */
#define CMW_PIXEL_FORMAT_YUV420_10   DCMIPP_DT_YUV420_10  /*!< Data Type YUV420 10bit  */
#define CMW_PIXEL_FORMAT_YUV422_8    DCMIPP_DT_YUV422_8   /*!< Data Type YUV422 8bit  */
#define CMW_PIXEL_FORMAT_YUV422_10   DCMIPP_DT_YUV422_10  /*!< Data Type YUV422 10bit */
#define CMW_PIXEL_FORMAT_RGB444      DCMIPP_DT_RGB444     /*!< Data Type RGB444       */
#define CMW_PIXEL_FORMAT_RGB555      DCMIPP_DT_RGB555     /*!< Data Type RGB555       */
#define CMW_PIXEL_FORMAT_RGB565      DCMIPP_DT_RGB565     /*!< Data Type RGB565       */
#define CMW_PIXEL_FORMAT_RGB666      DCMIPP_DT_RGB666     /*!< Data Type RGB666       */
#define CMW_PIXEL_FORMAT_RGB888      DCMIPP_DT_RGB888     /*!< Data Type RGB888       */
#define CMW_PIXEL_FORMAT_RAW8        DCMIPP_DT_RAW8       /*!< Data Type RawBayer8    */
#define CMW_PIXEL_FORMAT_RAW10       DCMIPP_DT_RAW10      /*!< Data Type RawBayer10   */
#define CMW_PIXEL_FORMAT_RAW12       DCMIPP_DT_RAW12      /*!< Data Type RawBayer12   */
#define CMW_PIXEL_FORMAT_RAW14       DCMIPP_DT_RAW14      /*!< Data Type RawBayer14   */

/* Mirror/Flip */
#define CMW_MIRRORFLIP_NONE          0x00U   /* Set camera normal mode          */
#define CMW_MIRRORFLIP_FLIP          0x01U   /* Set camera flip config          */
#define CMW_MIRRORFLIP_MIRROR        0x02U   /* Set camera mirror config        */
#define CMW_MIRRORFLIP_FLIP_MIRROR   0x03U   /* Set camera flip + mirror config */


typedef enum {
  CMW_UNKNOWN_Sensor = 0x0,
  CMW_VD66GY_Sensor,
  CMW_VD56G3_Sensor,
  CMW_IMX335_Sensor,
  CMW_OV5640_Sensor,
  CMW_VD55G1_Sensor,
  CMW_VD65G4_Sensor,
  CMW_VD1943_Sensor,
  CMW_VD5943_Sensor
} CMW_Sensor_Name_t;

typedef struct
{
  uint32_t pixel_format;  /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  int line_len;
} CMW_VD66GY_config_t;

typedef struct
{
  uint32_t pixel_format;  /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  int line_len;
} CMW_VD56G3_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD55G1_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD65G4_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD1943_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD5943_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
} CMW_IMX335_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
} CMW_OV5640_config_t;


typedef struct
{
  CMW_Sensor_Name_t selected_sensor;
  union {
    CMW_IMX335_config_t imx335_config;
    CMW_VD66GY_config_t vd66gy_config;
    CMW_VD56G3_config_t vd56g3_config;
    CMW_VD55G1_config_t vd55g1_config;
    CMW_VD65G4_config_t vd65g4_config;
    CMW_VD1943_config_t vd1943_config;
    CMW_VD5943_config_t vd5943_config;
    CMW_OV5640_config_t ov5640_config;
  } config_sensor;
} CMW_Advanced_Config_t;


typedef enum {
  CMW_Aspect_ratio_crop = 0x0,
  CMW_Aspect_ratio_fit,
  CMW_Aspect_ratio_fullscreen,
  CMW_Aspect_ratio_manual_roi,
} CMW_Aspect_Ratio_Mode_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t offset_x;
  uint32_t offset_y;
} CMW_Manual_roi_area_t;

typedef struct {
  /* Camera settings */
  uint32_t width;
  uint32_t height;
  int fps;
  int mirror_flip;
} CMW_CameraInit_t;

typedef struct {
  /* pipe output settings */
  uint32_t output_width;
  uint32_t output_height;
  int output_format;
  int output_bpp;
  int enable_swap;
  int enable_gamma_conversion;
  /*Output buffer of the pipe*/
  int mode;
  /* You must fill manual_conf when mode is CMW_Aspect_ratio_manual_roi */
  CMW_Manual_roi_area_t manual_conf;
} CMW_DCMIPP_Conf_t;


/* Camera exposure mode
* Some cameras embed their own Auto Exposure algorithm.
* The following defines allow the user to chose the exposure mode of the camera.
* Camera exposure mode has no impact if the camera does not support it.
*/
#define CMW_EXPOSUREMODE_AUTO          0x00U   /* Start the camera auto exposure functionnality */
#define CMW_EXPOSUREMODE_AUTOFREEZE    0x01U   /* Stop the camera auto exposure functionnality and freeze the current value */
#define CMW_EXPOSUREMODE_MANUAL        0x02U   /* Set the camera in manual exposure (exposure is control by a software algorithm) */

DCMIPP_HandleTypeDef* CMW_CAMERA_GetDCMIPPHandle();

/**
  * @brief  Initializes the camera.
  * @param  initConf  Mandatory: General camera config
  * @param  advanced_config  Optional: Sensor specific configuration; NULL if you want to let CMW configure for you
  * @retval CMW status
  */
int32_t CMW_CAMERA_Init(CMW_CameraInit_t *init_conf, CMW_Advanced_Config_t *advanced_config);

/**
 * @brief  Fill the sensor configuration structure with default values.
 * @param  advanced_config  Pointer to the sensor configuration structure
 * @retval CMW status
 */
int32_t CMW_CAMERA_SetDefaultSensorValues(CMW_Advanced_Config_t *advanced_config);
int32_t CMW_CAMERA_DeInit();
int32_t CMW_CAMERA_Run();
int32_t CMW_CAMERA_SetPipeConfig(uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch);
int32_t CMW_CAMERA_GetSensorName(CMW_Sensor_Name_t *sensorName);
int32_t CMW_CAMERA_SetWBRefMode(uint8_t Automatic, uint32_t RefColorTemp);
int32_t CMW_CAMERA_ListWBRefModes(uint32_t RefColorTemp[]);

int32_t CMW_CAMERA_Start(uint32_t pipe, uint8_t *pbuff, uint32_t Mode);
int32_t CMW_CAMERA_DoubleBufferStart(uint32_t pipe, uint8_t *pbuff1, uint8_t *pbuff2, uint32_t Mode);
int32_t CMW_CAMERA_Suspend(uint32_t pipe);
int32_t CMW_CAMERA_Resume(uint32_t pipe);
int32_t CMW_CAMERA_EnableRestartState(ISP_RestartStateTypeDef *ISP_RestartState);
int32_t CMW_CAMERA_DisableRestartState();

int CMW_CAMERA_SetAntiFlickerMode(int flicker_mode);
int CMW_CAMERA_GetAntiFlickerMode(int *flicker_mode);

int CMW_CAMERA_SetBrightness(int Brightness);
int CMW_CAMERA_GetBrightness(int *Brightness);

int CMW_CAMERA_SetContrast(int Contrast);
int CMW_CAMERA_GetContrast(int *Contrast);

int CMW_CAMERA_SetGain(int32_t Gain);
int CMW_CAMERA_GetGain(int32_t *Gain);

int CMW_CAMERA_SetExposure(int32_t exposure);
int CMW_CAMERA_GetExposure(int32_t *exposure);

int32_t CMW_CAMERA_SetMirrorFlip(int32_t MirrorFlip);
int32_t CMW_CAMERA_GetMirrorFlip(int32_t *MirrorFlip);

int32_t CMW_CAMERA_SetExposureMode(int32_t exposureMode);
int32_t CMW_CAMERA_GetExposureMode(int32_t *exposureMode);

int32_t CMW_CAMERA_SetTestPattern(int32_t mode);
int32_t CMW_CAMERA_GetTestPattern(int32_t *mode);

int32_t CMW_CAMERA_GetSensorInfo(ISP_SensorInfoTypeDef *info);

HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp);

int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe);
int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t pipe);
void CMW_CAMERA_PIPE_ErrorCallback(uint32_t pipe);

#ifdef __cplusplus
}
#endif

#endif /* __MW_CAMERA_H */
