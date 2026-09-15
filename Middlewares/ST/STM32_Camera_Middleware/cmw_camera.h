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
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_core.h"
#endif

/* Number of White Balance reference modes returned by CMW_CAMERA_ListWBRefModes().
 * Callers must provide an array of at least this. */
#if !defined (CMW_USE_WITHOUT_ISP)
#define CMW_CAMERA_NB_WB_REF_MODES   ISP_AWB_COLORTEMP_REF
#else
#define CMW_CAMERA_NB_WB_REF_MODES   (5U)
#endif

/* Camera capture mode */
typedef enum {
  CMW_CAPTUREMODE_CONTINUOUS = DCMIPP_MODE_CONTINUOUS,   /*!< Continuous capture mode */
  CMW_CAPTUREMODE_SNAPSHOT   = DCMIPP_MODE_SNAPSHOT,     /*!< Snapshot capture mode   */
} CMW_CaptureMode_t;

typedef enum {
  CMW_PIXEL_FORMAT_DEFAULT     = 0x00,                 /*!< Default Data Type chosen by cmw */
  CMW_PIXEL_FORMAT_YUV420_8    = DCMIPP_DT_YUV420_8,   /*!< Data Type YUV420 8bit  */
  CMW_PIXEL_FORMAT_YUV420_10   = DCMIPP_DT_YUV420_10,  /*!< Data Type YUV420 10bit  */
  CMW_PIXEL_FORMAT_YUV422_8    = DCMIPP_DT_YUV422_8,   /*!< Data Type YUV422 8bit  */
  CMW_PIXEL_FORMAT_YUV422_10   = DCMIPP_DT_YUV422_10,  /*!< Data Type YUV422 10bit */
  CMW_PIXEL_FORMAT_RGB444      = DCMIPP_DT_RGB444,     /*!< Data Type RGB444       */
  CMW_PIXEL_FORMAT_RGB555      = DCMIPP_DT_RGB555,     /*!< Data Type RGB555       */
  CMW_PIXEL_FORMAT_RGB565      = DCMIPP_DT_RGB565,     /*!< Data Type RGB565       */
  CMW_PIXEL_FORMAT_RGB666      = DCMIPP_DT_RGB666,     /*!< Data Type RGB666       */
  CMW_PIXEL_FORMAT_RGB888      = DCMIPP_DT_RGB888,     /*!< Data Type RGB888       */
  CMW_PIXEL_FORMAT_RAW8        = DCMIPP_DT_RAW8,       /*!< Data Type RawBayer8    */
  CMW_PIXEL_FORMAT_RAW10       = DCMIPP_DT_RAW10,      /*!< Data Type RawBayer10   */
  CMW_PIXEL_FORMAT_RAW12       = DCMIPP_DT_RAW12,      /*!< Data Type RawBayer12   */
  CMW_PIXEL_FORMAT_RAW14       = DCMIPP_DT_RAW14,      /*!< Data Type RawBayer14   */
} CMW_PixelFormat_t;

/* Mirror/Flip */
typedef enum {
  CMW_MIRRORFLIP_NONE = 0x00U,          /*!< Set camera normal mode          */
  CMW_MIRRORFLIP_FLIP = 0x01U,          /*!< Set camera flip config          */
  CMW_MIRRORFLIP_MIRROR = 0x02U,        /*!< Set camera mirror config        */
  CMW_MIRRORFLIP_FLIP_MIRROR = 0x03U,   /*!< Set camera flip + mirror config */
} CMW_MirrorFlip_t;

typedef struct
{
  const char *sensor_name;   /*!< Sensor name string (registry name). NULL to auto-probe */
  void *sensor_config;       /*!< Sensor-specific configuration structure */
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
  CMW_MirrorFlip_t mirror_flip;
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
#if defined (CMW_USE_WITHOUT_ISP)
  int isp_decimation_ratio_h;
  int isp_decimation_ratio_v;
#endif
} CMW_DCMIPP_Conf_t;


