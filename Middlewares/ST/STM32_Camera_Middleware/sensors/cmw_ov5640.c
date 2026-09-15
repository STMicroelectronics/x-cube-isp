/**
  ******************************************************************************
  * @file    cmw_ov5640.c
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
#include "cmw_ov5640.h"
#include "cmw_camera.h"
#include "cmw_io.h"
#include "cmw_utils.h"
#include "ov5640_reg.h"
#include "ov5640.h"

/**
  * @brief  Get the sensor info
  * @param  pObj  pointer to component object
  * @param  pInfo pointer to sensor info structure
  * @retval Component status
  */
static int32_t CMW_OV5640_GetSensorInfo(void *io_ctx, CMW_Sensor_Info_t *info)
{
  if ((io_ctx ==  NULL) || (info == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  /* Return the default full resolution */
  info->width = 640;
  info->height = 480;

  return CMW_ERROR_NONE;
}

static int32_t CMW_OV5640_GetIspDecimationRatio(void *io_ctx, int32_t *ratio_h, int32_t *ratio_v)
{
  return CMW_UTILS_GetIspDecimationRatio_NoIsp(ratio_h, ratio_v);
}

static int CMW_OV5640_GetResType(uint32_t width, uint32_t height, uint32_t*res)
{
  if (width == 160 && height == 120)
  {
    *res = OV5640_R160x120;
  }
  else if (width == 320 && height == 240)
  {
    *res = OV5640_R320x240;
  }
  else if (width == 480 && height == 272)
  {
    *res = OV5640_R480x272;
  }
  else if (width == 640 && height == 480)
  {
    *res = OV5640_R640x480;
  }
  else if (width == 800 && height == 480)
  {
    *res = OV5640_R800x480;
  }
  else
  {
    return -1;
  }

  return 0;
}

static int32_t CMW_OV5640_getMirrorFlipConfig(uint32_t Config)
{
  switch (Config) {
    case CMW_MIRRORFLIP_NONE:
      return OV5640_MIRROR_FLIP_NONE;
      break;
    case CMW_MIRRORFLIP_FLIP:
      return OV5640_FLIP;
      break;
    case CMW_MIRRORFLIP_MIRROR:
      return OV5640_MIRROR;
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
      return OV5640_MIRROR_FLIP;
      break;
    default:
      return CMW_ERROR_PERIPH_FAILURE;
  }
}

static int32_t CMW_OV5640_DeInit(void *io_ctx)
{
  CMW_OV5640_t *ov5640_ctx = (CMW_OV5640_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  ret = OV5640_DeInit(&ov5640_ctx->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return ret;
}

static int32_t CMW_OV5640_SetMirrorFlip(void *io_ctx, uint32_t config)
{
  CMW_OV5640_t *ov5640_ctx = (CMW_OV5640_t *)io_ctx;
  int32_t mirrorFlip = CMW_OV5640_getMirrorFlipConfig(config);

  return OV5640_MirrorFlipConfig(&ov5640_ctx->ctx_driver, mirrorFlip);
}

static int32_t CMW_OV5640_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor)
{
  CMW_OV5640_t *ov5640_ctx = (CMW_OV5640_t *)io_ctx;
  int ret = CMW_ERROR_NONE;
  uint32_t resolution;
  uint32_t pixelFormat;
  CMW_OV5640_config_t *sensor_config;
  sensor_config = (CMW_OV5640_config_t*)(initSensor->sensor_config);
  assert(sensor_config != NULL);

  ret = CMW_OV5640_GetResType(initSensor->width, initSensor->height, &resolution);
  if (ret)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  switch (sensor_config->pixel_format)
  {

    case CMW_PIXEL_FORMAT_YUV422_8:
    {
      pixelFormat = OV5640_YUV422;
      break;
    }
    case CMW_PIXEL_FORMAT_DEFAULT:
    case CMW_PIXEL_FORMAT_RGB565:
    {
      pixelFormat = OV5640_RGB565;
      break;
    }
    case CMW_PIXEL_FORMAT_RGB888:
    {
      pixelFormat = OV5640_RGB888;
      break;
    }
    case CMW_PIXEL_FORMAT_RAW8:
    {
      pixelFormat = OV5640_Y8;
      break;
    }
    default:
      return CMW_ERROR_COMPONENT_FAILURE;
      break;
  }

  ret = OV5640_Init(&ov5640_ctx->ctx_driver, resolution, pixelFormat);
  if (ret != OV5640_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = CMW_OV5640_SetMirrorFlip(io_ctx, initSensor->mirrorFlip);
  if (ret)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  return CMW_ERROR_NONE;
}

void CMW_OV5640_SetDefaultSensorValues(void *sensor_config)
{
  assert(sensor_config != NULL);
  CMW_OV5640_config_t *ov5640_config = (CMW_OV5640_config_t *)sensor_config;
  ov5640_config->pixel_format = CMW_PIXEL_FORMAT_RGB565;
}

static int32_t CMW_OV5640_Start(void *io_ctx)
{
  CMW_OV5640_t *ov5640_ctx = (CMW_OV5640_t *)io_ctx;

  return OV5640_Start(&ov5640_ctx->ctx_driver);
}

static void CMW_OV5640_PowerOn(CMW_OV5640_t *io_ctx)
{
  /* Camera sensor Power-On sequence */
  /* Assert the camera  NRST pins */
  io_ctx->EnablePin(1);
  io_ctx->ShutdownPin(0);  /* Disable MB1723 2V8 signal  */
  io_ctx->Delay(200); /* NRST signals asserted during 200ms */
  /* De-assert the camera STANDBY pin (active high) */
  io_ctx->ShutdownPin(1);  /* Disable MB1723 2V8 signal  */
	io_ctx->Delay(20);     /* NRST de-asserted during 20ms */
}

static int CMW_OV5640_Probe(CMW_OV5640_t *io_ctx, CMW_Sensor_if_t *ov5640_if)
{
  int ret = CMW_ERROR_NONE;
  uint32_t id;
  io_ctx->ctx_driver.IO.Address = io_ctx->Address;
  io_ctx->ctx_driver.IO.Init = io_ctx->Init;
  io_ctx->ctx_driver.IO.DeInit = io_ctx->DeInit;
  io_ctx->ctx_driver.IO.GetTick = io_ctx->GetTick;
  io_ctx->ctx_driver.IO.ReadReg = io_ctx->ReadReg;
  io_ctx->ctx_driver.IO.WriteReg = io_ctx->WriteReg;
  io_ctx->ctx_driver.Mode = SERIAL_MODE;
  io_ctx->ctx_driver.VirtualChannelID = 0U;

  CMW_OV5640_PowerOn(io_ctx);

  ret = OV5640_RegisterBusIO(&io_ctx->ctx_driver, &io_ctx->ctx_driver.IO);
  if (ret != OV5640_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = OV5640_ReadID(&io_ctx->ctx_driver, &id);
  if (ret != OV5640_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  if (id != OV5640_ID)
  {
      ret = CMW_ERROR_UNKNOWN_COMPONENT;
  }

  memset(ov5640_if, 0, sizeof(*ov5640_if));
  ov5640_if->Start = CMW_OV5640_Start;
  ov5640_if->DeInit = CMW_OV5640_DeInit;
  ov5640_if->SetMirrorFlip = CMW_OV5640_SetMirrorFlip;
  ov5640_if->GetSensorInfo = CMW_OV5640_GetSensorInfo;
  ov5640_if->GetIspDecimationRatio = CMW_OV5640_GetIspDecimationRatio;

  return ret;
}

static void CMW_OV5640_ShutdownPin(int value)
{
  HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void CMW_OV5640_EnablePin(int value)
{
  HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int32_t CMW_CAMERA_OV5640_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx, DCMIPP_HandleTypeDef *hdcmipp,
                              CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP)
{
  int32_t ret = CMW_ERROR_NONE;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };
  uint32_t dt_format = 0;
  uint32_t dt = 0;
  CMW_OV5640_config_t default_sensor_config;
  CMW_OV5640_config_t *sensor_config;
  CMW_OV5640_t *ov5640_ctx = (CMW_OV5640_t *)sensor_ctx;

  memset(ov5640_ctx, 0, sizeof(*ov5640_ctx));
  ov5640_ctx->Address     = CAMERA_OV5640_ADDRESS;
  ov5640_ctx->Init        = CMW_I2C_INIT;
  ov5640_ctx->DeInit      = CMW_I2C_DEINIT;
  ov5640_ctx->WriteReg    = CMW_I2C_WRITEREG16;
  ov5640_ctx->ReadReg     = CMW_I2C_READREG16;
  ov5640_ctx->GetTick     = BSP_GetTick;
  ov5640_ctx->Delay       = HAL_Delay;
  ov5640_ctx->ShutdownPin = CMW_OV5640_ShutdownPin;
  ov5640_ctx->EnablePin   = CMW_OV5640_EnablePin;

  ret = CMW_OV5640_Probe(ov5640_ctx, camera_drv);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Special case: when resolution is not specified take the full sensor resolution */
  if ((initSensors_params->width == 0) || (initSensors_params->height == 0))
  {
    CMW_Sensor_Info_t sensor_info;
    camera_drv->GetSensorInfo(ov5640_ctx, &sensor_info);
    initSensors_params->width = sensor_info.width;
    initSensors_params->height = sensor_info.height;
  }

  CMW_OV5640_SetDefaultSensorValues(&default_sensor_config);
  initSensors_params->sensor_config = initSensors_params->sensor_config ? initSensors_params->sensor_config : &default_sensor_config;
  sensor_config = (CMW_OV5640_config_t*) (initSensors_params->sensor_config);

  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_250;
  ret = HAL_DCMIPP_CSI_SetConfig(hdcmipp, &csi_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  switch (sensor_config->pixel_format)
  {
    case CMW_PIXEL_FORMAT_YUV422_8:
    {
      dt_format = DCMIPP_CSI_DT_BPP8;
      dt = DCMIPP_DT_YUV422_8;
      break;
    }
    case CMW_PIXEL_FORMAT_DEFAULT:
    case CMW_PIXEL_FORMAT_RGB565:
    {
      dt_format = DCMIPP_CSI_DT_BPP8;
      dt = DCMIPP_DT_RGB565;
      break;
    }
    case CMW_PIXEL_FORMAT_RGB888:
    {
      dt_format = DCMIPP_CSI_DT_BPP8;
      dt = DCMIPP_DT_RGB888;
      break;
    }
    case CMW_PIXEL_FORMAT_RAW8:
    {
      dt_format = DCMIPP_CSI_DT_BPP8;
      dt = DCMIPP_DT_RAW8;
      break;
    }
    default:
      return CMW_ERROR_COMPONENT_FAILURE;
      break;
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

  ret = CMW_OV5640_Init(ov5640_ctx, initSensors_params);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return CMW_ERROR_NONE;
}
