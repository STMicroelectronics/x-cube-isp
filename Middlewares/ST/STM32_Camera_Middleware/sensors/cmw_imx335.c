/**
  ******************************************************************************
  * @file    cmw_imx335.c
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "cmw_imx335.h"
#include "cmw_camera.h"
#include "cmw_utils.h"
#include "imx335_reg.h"
#include "imx335.h"
#include "cmw_io.h"
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_param_conf.h"
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static int CMW_IMX335_GetResType(uint32_t width, uint32_t height, uint32_t*res)
{
  if (width == 2592 && height == 1944)
  {
    *res = IMX335_R2592_1944;
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  return 0;
}

static int32_t CMW_IMX335_getMirrorFlipConfig(uint32_t Config)
{
  int32_t ret;

  switch (Config)
  {
    case CMW_MIRRORFLIP_NONE:
      ret = IMX335_MIRROR_FLIP_NONE;
      break;
    case CMW_MIRRORFLIP_FLIP:
      ret = IMX335_FLIP;
      break;
    case CMW_MIRRORFLIP_MIRROR:
      ret = IMX335_MIRROR;
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
    default:
      ret = IMX335_MIRROR_FLIP;
      break;
  }

  return ret;
}

static int32_t CMW_IMX335_Stop(void *io_ctx)
{
  /*Not Implemented in the Driver */
  UNUSED(io_ctx);
  return CMW_ERROR_NONE;
}

static int32_t CMW_IMX335_DeInit(void *io_ctx)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
  int ret = CMW_ERROR_NONE;
#if !defined (CMW_USE_WITHOUT_ISP)
  ret = ISP_DeInit(&imx335_ctx->hIsp);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
#endif

  ret = IMX335_DeInit(&imx335_ctx->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return ret;
}

static int32_t CMW_IMX335_SetGain(void *io_ctx, int32_t gain)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;

  return IMX335_SetGain(&imx335_ctx->ctx_driver, gain);
}

static int32_t CMW_IMX335_SetExposure(void *io_ctx, int32_t exposure)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;

  return IMX335_SetExposure(&imx335_ctx->ctx_driver, exposure);
}

/**
  * @brief  Set the sensor white balance mode
  * @param  io_ctx  pointer to component object
  * @param  Automatic automatic mode enable/disable
  * @param  RefColorTemp color temperature if automatic mode is disabled
  * @retval Component status
  */
static int32_t CMW_IMX335_SetWBRefMode(void *io_ctx, uint8_t Automatic, uint32_t RefColorTemp)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  ret = ISP_SetWBRefMode(&imx335_ctx->hIsp, Automatic, RefColorTemp);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }
  return CMW_ERROR_NONE;
#else

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
#endif
}

/**
  * @brief  List the sensor white balance modes
  * @param  io_ctx  pointer to component object
  * @param  RefColorTemp color temperature list
  * @param  array_size number of entries available in RefColorTemp
  * @retval Component status
  */
static int32_t CMW_IMX335_ListWBRefModes(void *io_ctx, uint32_t RefColorTemp[], uint32_t array_size)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  assert(array_size >= CMW_CAMERA_NB_WB_REF_MODES);

  ret = ISP_ListWBRefModes(&imx335_ctx->hIsp, RefColorTemp);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }
  return CMW_ERROR_NONE;
#else

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
#endif
}

static int32_t CMW_IMX335_SetFrequency(void *io_ctx, int32_t frequency)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;

  return IMX335_SetFrequency(&imx335_ctx->ctx_driver, frequency);
}

static int32_t CMW_IMX335_SetFramerate(void *io_ctx, int32_t framerate)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
  const int32_t available_imx335_fps[] = {10, 15, 20, 25, 30};

  for (int i = 0; i < ARRAY_SIZE(available_imx335_fps); i++)
    if (framerate == available_imx335_fps[i])
      return IMX335_SetFramerate(&imx335_ctx->ctx_driver, framerate);

  return CMW_ERROR_WRONG_PARAM;
}

static int32_t CMW_IMX335_SetMirrorFlip(void *io_ctx, uint32_t config)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
  int32_t mirrorFlip = CMW_IMX335_getMirrorFlipConfig(config);

  return IMX335_MirrorFlipConfig(&imx335_ctx->ctx_driver, mirrorFlip);
}

