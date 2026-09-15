/**
  ******************************************************************************
  * @file    cmw_<sensor>.c
  * @brief   Template sensor adaptation layer for stm32-mw-camera.
  *
  * Replace:
  * - SENSORNAME  -> your sensor name in uppercase (example: IMX335)
  * - SENSORNAME  -> your sensor name in lowercase (example: imx335)
  * - SENSORNAME  -> your sensor name in CamelCase if needed by your vendor driver
  * - You can leverage the closest existing driver:
  * - RAW Bayer + ISP: `cmw_imx335` or `cmw_vd66gy`
  * - RGB/YUV no ISP usage: `cmw_ov5640`
  ******************************************************************************
  */

#include <assert.h>
#include <string.h>
#include "cmw_sensor_template.h"
#include "cmw_camera.h"
#include "cmw_io.h"
#include "cmw_utils.h"

#ifndef CAMERA_SENSORNAME_ADDRESS
#define CAMERA_SENSORNAME_ADDRESS        0x20U
#endif

#ifndef CAMERA_SENSORNAME_FREQ_IN_HZ
#define CAMERA_SENSORNAME_FREQ_IN_HZ     24000000U
#endif

#ifndef SENSORNAME_NATIVE_FORMAT
#define SENSORNAME_NATIVE_FORMAT         CMW_PIXEL_FORMAT_RAW10
#endif

#ifndef SENSORNAME_CSI_PHY_BITRATE
#define SENSORNAME_CSI_PHY_BITRATE       DCMIPP_CSI_PHY_BT_800
#endif

