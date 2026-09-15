 /**
 ******************************************************************************
 * @file    cmw_camera.c
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


/* Includes ------------------------------------------------------------------*/
#include "cmw_camera.h"

#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_api.h"
#endif
#include "stm32n6xx_hal_dcmipp.h"
#include "cmw_utils.h"
#include "cmw_io.h"
#include "assert.h"
#include <string.h>
#include "cmw_sensor_registry.h"

/* A valid decimation ratio is one of: 1, 2, 4, 8 */
#define CMW_CAMERA_IS_VALID_DECIMATION_RATIO(ratio) \
  (((ratio) >= 1) && ((ratio) <= 8) && (((ratio) & ((ratio) - 1)) == 0))

typedef struct
{
  uint32_t Resolution;
  uint32_t pixel_format;
  uint32_t LightMode;
  uint32_t ColorEffect;
  int32_t  Brightness;
  int32_t  Saturation;
  int32_t  Contrast;
  int32_t  HueDegree;
  int32_t  Gain;
  int32_t  Exposure;
  int32_t  ExposureMode;
  uint32_t MirrorFlip;
  uint32_t Zoom;
  uint32_t NightMode;
  uint32_t IsMspCallbacksValid;
  uint32_t TestPattern;
  int32_t isp_decimation_ratio_h;
  int32_t isp_decimation_ratio_v;
} CAMERA_Ctx_t;

CMW_CameraInit_t  camera_conf;
CAMERA_Ctx_t  Camera_Ctx;

DCMIPP_HandleTypeDef hcamera_dcmipp;

static camera_sensor_t *active_sensor = NULL;
static CMW_Sensor_if_t Camera_Drv;
#if !defined (CMW_USE_WITHOUT_ISP)
static ISP_AppliHelpersTypeDef appliHelpers_ISP;
#endif

int is_camera_init = 0;
int is_camera_started = 0;
int is_pipe1_2_shared = 0;

static void CMW_CAMERA_EnableGPIOs(void);
static void CMW_CAMERA_PwrDown(void);
static int32_t CMW_CAMERA_SetPipe(DCMIPP_HandleTypeDef *hdcmipp, uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch);
static int CMW_CAMERA_Probe_Sensor(CMW_Sensor_Init_t *initValues, const char *sensor_name);
static camera_sensor_t *CMW_CAMERA_FindSensorByName(const char *sensor_name);
#if !defined (CMW_USE_WITHOUT_ISP)
static ISP_StatusTypeDef CB_ISP_SetSensorGain(uint32_t camera_instance, int32_t gain);
static ISP_StatusTypeDef CB_ISP_GetSensorGain(uint32_t camera_instance, int32_t *gain);
static ISP_StatusTypeDef CB_ISP_SetSensorExposure(uint32_t camera_instance, int32_t exposure);
static ISP_StatusTypeDef CB_ISP_GetSensorExposure(uint32_t camera_instance, int32_t *exposure);
static ISP_StatusTypeDef CB_ISP_GetSensorInfo(uint32_t camera_instance, ISP_SensorInfoTypeDef *Info);
#endif

DCMIPP_HandleTypeDef* CMW_CAMERA_GetDCMIPPHandle(void)
{
    return &hcamera_dcmipp;
}

int32_t CMW_CAMERA_SetPipeConfig(uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch)
{
  return CMW_CAMERA_SetPipe(&hcamera_dcmipp, pipe, p_conf, pitch);
}

/**
  * @brief  Get Sensor name.
  * @param  sensorName  Camera sensor name
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetSensorName(const char **sensor_name)
{
  int32_t ret = CMW_ERROR_NONE;
  CMW_Sensor_Init_t initValues = {0};

  if (sensor_name == NULL)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  if ((is_camera_init != 0) && (active_sensor != NULL))
  {
    *sensor_name = active_sensor->name;
    return CMW_ERROR_NONE;
  }

  initValues.width = 0;
  initValues.height = 0;
  initValues.fps = 30;
  initValues.mirrorFlip = CMW_MIRRORFLIP_NONE;
  initValues.sensor_config = NULL;

  /* Set DCMIPP instance */
  hcamera_dcmipp.Instance = DCMIPP;

  /* Configure DCMIPP clock */
  ret = MX_DCMIPP_ClockConfig(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }
  /* Enable DCMIPP clock */
  ret = HAL_DCMIPP_Init(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  CMW_CAMERA_EnableGPIOs();
#if !defined (CMW_USE_WITHOUT_ISP)
  appliHelpers_ISP.SetSensorGain = CB_ISP_SetSensorGain;
  appliHelpers_ISP.GetSensorGain = CB_ISP_GetSensorGain;
  appliHelpers_ISP.SetSensorExposure = CB_ISP_SetSensorExposure;
  appliHelpers_ISP.GetSensorExposure = CB_ISP_GetSensorExposure;
  appliHelpers_ISP.GetSensorInfo = CB_ISP_GetSensorInfo;
#endif

  ret = CMW_CAMERA_Probe_Sensor(&initValues, NULL);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
  if (active_sensor == NULL)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
  *sensor_name = active_sensor->name;
  is_camera_init++;

  ret = CMW_CAMERA_DeInit();
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return CMW_ERROR_NONE;
}