static int32_t CMW_IMX335_GetSensorInfo(void *io_ctx, CMW_Sensor_Info_t *info)
{
  if ((io_ctx ==  NULL) || (info == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  if (sizeof(info->name) >= strlen(IMX335_NAME) + 1)
  {
    strcpy(info->name, IMX335_NAME);
  }
  else
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  info->bayer_pattern = IMX335_BAYER_PATTERN;
  info->color_depth = IMX335_COLOR_DEPTH;
  info->width = IMX335_WIDTH;
  info->height = IMX335_HEIGHT;
  info->gain_min = IMX335_GAIN_MIN;
  info->gain_max = IMX335_GAIN_MAX;
  info->again_max = IMX335_AGAIN_MAX;
  info->exposure_min = IMX335_EXPOSURE_MIN;
  info->exposure_max = IMX335_EXPOSURE_MAX;

  return CMW_ERROR_NONE;
}

static int32_t CMW_IMX335_SetTestPattern(void *io_ctx, int32_t mode)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;

  return IMX335_SetTestPattern(&imx335_ctx->ctx_driver, mode);
}

static int32_t CMW_IMX335_GetIspDecimationRatio(void *io_ctx, int32_t *ratio_h, int32_t *ratio_v)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX335_t *ctx = (CMW_IMX335_t *) io_ctx;

  return CMW_UTILS_GetIspDecimationRatio_WithIsp(&ctx->hIsp, ratio_h, ratio_v);
#else
  return CMW_UTILS_GetIspDecimationRatio_NoIsp(ratio_h, ratio_v);
#endif
}

static int32_t CMW_IMX335_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor, void *p_appliHelpers_ISP)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
  int ret = CMW_ERROR_NONE;
  uint32_t resolution;
  CMW_IMX335_config_t *sensor_config;
  sensor_config = (CMW_IMX335_config_t*)(initSensor->sensor_config);
  assert(sensor_config != NULL);

  ret = CMW_IMX335_GetResType(initSensor->width, initSensor->height, &resolution);
  if (ret)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = CMW_IMX335_SetMirrorFlip(io_ctx, initSensor->mirrorFlip);
  if (ret)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = IMX335_Init(&imx335_ctx->ctx_driver, resolution, sensor_config->pixel_format);
  if (ret != IMX335_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

#if !defined (CMW_USE_WITHOUT_ISP)
  /* Statistic area is provided with null value so that it force the ISP Library to get the statistic
   * area information from the tuning file.
   */
  (void) ISP_IQParamCacheInit; /* unused */
  ret = ISP_Init(&imx335_ctx->hIsp, imx335_ctx->hdcmipp, 0, (ISP_AppliHelpersTypeDef *)p_appliHelpers_ISP, &ISP_IQParamCacheInit_IMX335);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = ISP_SetAEConvergenceSpeed(&imx335_ctx->hIsp, ISP_AE_CONVERGENCESPEED_MEDIUM);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = ISP_SetAWBConvergenceSpeed(&imx335_ctx->hIsp, ISP_AWB_CONVERGENCESPEED_MEDIUM);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_WRONG_PARAM;
  }
#endif

  return CMW_ERROR_NONE;
}

void CMW_IMX335_SetDefaultSensorValues(void *sensor_config)
{
  assert(sensor_config != NULL);
  CMW_IMX335_config_t *imx335_config = (CMW_IMX335_config_t *)sensor_config;
  imx335_config->pixel_format = CMW_PIXEL_FORMAT_RAW10;
}