static int32_t SENSORNAME_VENDOR_RegisterBusIO(CMW_SENSORNAME_t *io_ctx)
{
  UNUSED(io_ctx);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t SENSORNAME_VENDOR_ReadID(CMW_SENSORNAME_t *io_ctx, uint32_t *id)
{
  UNUSED(io_ctx);
  UNUSED(id);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t SENSORNAME_VENDOR_Init(CMW_SENSORNAME_t *io_ctx,
                                     CMW_Sensor_Init_t *initSensor,
                                     CMW_SENSORNAME_config_t *sensor_config)
{
  UNUSED(io_ctx);
  UNUSED(initSensor);
  UNUSED(sensor_config);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t SENSORNAME_VENDOR_Start(CMW_SENSORNAME_t *io_ctx)
{
  UNUSED(io_ctx);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t SENSORNAME_VENDOR_DeInit(CMW_SENSORNAME_t *io_ctx)
{
  UNUSED(io_ctx);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t CMW_SENSORNAME_GetSensorInfo(void *io_ctx, CMW_Sensor_Info_t *info)
{
  if ((io_ctx == NULL) || (info == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  memset(info, 0, sizeof(*info));

  return CMW_ERROR_NONE;
}

static int32_t CMW_SENSORNAME_GetIspDecimationRatio(void *io_ctx, int32_t *ratio_h, int32_t *ratio_v)
{
  return CMW_UTILS_GetIspDecimationRatio_NoIsp(ratio_h, ratio_v);
}

static int32_t CMW_SENSORNAME_DeInit(void *io_ctx)
{
  return SENSORNAME_VENDOR_DeInit((CMW_SENSORNAME_t *)io_ctx);
}

static int32_t CMW_SENSORNAME_SetMirrorFlip(void *io_ctx, uint32_t config)
{
  UNUSED(io_ctx);
  UNUSED(config);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t CMW_SENSORNAME_SetGain(void *io_ctx, int32_t gain)
{
  UNUSED(io_ctx);
  UNUSED(gain);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t CMW_SENSORNAME_SetExposure(void *io_ctx, int32_t exposure)
{
  UNUSED(io_ctx);
  UNUSED(exposure);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t CMW_SENSORNAME_SetExposureMode(void *io_ctx, int32_t mode)
{
  UNUSED(io_ctx);
  UNUSED(mode);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t CMW_SENSORNAME_SetTestPattern(void *io_ctx, int32_t mode)
{
  UNUSED(io_ctx);
  UNUSED(mode);

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
}

static int32_t CMW_SENSORNAME_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor)
{
  CMW_SENSORNAME_config_t *sensor_config = (CMW_SENSORNAME_config_t *)initSensor->sensor_config;

  if ((io_ctx == NULL) || (initSensor == NULL) || (sensor_config == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  return SENSORNAME_VENDOR_Init((CMW_SENSORNAME_t *)io_ctx, initSensor, sensor_config);
}

void CMW_SENSORNAME_SetDefaultSensorValues(void *sensor_config)
{
  CMW_SENSORNAME_config_t *SENSORNAME_config;

  assert(sensor_config != NULL);
  SENSORNAME_config = (CMW_SENSORNAME_config_t *)sensor_config;
  SENSORNAME_config->pixel_format = SENSORNAME_NATIVE_FORMAT;
}

static int32_t CMW_SENSORNAME_Start(void *io_ctx)
{
  return SENSORNAME_VENDOR_Start((CMW_SENSORNAME_t *)io_ctx);
}

static int32_t CMW_SENSORNAME_Run(void *io_ctx)
{
  UNUSED(io_ctx);

  return CMW_ERROR_NONE;
}

static void CMW_SENSORNAME_VsyncEventCallback(void *io_ctx, uint32_t pipe)
{
  UNUSED(io_ctx);
  UNUSED(pipe);
}

static void CMW_SENSORNAME_FrameEventCallback(void *io_ctx, uint32_t pipe)
{
  UNUSED(io_ctx);
  UNUSED(pipe);
}

static void CMW_SENSORNAME_PowerOn(CMW_SENSORNAME_t *io_ctx)
{
  /*@TODO: Implement power-on sequence for SENSORNAME */
  io_ctx->EnablePin(1);
  io_ctx->ShutdownPin(0);
  io_ctx->Delay(1);
  io_ctx->ShutdownPin(1);
  io_ctx->Delay(1);
}

static int32_t CMW_SENSORNAME_Probe(CMW_SENSORNAME_t *io_ctx, CMW_Sensor_if_t *camera_if)
{
  int32_t ret;

  if ((io_ctx == NULL) || (camera_if == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  CMW_SENSORNAME_PowerOn(io_ctx);

  ret = SENSORNAME_VENDOR_RegisterBusIO(io_ctx);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = SENSORNAME_VENDOR_ReadID(io_ctx, NULL);
  if ((ret != CMW_ERROR_NONE) && (ret != CMW_ERROR_FEATURE_NOT_SUPPORTED))
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  memset(camera_if, 0, sizeof(*camera_if));
  camera_if->Start = CMW_SENSORNAME_Start;
  camera_if->DeInit = CMW_SENSORNAME_DeInit;
  camera_if->Run = CMW_SENSORNAME_Run;
  camera_if->VsyncEventCallback = CMW_SENSORNAME_VsyncEventCallback;
  camera_if->FrameEventCallback = CMW_SENSORNAME_FrameEventCallback;
  camera_if->SetGain = CMW_SENSORNAME_SetGain;
  camera_if->SetExposure = CMW_SENSORNAME_SetExposure;
  camera_if->SetExposureMode = CMW_SENSORNAME_SetExposureMode;
  camera_if->SetMirrorFlip = CMW_SENSORNAME_SetMirrorFlip;
  camera_if->SetTestPattern = CMW_SENSORNAME_SetTestPattern;
  camera_if->GetSensorInfo = CMW_SENSORNAME_GetSensorInfo;
  camera_if->GetIspDecimationRatio = CMW_SENSORNAME_GetIspDecimationRatio;

  return CMW_ERROR_NONE;
}

static void CMW_SENSORNAME_ShutdownPin(int value)
{
  HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void CMW_SENSORNAME_EnablePin(int value)
{
  HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int32_t CMW_CAMERA_SENSORNAME_Init(CMW_Sensor_if_t *camera_drv,
                                  void *sensor_ctx,
                                  DCMIPP_HandleTypeDef *hdcmipp,
                                  CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP)
{
  int32_t ret;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };
  CMW_SENSORNAME_t *SENSORNAME_ctx = (CMW_SENSORNAME_t *)sensor_ctx;
  CMW_SENSORNAME_config_t default_sensor_config;
  CMW_SENSORNAME_config_t *sensor_config;
  uint32_t dt_format = 0;
  uint32_t dt = 0;

  if ((camera_drv == NULL) || (SENSORNAME_ctx == NULL) || (hdcmipp == NULL) || (initSensors_params == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  memset(SENSORNAME_ctx, 0, sizeof(*SENSORNAME_ctx));
  SENSORNAME_ctx->Address     = CAMERA_SENSORNAME_ADDRESS;
  SENSORNAME_ctx->ClockInHz   = CAMERA_SENSORNAME_FREQ_IN_HZ;
  SENSORNAME_ctx->Init        = CMW_I2C_INIT;
  SENSORNAME_ctx->DeInit      = CMW_I2C_DEINIT;
  SENSORNAME_ctx->ReadReg     = CMW_I2C_READREG16;
  SENSORNAME_ctx->WriteReg    = CMW_I2C_WRITEREG16;
  SENSORNAME_ctx->GetTick     = BSP_GetTick;
  SENSORNAME_ctx->Delay       = HAL_Delay;
  SENSORNAME_ctx->ShutdownPin = CMW_SENSORNAME_ShutdownPin;
  SENSORNAME_ctx->EnablePin   = CMW_SENSORNAME_EnablePin;
  SENSORNAME_ctx->hdcmipp     = hdcmipp;

  ret = CMW_SENSORNAME_Probe(SENSORNAME_ctx, camera_drv);
  if (ret != CMW_ERROR_NONE)
  {
    return ret;
  }

  if ((initSensors_params->width == 0U) || (initSensors_params->height == 0U))
  {
    CMW_Sensor_Info_t sensor_info;
    ret = camera_drv->GetSensorInfo(SENSORNAME_ctx, &sensor_info);
    if (ret != CMW_ERROR_NONE)
    {
      return ret;
    }
    initSensors_params->width = sensor_info.width;
    initSensors_params->height = sensor_info.height;
  }

  CMW_SENSORNAME_SetDefaultSensorValues(&default_sensor_config);
  initSensors_params->sensor_config = initSensors_params->sensor_config ? initSensors_params->sensor_config : &default_sensor_config;

  ret = CMW_SENSORNAME_Init(SENSORNAME_ctx, initSensors_params);
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
  csi_conf.PHYBitrate = SENSORNAME_CSI_PHY_BITRATE;
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
  for (uint32_t i = DCMIPP_PIPE0; i <= DCMIPP_PIPE2; i++)
  {
    ret = HAL_DCMIPP_CSI_PIPE_SetConfig(hdcmipp, i, &csi_pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }

  return CMW_ERROR_NONE;
}