/**
  * @brief  Set White Balance mode.
  * @param  automatic  If not null, set automatic white balance mode
  * @param  ref_color_temp  If automatic is null, set white balance mode
  * @retval CMW status
  */

int32_t CMW_CAMERA_SetWBRefMode(uint8_t automatic, uint32_t ref_color_temp)
{
  int ret;

  if (Camera_Drv.SetWBRefMode == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetWBRefMode(active_sensor->sensor_ctx, automatic, ref_color_temp);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = CMW_ERROR_NONE;
  /* Return CMW status */
  return ret;
}

/**
  * @brief  Get White Balance reference modes list.
  * @param  ref_color_temp  White Balance reference modes. Must point to an array
  *                         of at least CMW_CAMERA_NB_WB_REF_MODES entries.
  * @param  array_size      Number of entries available in ref_color_temp
  * @retval CMW status
  */
int32_t CMW_CAMERA_ListWBRefModes(uint32_t ref_color_temp[], uint32_t array_size)
{
  int ret;

  if ((ref_color_temp == NULL) || (array_size < CMW_CAMERA_NB_WB_REF_MODES))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  if (Camera_Drv.ListWBRefModes == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.ListWBRefModes(active_sensor->sensor_ctx, ref_color_temp, array_size);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = CMW_ERROR_NONE;
  /* Return CMW status */
  return ret;
}

/**
  * @brief  Probe camera sensor.
  * @param  initValues  Initialization values for the sensor
  * @param  sensorName  Camera sensor name
  * @retval CMW status
  */
static camera_sensor_t *CMW_CAMERA_FindSensorByName(const char *sensor_name)
{
  assert(sensor_name);
  for (int i = 0; i < cmw_sensor_registry_count; ++i)
  {
    camera_sensor_t *sensor = &cmw_sensor_registry[i];
    if ((sensor != NULL) && (strcmp(sensor->name, sensor_name) == 0))
    {
      return sensor;
    }
  }

  return NULL;
}

static int CMW_CAMERA_Probe_Sensor(CMW_Sensor_Init_t *initValues, const char *sensor_name)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  void *p_appliHelpers_ISP = &appliHelpers_ISP;
#else
  void *p_appliHelpers_ISP = NULL;
#endif

  if (sensor_name != NULL)
  {
    camera_sensor_t *sensor = CMW_CAMERA_FindSensorByName(sensor_name);
    if ((sensor == NULL) || (sensor->init == NULL) || (sensor->sensor_ctx == NULL))
    {
      return CMW_ERROR_WRONG_PARAM;
    }

    memset(sensor->sensor_ctx, 0, sizeof(sensor_ctx_u));
    /* active_sensor must be set prior to init because ISP_Init (called from the
     * sensor init) relies on the appliHelpers callbacks that use active_sensor */
    active_sensor = sensor;
    if (sensor->init(&Camera_Drv, sensor->sensor_ctx, &hcamera_dcmipp, initValues, p_appliHelpers_ISP) == 0)
    {
      return CMW_ERROR_NONE;
    }
    active_sensor = NULL;
    return CMW_ERROR_WRONG_PARAM;
  }

  const CMW_Sensor_Init_t requestedValues = *initValues;

  for (int i = 0; i < cmw_sensor_registry_count; ++i)
  {
    camera_sensor_t *sensor = &cmw_sensor_registry[i];
    if (sensor && sensor->init && sensor->sensor_ctx)
    {
      memset(sensor->sensor_ctx, 0, sizeof(sensor_ctx_u));
      *initValues = requestedValues;
      /* active_sensor must be set prior to init (see comment above) */
      active_sensor = sensor;
      if (sensor->init(&Camera_Drv, sensor->sensor_ctx, &hcamera_dcmipp, initValues, p_appliHelpers_ISP) == 0)
      {
        return CMW_ERROR_NONE;
      }
      active_sensor = NULL;
    }
  }
  return CMW_ERROR_WRONG_PARAM;
}

/**
  * @brief  Initializes the camera.
  * @param  initConf  Mandatory: General camera config
  * @param  advanced_config  Optional: Sensor specific configuration
  * @retval CMW status
  */
int32_t CMW_CAMERA_Init(CMW_CameraInit_t *initConf, CMW_Advanced_Config_t *advanced_config)
{
  int32_t ret = CMW_ERROR_NONE;
  CMW_Sensor_Init_t initValues = {0};
  CMW_Sensor_Info_t info = {0};
  const char *selected_sensor_name = NULL;

  if (is_camera_init > 0)
  {
    return CMW_ERROR_ALREADY_INITIALIZED;
  }

  initValues.width = initConf->width;
  initValues.height = initConf->height;
  initValues.fps = initConf->fps;
  initValues.mirrorFlip = initConf->mirror_flip;

  if (advanced_config != NULL)
  {
    selected_sensor_name = advanced_config->sensor_name;
    initValues.sensor_config = advanced_config->sensor_config;
  }
  else
  {
    selected_sensor_name = NULL;
    initValues.sensor_config = NULL;
  }

  /* Set DCMIPP instance */
  hcamera_dcmipp.Instance = DCMIPP;

  /* Configure DCMIPP clock */
  ret = MX_DCMIPP_ClockConfig(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }
  /* Enable DCMIPP clock */
  ret = HAL_DCMIPP_Init(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  CMW_CAMERA_EnableGPIOs();

#if !defined (CMW_USE_WITHOUT_ISP)
  appliHelpers_ISP.SetSensorGain = CB_ISP_SetSensorGain;
  appliHelpers_ISP.GetSensorGain = CB_ISP_GetSensorGain;
  appliHelpers_ISP.SetSensorExposure = CB_ISP_SetSensorExposure;
  appliHelpers_ISP.GetSensorExposure = CB_ISP_GetSensorExposure;
  appliHelpers_ISP.GetSensorInfo = CB_ISP_GetSensorInfo;
#endif

  ret = CMW_CAMERA_Probe_Sensor(&initValues, selected_sensor_name);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }

  /* Configure exposure and gain for a more suitable quality */
  ret = CMW_CAMERA_GetSensorInfo(&info);
  if (ret == CMW_ERROR_COMPONENT_FAILURE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
  ret = CMW_CAMERA_SetExposure(info.exposure_min);
  if (ret == CMW_ERROR_COMPONENT_FAILURE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
  ret = CMW_CAMERA_SetGain(info.gain_min);
  if (ret == CMW_ERROR_COMPONENT_FAILURE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }

  /* Write back the initValue width and height that might be changed */
  initConf->width = initValues.width;
  initConf->height = initValues.height ;
  camera_conf = *initConf;

#if !defined (CMW_USE_WITHOUT_ISP)
  ret = Camera_Drv.GetIspDecimationRatio(active_sensor->sensor_ctx, &Camera_Ctx.isp_decimation_ratio_h,
                                         &Camera_Ctx.isp_decimation_ratio_v);
  if (ret == CMW_ERROR_COMPONENT_FAILURE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
#endif

  is_camera_init++;
  /* CMW status */
  ret = CMW_ERROR_NONE;

  return ret;
}

/**
  * @brief  Set the camera Mirror/Flip.
  * @param  mirror_flip CMW_MIRRORFLIP_NONE CMW_MIRRORFLIP_FLIP CMW_MIRRORFLIP_MIRROR CMW_MIRRORFLIP_FLIP_MIRROR
  * @retval CMW status
*/
int32_t CMW_CAMERA_SetMirrorFlip(CMW_MirrorFlip_t mirror_flip)
{
  int ret;

  if (Camera_Drv.SetMirrorFlip == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetMirrorFlip(active_sensor->sensor_ctx, mirror_flip);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  camera_conf.mirror_flip = mirror_flip;
  ret = CMW_ERROR_NONE;
  /* Return CMW status */
  return ret;
}

/**
  * @brief  Get the camera Mirror/Flip.
  * @param  mirror_flip CMW_MIRRORFLIP_NONE CMW_MIRRORFLIP_FLIP CMW_MIRRORFLIP_MIRROR CMW_MIRRORFLIP_FLIP_MIRROR
  * @retval CMW status
*/
int32_t CMW_CAMERA_GetMirrorFlip(CMW_MirrorFlip_t *mirror_flip)
{
  *mirror_flip = camera_conf.mirror_flip;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff pointer to the camera output buffer
  * @param  mode  CMW_CAPTUREMODE_CONTINUOUS or CMW_CAPTUREMODE_SNAPSHOT
  * @retval CMW status
  */
int32_t CMW_CAMERA_Start(uint32_t pipe, uint8_t *pbuff, CMW_CaptureMode_t mode)
{
  int32_t ret = CMW_ERROR_NONE;

  if (pipe >= DCMIPP_NUM_OF_PIPES)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = HAL_DCMIPP_CSI_PIPE_Start(&hcamera_dcmipp, pipe, DCMIPP_VIRTUAL_CHANNEL0, (uint32_t)pbuff, mode);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  if (!is_camera_started)
  {
    ret = Camera_Drv.Start(active_sensor->sensor_ctx);
    if (ret != CMW_ERROR_NONE)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
    is_camera_started++;
  }

  /* Return CMW status */
  return ret;
}

/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff1 pointer to the first camera output buffer
  * @param  pbuff2 pointer to the second camera output buffer
  * @param  mode  CMW_CAPTUREMODE_CONTINUOUS or CMW_CAPTUREMODE_SNAPSHOT
  * @retval CMW status
  */
int32_t CMW_CAMERA_DoubleBufferStart(uint32_t pipe, uint8_t *pbuff1, uint8_t *pbuff2, CMW_CaptureMode_t mode)
{
  int32_t ret = CMW_ERROR_NONE;

  if (pipe >= DCMIPP_NUM_OF_PIPES)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  if (HAL_DCMIPP_CSI_PIPE_DoubleBufferStart(&hcamera_dcmipp, pipe, DCMIPP_VIRTUAL_CHANNEL0, (uint32_t)pbuff1,
                                            (uint32_t)pbuff2, mode) != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  if (!is_camera_started)
  {
    ret = Camera_Drv.Start(active_sensor->sensor_ctx);
    if (ret != CMW_ERROR_NONE)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
    is_camera_started++;
  }

  /* Return CMW status */
  return ret;
}

/**
  * @brief  Stops the camera stream and all the pipes.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Stop(void)
{
  int32_t ret = CMW_ERROR_NONE;

  if (Camera_Drv.Stop == NULL)
  {
     return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  for (uint32_t pipe = DCMIPP_PIPE0; pipe <= DCMIPP_PIPE2; pipe++)
  {
    if (HAL_DCMIPP_PIPE_GetState(&hcamera_dcmipp, pipe) != HAL_DCMIPP_PIPE_STATE_RESET)
    {
      ret = HAL_DCMIPP_CSI_PIPE_Stop(&hcamera_dcmipp, pipe, DCMIPP_VIRTUAL_CHANNEL0);
      if (ret != HAL_OK)
      {
        return CMW_ERROR_PERIPH_FAILURE;
      }
    }
  }

  if (is_camera_started > 0)
  {
    ret = Camera_Drv.Stop(active_sensor->sensor_ctx);
    if (ret != CMW_ERROR_NONE)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
    is_camera_started--;
  }

  /* Return CMW status */
  return CMW_ERROR_NONE;
}


/**
  * @brief  DCMIPP Clock Config for DCMIPP.
  * @param  hdcmipp  DCMIPP Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);

  return HAL_OK;
}

/**
  * @brief  DeInitializes the camera.
  * @retval CMW status
  */
int32_t CMW_CAMERA_DeInit(void)
{
  int32_t ret = CMW_ERROR_NONE;

  if (is_camera_init <= 0)
  {
    return CMW_ERROR_NONE;
  }

  if (Camera_Drv.DeInit == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  if (is_camera_started > 0)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  /* De-initialize the camera module */
  ret = Camera_Drv.DeInit(active_sensor->sensor_ctx);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Set Camera in Power Down */
  CMW_CAMERA_PwrDown();

  if (is_pipe1_2_shared > 0)
  {
    is_pipe1_2_shared--;
  }

  ret = HAL_DCMIPP_DeInit(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  is_camera_init--;

  active_sensor = NULL;
  memset(&Camera_Drv, 0, sizeof(Camera_Drv));
  /* Return CMW status */
  return CMW_ERROR_NONE;
}

/**
  * @brief  Suspend the CAMERA capture on selected pipe
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Suspend(uint32_t pipe)
{
  HAL_DCMIPP_PipeStateTypeDef state = hcamera_dcmipp.PipeState[pipe];

  if (state == HAL_DCMIPP_PIPE_STATE_SUSPEND)
  {
    return CMW_ERROR_NONE;
  }
  else if (state > HAL_DCMIPP_PIPE_STATE_READY)
  {
    if (HAL_DCMIPP_PIPE_Suspend(&hcamera_dcmipp, pipe) != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return CMW status */
  return CMW_ERROR_NONE;
}

/**
  * @brief  Resume the CAMERA capture on selected pipe
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Resume(uint32_t pipe)
{
  HAL_DCMIPP_PipeStateTypeDef state = hcamera_dcmipp.PipeState[pipe];

  if (state == HAL_DCMIPP_PIPE_STATE_BUSY)
  {
    return CMW_ERROR_NONE;
  }
  else if (state > HAL_DCMIPP_PIPE_STATE_BUSY)
  {
    if (HAL_DCMIPP_PIPE_Resume(&hcamera_dcmipp, pipe) != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return CMW status */
  return CMW_ERROR_NONE;
}

#if !defined (CMW_USE_WITHOUT_ISP)
/**
  * @brief  Enable the Restart State. When enabled, at system restart, the ISP middleware configuration
  *         is restored from the last update before the restart.
  * @param  ISP_RestartState pointer to ISP Restart State. To use this mode in a Low Power use case, where
  *         the ISP state is applied at system wake up, this pointer must be in some retention memory.
  * @retval CMW status
  */
int32_t CMW_CAMERA_EnableRestartState(ISP_RestartStateTypeDef *ISP_RestartState)
{
  if (ISP_EnableRestartState(NULL, ISP_RestartState) != ISP_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Return CMW status */
  return CMW_ERROR_NONE;
}

/**
  * @brief  Disable the Restart State
  * @retval CMW status
  */
int32_t CMW_CAMERA_DisableRestartState()
{
  if (ISP_DisableRestartState(NULL) != ISP_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Return CMW status */
  return CMW_ERROR_NONE;
}
#endif /* !CMW_USE_WITHOUT_ISP */

/**
  * @brief  Set the camera gain.
  * @param  Gain     Gain in mdB
  * @retval CMW status
  */
int CMW_CAMERA_SetGain(int32_t Gain)
{
  int ret;
  if(Camera_Drv.SetGain == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetGain(active_sensor->sensor_ctx, Gain);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  Camera_Ctx.Gain = Gain;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the camera gain.
  * @param  Gain     Gain in mdB
  * @retval CMW status
  */
int CMW_CAMERA_GetGain(int32_t *Gain)
{
  *Gain = Camera_Ctx.Gain;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Set the camera exposure.
  * @param  exposure exposure in microseconds
  * @retval CMW status
  */
int CMW_CAMERA_SetExposure(int32_t exposure)
{
  int ret;

  if(Camera_Drv.SetExposure == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetExposure(active_sensor->sensor_ctx, exposure);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  Camera_Ctx.Exposure = exposure;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the camera exposure.
  * @param  exposure exposure in microseconds
  * @retval CMW status
  */
int CMW_CAMERA_GetExposure(int32_t *exposure)
{
  *exposure = Camera_Ctx.Exposure;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Set the camera exposure mode.
  * @param  exposure_mode Exposure mode CMW_EXPOSUREMODE_AUTO, CMW_EXPOSUREMODE_AUTOFREEZE, CMW_EXPOSUREMODE_MANUAL
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetExposureMode(CMW_ExposureMode_t exposure_mode)
{
  int ret;

  if(Camera_Drv.SetExposureMode == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetExposureMode(active_sensor->sensor_ctx, exposure_mode);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  Camera_Ctx.ExposureMode = exposure_mode;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the camera exposure mode.
  * @param  exposure_mode Exposure mode CAMERA_EXPOSURE_AUTO, CAMERA_EXPOSURE_AUTOFREEZE, CAMERA_EXPOSURE_MANUAL
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetExposureMode(CMW_ExposureMode_t *exposure_mode)
{
  *exposure_mode = Camera_Ctx.ExposureMode;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Set (Enable/Disable and Configure) the camera test pattern
  * @param  mode Pattern mode (sensor specific value) to be configured. '-1' means disable.
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetTestPattern(int32_t mode)
{
  int32_t ret;

  if(Camera_Drv.SetTestPattern == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetTestPattern(active_sensor->sensor_ctx, mode);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  Camera_Ctx.TestPattern = mode;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the camera test pattern
  * @param  mode Pattern mode (sensor specific value) to be returned. '-1' means disable.
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetTestPattern(int32_t *mode)
{
  *mode = Camera_Ctx.TestPattern;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the Camera Sensor info.
  * @param  info  pointer to sensor info
  * @note   This function should be called after the init. This to get Capabilities
  *         from the camera sensor
  * @retval Component status
  */
int32_t CMW_CAMERA_GetSensorInfo(CMW_Sensor_Info_t *info)
{

  int32_t ret;

  if(Camera_Drv.GetSensorInfo == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.GetSensorInfo(active_sensor->sensor_ctx, info);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return CMW_ERROR_NONE;
}



int32_t CMW_CAMERA_Run()
{
  if(Camera_Drv.Run != NULL)
  {
      return Camera_Drv.Run(active_sensor->sensor_ctx);
  }
  return CMW_ERROR_NONE;
}

/**
 * @brief  Vsync Event callback on pipe
 * @param  Pipe  Pipe receiving the callback
 * @retval None
 */
__weak int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t pipe)
{
  UNUSED(pipe);

  return CMW_ERROR_NONE;
}

/**
 * @brief  Frame Event callback on pipe
 * @param  Pipe  Pipe receiving the callback
 * @retval None
 */
__weak int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe)
{
  UNUSED(pipe);

  return CMW_ERROR_NONE;
}

/**
 * @brief  Error callback on pipe
 * @param  Pipe  Pipe receiving the callback
 * @retval None
 */
__weak void CMW_CAMERA_PIPE_ErrorCallback(uint32_t pipe)
{
  assert(0);
}

/**
 * @brief  Vsync Event callback on pipe
 * @param  hdcmipp DCMIPP device handle
 *         Pipe    Pipe receiving the callback
 * @retval None
 */
void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);
  if(Camera_Drv.VsyncEventCallback != NULL)
  {
      Camera_Drv.VsyncEventCallback(active_sensor->sensor_ctx, Pipe);
  }
  CMW_CAMERA_PIPE_VsyncEventCallback(Pipe);
}

/**
 * @brief  Frame Event callback on pipe
 * @param  hdcmipp DCMIPP device handle
 *         Pipe    Pipe receiving the callback
 * @retval None
 */
void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);
  if(Camera_Drv.FrameEventCallback != NULL)
  {
      Camera_Drv.FrameEventCallback(active_sensor->sensor_ctx, Pipe);
  }
  CMW_CAMERA_PIPE_FrameEventCallback(Pipe);
}

/**
  * @brief  Initializes the DCMIPP MSP.
  * @param  hdcmipp  DCMIPP handle
  * @retval None
  */
void HAL_DCMIPP_MspInit(DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);

  /*** Enable peripheral clock ***/
  /* Enable DCMIPP clock */
  __HAL_RCC_DCMIPP_CLK_ENABLE();
  __HAL_RCC_DCMIPP_CLK_SLEEP_ENABLE();
  __HAL_RCC_DCMIPP_FORCE_RESET();
  __HAL_RCC_DCMIPP_RELEASE_RESET();

  /*** Configure the NVIC for DCMIPP ***/
  /* NVIC configuration for DCMIPP transfer complete interrupt */
  HAL_NVIC_SetPriority(DCMIPP_IRQn, 0x07, 0);
  HAL_NVIC_EnableIRQ(DCMIPP_IRQn);

  /*** Enable peripheral clock ***/
  /* Enable CSI clock */
  __HAL_RCC_CSI_CLK_ENABLE();
  __HAL_RCC_CSI_CLK_SLEEP_ENABLE();
  __HAL_RCC_CSI_FORCE_RESET();
  __HAL_RCC_CSI_RELEASE_RESET();

  /*** Configure the NVIC for CSI ***/
  /* NVIC configuration for CSI transfer complete interrupt */
  HAL_NVIC_SetPriority(CSI_IRQn, 0x07, 0);
  HAL_NVIC_EnableIRQ(CSI_IRQn);

}

/**
  * @brief  DeInitializes the DCMIPP MSP.
  * @param  hdcmipp  DCMIPP handle
  * @retval None
  */
void HAL_DCMIPP_MspDeInit(DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);

  __HAL_RCC_DCMIPP_FORCE_RESET();
  __HAL_RCC_DCMIPP_RELEASE_RESET();

  /* Disable NVIC  for DCMIPP transfer complete interrupt */
  HAL_NVIC_DisableIRQ(DCMIPP_IRQn);

  /* Disable DCMIPP clock */
  __HAL_RCC_DCMIPP_CLK_DISABLE();

  __HAL_RCC_CSI_FORCE_RESET();
  __HAL_RCC_CSI_RELEASE_RESET();

  /* Disable NVIC  for DCMIPP transfer complete interrupt */
  HAL_NVIC_DisableIRQ(CSI_IRQn);

  /* Disable DCMIPP clock */
  __HAL_RCC_CSI_CLK_DISABLE();
}

/**
  * @brief  CAMERA hardware reset
  * @retval CMW status
  */
static void CMW_CAMERA_EnableGPIOs(void)
{
  GPIO_InitTypeDef gpio_init_structure = {0};

  /* Enable GPIO clocks */
  EN_CAM_GPIO_ENABLE_VDDIO();
  EN_CAM_GPIO_CLK_ENABLE();
  NRST_CAM_GPIO_ENABLE_VDDIO();
  NRST_CAM_GPIO_CLK_ENABLE();

  gpio_init_structure.Pin       = EN_CAM_PIN;
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(EN_CAM_PORT, &gpio_init_structure);

  gpio_init_structure.Pin       = NRST_CAM_PIN;
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(NRST_CAM_PORT, &gpio_init_structure);
}

/**
  * @brief  CAMERA power down
  * @retval CMW status
  */
static void CMW_CAMERA_PwrDown(void)
{
  GPIO_InitTypeDef gpio_init_structure = {0};

  gpio_init_structure.Pin       = EN_CAM_PIN;
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(EN_CAM_PORT, &gpio_init_structure);

  gpio_init_structure.Pin       = NRST_CAM_PIN;
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(NRST_CAM_PORT, &gpio_init_structure);

  /* Camera power down sequence */
  /* Assert the camera Enable pin (active high) */
  HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, GPIO_PIN_RESET);

  /* De-assert the camera NRST pin (active low) */
  HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, GPIO_PIN_RESET);

}

static int32_t CMW_CAMERA_SetPipe(DCMIPP_HandleTypeDef *hdcmipp, uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch)
{
  int isp_decimation_ratio_h = Camera_Ctx.isp_decimation_ratio_h;
  int isp_decimation_ratio_v = Camera_Ctx.isp_decimation_ratio_v;
  DCMIPP_DecimationConfTypeDef dec_conf = { 0 };
  DCMIPP_PipeConfTypeDef pipe_conf = { 0 };
  DCMIPP_DownsizeTypeDef down_conf = { 0 };
  DCMIPP_CropConfTypeDef crop_conf = { 0 };
  int ret;

  /* specific case for pipe0 which is only a dump pipe */
  if (pipe == DCMIPP_PIPE0)
  {
    /*  TODO: properly configure the dump pipe with decimation and crop */
    pipe_conf.FrameRate = DCMIPP_FRAME_RATE_ALL;
    ret = HAL_DCMIPP_PIPE_SetConfig(hdcmipp, pipe, &pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }

    return CMW_ERROR_NONE;
  }

#if defined (CMW_USE_WITHOUT_ISP)
  isp_decimation_ratio_h = p_conf->isp_decimation_ratio_h;
  isp_decimation_ratio_v = p_conf->isp_decimation_ratio_v;
#endif

  /* Validate decimation ratios are valid values */
  if (!CMW_CAMERA_IS_VALID_DECIMATION_RATIO(isp_decimation_ratio_h) ||
      !CMW_CAMERA_IS_VALID_DECIMATION_RATIO(isp_decimation_ratio_v))
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  CMW_UTILS_GetPipeConfig(camera_conf.width, camera_conf.height, isp_decimation_ratio_h, isp_decimation_ratio_v, p_conf,
                          &crop_conf, &dec_conf, &down_conf);

  if (crop_conf.VSize != 0 || crop_conf.HSize != 0)
  {
    ret = HAL_DCMIPP_PIPE_SetCropConfig(hdcmipp, pipe, &crop_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }

    ret = HAL_DCMIPP_PIPE_EnableCrop(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = HAL_DCMIPP_PIPE_DisableCrop(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  if (dec_conf.VRatio != 0 || dec_conf.HRatio != 0)
  {
    ret = HAL_DCMIPP_PIPE_SetDecimationConfig(hdcmipp, pipe, &dec_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }

    ret = HAL_DCMIPP_PIPE_EnableDecimation(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = HAL_DCMIPP_PIPE_DisableDecimation(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  ret = HAL_DCMIPP_PIPE_SetDownsizeConfig(hdcmipp, pipe, &down_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = HAL_DCMIPP_PIPE_EnableDownsize(hdcmipp, pipe);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  if (p_conf->enable_swap)
  {
    /* Config pipe */
    ret = HAL_DCMIPP_PIPE_EnableRedBlueSwap(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = HAL_DCMIPP_PIPE_DisableRedBlueSwap(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  /* Ignore the configuration of gamma if -1
   * Activation is then done by the ISP Library
   */
  if (p_conf->enable_gamma_conversion > -1)
  {
    if (p_conf->enable_gamma_conversion)
    {
      ret = HAL_DCMIPP_PIPE_EnableGammaConversion(hdcmipp, pipe);
      if (ret != HAL_OK)
      {
        return CMW_ERROR_COMPONENT_FAILURE;
      }
    }
    else
    {
      ret = HAL_DCMIPP_PIPE_DisableGammaConversion(hdcmipp, pipe);
      if (ret != HAL_OK)
      {
        return CMW_ERROR_COMPONENT_FAILURE;
      }
    }
  }

  if (pipe == DCMIPP_PIPE2)
  {
    if (!is_pipe1_2_shared)
    {
      ret = HAL_DCMIPP_PIPE_CSI_EnableShare(hdcmipp, pipe);
      if (ret != HAL_OK)
      {
        return CMW_ERROR_COMPONENT_FAILURE;
      }
      is_pipe1_2_shared++;
    }
  }

  pipe_conf.FrameRate = DCMIPP_FRAME_RATE_ALL;
  pipe_conf.PixelPipePitch = p_conf->output_width * p_conf->output_bpp;
  /* Hardware constraint, pitch must be multiple of 16 */
  pipe_conf.PixelPipePitch = (pipe_conf.PixelPipePitch + 15) & (uint32_t) ~15;
  pipe_conf.PixelPackerFormat = p_conf->output_format;

  /* Support of YUV pixel format */
  if (pipe_conf.PixelPackerFormat == DCMIPP_PIXEL_PACKER_FORMAT_YUV422_1)
  {
    if (pipe != DCMIPP_PIPE1)
    {
      /* Only pipe 1 support YUV conversion */
      return CMW_ERROR_FEATURE_NOT_SUPPORTED;
    }

    #define N10(val) (((val) ^ 0x7FF) + 1)
    DCMIPP_ColorConversionConfTypeDef yuv_color_conf = {
    .ClampOutputSamples = ENABLE,
    .OutputSamplesType = 0,
    .RR = 131,     .RG = N10(110), .RB = N10(21), .RA = 128,
    .GR = 77,      .GG = 150,      .GB = 29,      .GA = 0,
    .BR = N10(44), .BG = N10(87),  .BB = 131,     .BA = 128,
    };

    ret = HAL_DCMIPP_PIPE_SetYUVConversionConfig(hdcmipp, pipe, &yuv_color_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
    ret = HAL_DCMIPP_PIPE_EnableYUVConversion(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  if (hcamera_dcmipp.PipeState[pipe] == HAL_DCMIPP_PIPE_STATE_RESET)
  {
    ret = HAL_DCMIPP_PIPE_SetConfig(hdcmipp, pipe, &pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    if (HAL_DCMIPP_PIPE_SetPixelPackerFormat(hdcmipp, pipe, pipe_conf.PixelPackerFormat) != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }

    if (HAL_DCMIPP_PIPE_SetPitch(hdcmipp, pipe, pipe_conf.PixelPipePitch) != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  /* Update the pitch field so that application can use this information for
   * buffer alignement */
  *pitch = pipe_conf.PixelPipePitch;

  return CMW_ERROR_NONE;
}

int32_t CMW_CAMERA_SetDefaultSensorValues( CMW_Advanced_Config_t *advanced_config)
{
  if (advanced_config == NULL)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  if ((advanced_config->sensor_name == NULL) || (advanced_config->sensor_config == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  camera_sensor_t *sensor = CMW_CAMERA_FindSensorByName(advanced_config->sensor_name);
  if (sensor == NULL)
  {
    return CMW_ERROR_WRONG_PARAM;
  }
  if (sensor->set_defaultSensorValues == NULL)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  sensor->set_defaultSensorValues(advanced_config->sensor_config);

  return CMW_ERROR_NONE;
}

#if !defined (CMW_USE_WITHOUT_ISP)
static ISP_StatusTypeDef CB_ISP_SetSensorGain(uint32_t camera_instance, int32_t gain)
{
  if (CMW_CAMERA_SetGain(gain) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORGAIN;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_GetSensorGain(uint32_t camera_instance, int32_t *gain)
{
  if (CMW_CAMERA_GetGain(gain) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORGAIN;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_SetSensorExposure(uint32_t camera_instance, int32_t exposure)
{
  if (CMW_CAMERA_SetExposure(exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_GetSensorExposure(uint32_t camera_instance, int32_t *exposure)
{
  if (CMW_CAMERA_GetExposure(exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_GetSensorInfo(uint32_t camera_instance, ISP_SensorInfoTypeDef *Info)
{
  CMW_Sensor_Info_t sensor_info;

  if(Camera_Drv.GetSensorInfo != NULL)
  {
    if (Camera_Drv.GetSensorInfo(active_sensor->sensor_ctx, &sensor_info) != CMW_ERROR_NONE)
      return ISP_ERR_SENSORINFO;

    /* Convert from cmw typedef to isp typedef */
    strncpy(Info->name, sensor_info.name, sizeof(Info->name) - 1);
    Info->name[sizeof(Info->name) - 1] = '\0';
    Info->bayer_pattern = sensor_info.bayer_pattern;
    Info->color_depth = sensor_info.color_depth;
    Info->width = sensor_info.width;
    Info->height = sensor_info.height;
    Info->gain_min = sensor_info.gain_min;
    Info->gain_max = sensor_info.gain_max;
    Info->again_max = sensor_info.again_max;
    Info->exposure_min = sensor_info.exposure_min;
    Info->exposure_max = sensor_info.exposure_max;
  }
  return ISP_OK;
}
#endif /* !CMW_USE_WITHOUT_ISP */

/**
  * @brief  Error callback on the pipe. Occurs when overrun occurs on the pipe.
  * @param  hdcmipp  Pointer to DCMIPP handle
  * @param  Pipe     Specifies the DCMIPP pipe, can be a value from @ref DCMIPP_Pipes
  * @retval None
  */
void HAL_DCMIPP_PIPE_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);

  CMW_CAMERA_PIPE_ErrorCallback(Pipe);
}