static int32_t CMW_IMX335_Start(void *io_ctx)
{
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
#if !defined (CMW_USE_WITHOUT_ISP)
  int ret;
  ret = ISP_Start(&imx335_ctx->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif

  return IMX335_Start(&imx335_ctx->ctx_driver);
}

static int32_t CMW_IMX335_Run(void *io_ctx)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
  int ret;
  ret = ISP_BackgroundProcess(&imx335_ctx->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif

  return CMW_ERROR_NONE;
}

static void CMW_IMX335_PowerOn(CMW_IMX335_t *io_ctx)
{
  /* Note: According to the IMX335 datasheet (E18112A84), the power-on sequence
   * should be as followed:
   * 1. Turn On the power supplies so that the power supplies rise in order of
   *    1.2V power supply (DVDD),
   *    1.8V power supply (OVDD),
   *    2.9V power supply (AVDD)
   *   with a rise time of 0 to 200ms.
   *   In addition, all power supplies should finish rising within 200ms.
   * 2. The register values are undefined immediately after power-on, so the
   *    system must be cleared. Hold XCLR at Low level for 500 ns or more after
   *    all the power supplies have finished rising. (The register values after
   *    a system clear are the default values.)
   * 3. The system clear is applied by setting XCLR to High level. The master
   *    clock input after setting the XCLR pin to High level.
   * 4. Make the sensor setting by register communication after the system
   *    clear.
   *
   * By default, on the MB1939 STM32N6-DK board, OVDD (1.8V) and AVDD (2.8V) are
   * always on when PWR_ON is high (VDD3V3 and VCORE ON). Power-on order cannot
   * be followed.
   *
   * MB1938 N6-DK and MB1854 IMX335 board IO mapping:
   *  ShutdownPin PC8 (CN14-17) -- (CN1-17) NRST_CAM  -- 1V8 (CN2-10) RESET (XCLR?)
   *  EnablePin   PD2 (CN14-18) -- (CN1-18) EN_MODULE -- 1V2 enable (CN2-6) DVDD
   */
  io_ctx->EnablePin(1);   /* Enable MB1854 1V2: DVDD and 24MHz CAM_CLK*/
  io_ctx->ShutdownPin(0); /* Set RESET low */
  io_ctx->Delay(1);       /* Hold RESET low for at least 500 ns after power supply have finished rising */
  io_ctx->ShutdownPin(1); /* Release RESET */
  io_ctx->Delay(1);      /* Wait for sensor to be ready */
}

static void CMW_IMX335_VsyncEventCallback(void *io_ctx, uint32_t pipe)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  /* Update the ISP frame counter and call its statistics handler */
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)io_ctx;
  switch (pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&imx335_ctx->hIsp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&imx335_ctx->hIsp);
      ISP_GatherStatistics(&imx335_ctx->hIsp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&imx335_ctx->hIsp);
      break;
  }
#endif
}

static void CMW_IMX335_FrameEventCallback(void *io_ctx, uint32_t pipe)
{
}