/* Camera exposure mode
* Some cameras embed their own Auto Exposure algorithm.
* The following defines allow the user to chose the exposure mode of the camera.
* Camera exposure mode has no impact if the camera does not support it.
*/
typedef enum {
  CMW_EXPOSUREMODE_AUTO = 0x00U,        /*!< Start the camera auto exposure functionnality */
  CMW_EXPOSUREMODE_AUTOFREEZE = 0x01U,  /*!< Stop the camera auto exposure functionnality and freeze the current value */
  CMW_EXPOSUREMODE_MANUAL = 0x02U,      /*!< Set the camera in manual exposure (exposure is control by a software algorithm) */
} CMW_ExposureMode_t;

/**
  * @brief  Get the DCMIPP handle used by the camera middleware.
  * @retval Pointer to the DCMIPP handle
  */
DCMIPP_HandleTypeDef* CMW_CAMERA_GetDCMIPPHandle();

/**
  * @brief  Initializes the camera.
  * @param  init_conf  Mandatory: General camera config
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

/**
  * @brief  Run the camera middleware periodic processing (e.g. ISP run algorithms).
  * @retval CMW status
  */
int32_t CMW_CAMERA_Run();

/**
  * @brief  Configure a DCMIPP pipe output (crop, decimation, downsize, format).
  * @param  pipe  DCMIPP Pipe
  * @param  p_conf  Pointer to the pipe output configuration
  * @param  pitch  Pointer that receives the resulting pipe pitch (for buffer alignment)
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetPipeConfig(uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch);

/**
  * @brief  Get Sensor name.
  * @param  sensor_name  Camera sensor name
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetSensorName(const char **sensor_name);

/**
  * @brief  Set White Balance mode.
  * @param  automatic  If not null, set automatic white balance mode
  * @param  ref_color_temp  If automatic is null, set white balance mode
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetWBRefMode(uint8_t automatic, uint32_t ref_color_temp);

/**
  * @brief  Get White Balance reference modes list.
  * @param  ref_color_temp  White Balance reference modes. Must point to an array
  *                         of at least @ref CMW_CAMERA_NB_WB_REF_MODES entries.
  * @param  array_size      Number of entries available in ref_color_temp
  * @retval CMW status
  */
int32_t CMW_CAMERA_ListWBRefModes(uint32_t ref_color_temp[], uint32_t array_size);

/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff pointer to the camera output buffer
  * @param  mode  CMW_CAPTUREMODE_CONTINUOUS or CMW_CAPTUREMODE_SNAPSHOT
  * @retval CMW status
  */
int32_t CMW_CAMERA_Start(uint32_t pipe, uint8_t *pbuff, CMW_CaptureMode_t mode);

/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff1 pointer to the first camera output buffer
  * @param  pbuff2 pointer to the second camera output buffer
  * @param  mode  CMW_CAPTUREMODE_CONTINUOUS or CMW_CAPTUREMODE_SNAPSHOT
  * @retval CMW status
  */
int32_t CMW_CAMERA_DoubleBufferStart(uint32_t pipe, uint8_t *pbuff1, uint8_t *pbuff2, CMW_CaptureMode_t mode);

/**
  * @brief  Stops the camera stream and all the pipes.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Stop(void);

/**
  * @brief  Suspend the CAMERA capture on selected pipe
  *         Do not stop the camera, just suspend the selected pipe.
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Suspend(uint32_t pipe);

/**
  * @brief  DeInitializes the camera.
  *         The user must call CMW_CAMERA_Stop if the camera is started before calling this function.
  * @retval CMW status
  */
int32_t CMW_CAMERA_DeInit();

/**
  * @brief  Resume the CAMERA capture on selected pipe
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Resume(uint32_t pipe);
#if !defined (CMW_USE_WITHOUT_ISP)
/**
  * @brief  Enable the Restart State. When enabled, at system restart, the ISP middleware configuration
  *         is restored from the last update before the restart.
  * @param  ISP_restart_state pointer to ISP Restart State. To use this mode in a Low Power use case, where
  *         the ISP state is applied at system wake up, this pointer must be in some retention memory.
  * @retval CMW status
  */