static int CMW_IMX335_Probe(CMW_IMX335_t *io_ctx, CMW_Sensor_if_t *imx335_if)
{
  int ret = CMW_ERROR_NONE;
  uint32_t id;
  io_ctx->ctx_driver.IO.Address = io_ctx->Address;
  io_ctx->ctx_driver.IO.Init = io_ctx->Init;
  io_ctx->ctx_driver.IO.DeInit = io_ctx->DeInit;
  io_ctx->ctx_driver.IO.GetTick = io_ctx->GetTick;
  io_ctx->ctx_driver.IO.ReadReg = io_ctx->ReadReg;
  io_ctx->ctx_driver.IO.WriteReg = io_ctx->WriteReg;

  CMW_IMX335_PowerOn(io_ctx);

  ret = IMX335_RegisterBusIO(&io_ctx->ctx_driver, &io_ctx->ctx_driver.IO);
  if (ret != IMX335_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = IMX335_ReadID(&io_ctx->ctx_driver, &id);
  if (ret != IMX335_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  if (id != IMX335_CHIP_ID)
  {
      ret = CMW_ERROR_UNKNOWN_COMPONENT;
  }

  memset(imx335_if, 0, sizeof(*imx335_if));
  imx335_if->Start = CMW_IMX335_Start;
  imx335_if->Stop = CMW_IMX335_Stop;
  imx335_if->DeInit = CMW_IMX335_DeInit;
  imx335_if->Run = CMW_IMX335_Run;
  imx335_if->VsyncEventCallback = CMW_IMX335_VsyncEventCallback;
  imx335_if->FrameEventCallback = CMW_IMX335_FrameEventCallback;
  imx335_if->SetGain = CMW_IMX335_SetGain;
  imx335_if->SetExposure = CMW_IMX335_SetExposure;
  imx335_if->SetWBRefMode = CMW_IMX335_SetWBRefMode;
  imx335_if->ListWBRefModes = CMW_IMX335_ListWBRefModes;
  imx335_if->SetFramerate = CMW_IMX335_SetFramerate;
  imx335_if->SetMirrorFlip = CMW_IMX335_SetMirrorFlip;
  imx335_if->GetSensorInfo = CMW_IMX335_GetSensorInfo;
  imx335_if->SetTestPattern = CMW_IMX335_SetTestPattern;
  imx335_if->GetIspDecimationRatio = CMW_IMX335_GetIspDecimationRatio;

  return ret;
}

static void CMW_IMX335_ShutdownPin(int value)
{
  HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void CMW_IMX335_EnablePin(int value)
{
  HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int32_t CMW_CAMERA_IMX335_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx,  DCMIPP_HandleTypeDef *hdcmipp, CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP)
{
  int32_t ret = CMW_ERROR_NONE;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };
  uint32_t dt_format = 0;
  uint32_t dt = 0;
  CMW_IMX335_t *imx335_ctx = (CMW_IMX335_t *)sensor_ctx;
  CMW_IMX335_config_t default_sensor_config;
  CMW_IMX335_config_t *sensor_config;

  memset(imx335_ctx, 0, sizeof(*imx335_ctx));
  imx335_ctx->Address     = CAMERA_IMX335_ADDRESS;
  imx335_ctx->Init        = CMW_I2C_INIT;
  imx335_ctx->DeInit      = CMW_I2C_DEINIT;
  imx335_ctx->ReadReg     = CMW_I2C_READREG16;
  imx335_ctx->WriteReg    = CMW_I2C_WRITEREG16;
  imx335_ctx->GetTick     = BSP_GetTick;
  imx335_ctx->Delay       = HAL_Delay;
  imx335_ctx->ShutdownPin = CMW_IMX335_ShutdownPin;
  imx335_ctx->EnablePin   = CMW_IMX335_EnablePin;
  imx335_ctx->hdcmipp     = hdcmipp;

  ret = CMW_IMX335_Probe(imx335_ctx, camera_drv);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Special case: when resolution is not specified take the full sensor resolution */
  if ((initSensors_params->width == 0) || (initSensors_params->height == 0))
  {
    CMW_Sensor_Info_t sensor_info;
    camera_drv->GetSensorInfo(imx335_ctx, &sensor_info);
    initSensors_params->width = sensor_info.width;
    initSensors_params->height = sensor_info.height;
  }

  CMW_IMX335_SetDefaultSensorValues(&default_sensor_config);
  initSensors_params->sensor_config = initSensors_params->sensor_config ? initSensors_params->sensor_config : &default_sensor_config;
  sensor_config = (CMW_IMX335_config_t*) (initSensors_params->sensor_config);

  ret = CMW_IMX335_SetFrequency(imx335_ctx, IMX335_INCK_24MHZ);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = camera_drv->SetFramerate(imx335_ctx, initSensors_params->fps);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  switch (sensor_config->pixel_format)
  {
    case CMW_PIXEL_FORMAT_DEFAULT:
    case CMW_PIXEL_FORMAT_RAW10:
    {
      dt_format = DCMIPP_CSI_DT_BPP10;
      dt = DCMIPP_DT_RAW10;
      break;
    }
    default:
      return CMW_ERROR_COMPONENT_FAILURE;
  }

  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_1600;
  ret = HAL_DCMIPP_CSI_SetConfig(hdcmipp, &csi_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = HAL_DCMIPP_CSI_SetVCConfig(hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, dt_format);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  csi_pipe_conf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  csi_pipe_conf.DataTypeIDA = dt;
  csi_pipe_conf.DataTypeIDB = 0;
  /* Pre-initialize CSI config for all the pipes */
  for (uint32_t i = DCMIPP_PIPE0; i <= DCMIPP_PIPE2; i++)
  {
    ret = HAL_DCMIPP_CSI_PIPE_SetConfig(hdcmipp, i, &csi_pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }

  ret = CMW_IMX335_Init(imx335_ctx, initSensors_params, p_appliHelpers_ISP);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return ret;
}