int32_t CMW_CAMERA_EnableRestartState(ISP_RestartStateTypeDef *ISP_restart_state);

/**
  * @brief  Disable the Restart State
  * @retval CMW status
  */
int32_t CMW_CAMERA_DisableRestartState();
#endif /* !CMW_USE_WITHOUT_ISP */

/**
  * @brief  Set the camera gain.
  * @param  gain     Gain in mdB
  * @retval CMW status
  */
int CMW_CAMERA_SetGain(int32_t gain);

/**
  * @brief  Get the camera gain.
  * @param  gain     Gain in mdB
  * @retval CMW status
  */
int CMW_CAMERA_GetGain(int32_t *gain);

/**
  * @brief  Set the camera exposure.
  * @param  exposure exposure in microseconds
  * @retval CMW status
  */
int CMW_CAMERA_SetExposure(int32_t exposure);

/**
  * @brief  Get the camera exposure.
  * @param  exposure exposure in microseconds
  * @retval CMW status
  */
int CMW_CAMERA_GetExposure(int32_t *exposure);

/**
  * @brief  Set the camera Mirror/Flip.
  * @param  mirror_flip CMW_MIRRORFLIP_NONE CMW_MIRRORFLIP_FLIP CMW_MIRRORFLIP_MIRROR CMW_MIRRORFLIP_FLIP_MIRROR
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetMirrorFlip(CMW_MirrorFlip_t mirror_flip);

/**
  * @brief  Get the camera Mirror/Flip.
  * @param  mirror_flip CMW_MIRRORFLIP_NONE CMW_MIRRORFLIP_FLIP CMW_MIRRORFLIP_MIRROR CMW_MIRRORFLIP_FLIP_MIRROR
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetMirrorFlip(CMW_MirrorFlip_t *mirror_flip);

/**
  * @brief  Set the camera exposure mode.
  * @param  exposure_mode Exposure mode CMW_EXPOSUREMODE_AUTO, CMW_EXPOSUREMODE_AUTOFREEZE, CMW_EXPOSUREMODE_MANUAL
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetExposureMode(CMW_ExposureMode_t exposure_mode);

/**
  * @brief  Get the camera exposure mode.
  * @param  exposure_mode Exposure mode CMW_EXPOSUREMODE_AUTO, CMW_EXPOSUREMODE_AUTOFREEZE, CMW_EXPOSUREMODE_MANUAL
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetExposureMode(CMW_ExposureMode_t *exposure_mode);

/**
  * @brief  Set (Enable/Disable and Configure) the camera test pattern
  * @param  mode Pattern mode (sensor specific value) to be configured. '-1' means disable.
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetTestPattern(int32_t mode);

/**
  * @brief  Get the camera test pattern
  * @param  mode Pattern mode (sensor specific value) to be returned. '-1' means disable.
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetTestPattern(int32_t *mode);

/**
  * @brief  Get the Camera Sensor info.
  * @param  info  pointer to sensor info
  * @note   This function should be called after the init. This to get Capabilities
  *         from the camera sensor
  * @retval Component status
  */
int32_t CMW_CAMERA_GetSensorInfo(CMW_Sensor_Info_t *info);

/**
  * @brief  DCMIPP Clock Config for DCMIPP.
  * @param  hdcmipp  DCMIPP Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL status
  */
HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp);

/**
  * @brief  Frame Event callback on pipe
  * @param  pipe  Pipe receiving the callback
  * @retval CMW status
  */
int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe);

/**
  * @brief  Vsync Event callback on pipe
  * @param  pipe  Pipe receiving the callback
  * @retval CMW status
  */
int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t pipe);

/**
  * @brief  Error callback on pipe
  * @param  pipe  Pipe receiving the callback
  * @retval None
  */
void CMW_CAMERA_PIPE_ErrorCallback(uint32_t pipe);

#ifdef __cplusplus
}
#endif

#endif /* CMW_CAMERA_H */
