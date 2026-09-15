/**
  ******************************************************************************
  * @file    cmw_imx477.c
  * @author  MDG Application Team
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "cmw_imx477.h"
#include "cmw_camera.h"
#include "cmw_io.h"
#include "cmw_utils.h"
#include "imx477/imx477.h"
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_param_conf.h"
#endif

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))

#ifndef MIN
#define MIN(a, b)                           ((a) < (b) ?  (a) : (b))
#endif

#define MDECIBEL_TO_LINEAR(mdB) (pow(10.0, (mdB / 1000.0) / 20.0))
#define LINEAR_TO_MDECIBEL(linearValue) (1000 * (20.0 * log10(linearValue)))
#define FLOAT_TO_FP88(x) ((uint16_t) ((x) * 256))
#define FP88_TO_FLOAT(fp) ((fp) / 256.0f)
#define ACODE_TO_GAIN(r) (1024.0 / (1024 - (r)))
#define GAIN_TO_ACODE(g) ((1024 - 1024 / (g)))

static int get_res(int w, int h)
{
  if (w == 0 && h == 0)
    return IMX477_RES_4056_3040;

  if (w == 4056 && h == 3040)
    return IMX477_RES_4056_3040;

  if (w == 2028 && h == 1520)
    return IMX477_RES_2028_1520;

  return -1;
}

static int CMW_IMX477_Read8(CMW_IMX477_t *pObj, uint16_t addr, uint8_t *value)
{
  return pObj->ReadReg(pObj->Address, addr, value, 1);
}

static int CMW_IMX477_Read16(CMW_IMX477_t *pObj, uint16_t addr, uint16_t *value)
{
  uint8_t data[2];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 2);
  if (ret)
    return ret;

  *value = (data[0] << 8) | data[1];

  return CMW_ERROR_NONE;
}

static int CMW_IMX477_Read32(CMW_IMX477_t *pObj, uint16_t addr, uint32_t *value)
{
  uint8_t data[4];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 4);
  if (ret)
    return ret;

  *value = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

  return 0;
}

static int CMW_IMX477_Write8(CMW_IMX477_t *pObj, uint16_t addr, uint8_t value)
{
  return pObj->WriteReg(pObj->Address, addr, &value, 1);
}

static int CMW_IMX477_Write16(CMW_IMX477_t *pObj, uint16_t addr, uint16_t value)
{
  uint16_t value_be = ((value << 8) & 0xff00) | ((value >> 8) & 0x00ff);

  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value_be, 2);
}

static int CMW_IMX477_Write32(CMW_IMX477_t *pObj, uint16_t addr, uint32_t value)
{
  uint32_t value_be = ((value << 24) & 0xff000000) | ((value << 8) & 0x00ff0000) | ((value >> 8) & 0x0000ff00) |
                      ((value >> 24) & 0x000000ff);

  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value_be, 4);
}

static int32_t CMW_IMX477_GetSensorInfo(void *io_ctx, CMW_Sensor_Info_t *info)
{
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;
  int ret;

  assert(io_ctx);
  assert(info);

  memset(info, 0, sizeof(*info));
  /* Get gain range */
  again_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(ACODE_TO_GAIN(IMX477_ANALOG_GAIN_MIN));
  again_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(ACODE_TO_GAIN(IMX477_ANALOG_GAIN_MAX));
  dgain_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP88_TO_FLOAT(IMX477_DIGITAL_GAIN_MIN));
  dgain_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP88_TO_FLOAT(IMX477_DIGITAL_GAIN_MAX));

  /* Get sensor name */
  if (sizeof(info->name) >= strlen(IMX477_NAME) + 1)
  {
    strcpy(info->name, IMX477_NAME);
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  info->bayer_pattern = ctx->ctx_driver.bayer;
  info->color_depth = 10;
  info->width = IMX477_MAX_WIDTH;
  info->height = IMX477_MAX_HEIGHT;
  info->gain_min = again_min_mdB + dgain_min_mdB;
  info->gain_max = again_max_mdB + dgain_max_mdB;
  info->again_max = again_max_mdB;
  /* Get exposure range */
  ret = IMX477_GetExposureRange(&ctx->ctx_driver, (unsigned int *)&info->exposure_min, (unsigned int *)&info->exposure_max);
  if (ret)
    return CMW_ERROR_COMPONENT_FAILURE;

  return CMW_ERROR_NONE;
}

static int32_t CMW_IMX477_GetIspDecimationRatio(void *io_ctx, int32_t *ratio_h, int32_t *ratio_v)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;

  return CMW_UTILS_GetIspDecimationRatio_WithIsp(&ctx->hIsp, ratio_h, ratio_v);
#else
  return CMW_UTILS_GetIspDecimationRatio_NoIsp(ratio_h, ratio_v);
#endif
}

static int32_t CMW_IMX477_SetWBRefMode(void *io_ctx, uint8_t Automatic, uint32_t RefColorTemp)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;
  int ret = CMW_ERROR_NONE;

  ret = ISP_SetWBRefMode(&ctx->hIsp, Automatic, RefColorTemp);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
#else

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
#endif
}

static int32_t CMW_IMX477_ListWBRefModes(void *io_ctx, uint32_t RefColorTemp[], uint32_t array_size)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX477_t *ctx = (CMW_IMX477_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  assert(array_size >= CMW_CAMERA_NB_WB_REF_MODES);

  ret = ISP_ListWBRefModes(&ctx->hIsp, RefColorTemp);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
#else

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
#endif
}

static int32_t CMW_IMX477_DeInit(void *io_ctx)
{
  CMW_IMX477_t *ctx = (CMW_IMX477_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

#if !defined (CMW_USE_WITHOUT_ISP)
  ret = ISP_DeInit(&ctx->hIsp);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
#endif

  ret = IMX477_DeInit(&ctx->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
}

static int32_t CMW_IMX477_SetGain(void *io_ctx, int32_t gain)
{
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;
  int32_t ret;
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;
  double analog_linear_gain, digital_linear_gain;
  double again_reg_f;
  unsigned int again_reg;

  again_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(ACODE_TO_GAIN(IMX477_ANALOG_GAIN_MIN));
  again_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(ACODE_TO_GAIN(IMX477_ANALOG_GAIN_MAX));
  dgain_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP88_TO_FLOAT(IMX477_DIGITAL_GAIN_MIN));
  dgain_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP88_TO_FLOAT(IMX477_DIGITAL_GAIN_MAX));

  if ((gain < dgain_min_mdB + again_min_mdB) || (gain > dgain_max_mdB + again_max_mdB))
    return CMW_ERROR_WRONG_PARAM;

  if (gain <= again_max_mdB)
  {
    /* Use analog gain only and set digital gain to its minimum */
    analog_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - dgain_min_mdB));
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)dgain_min_mdB);
    /* Take care to rounding issue */
    again_reg_f = GAIN_TO_ACODE(analog_linear_gain);
    again_reg = (unsigned int)(again_reg_f + 0.5);
    if (again_reg > IMX477_ANALOG_GAIN_MAX)
      again_reg = IMX477_ANALOG_GAIN_MAX;
  }
  else
  {
    /* Analog saturated, remainder goes to digital */
    again_reg = IMX477_ANALOG_GAIN_MAX;
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - again_max_mdB));
  }

  ret = IMX477_SetAnalogGain(&ctx->ctx_driver, again_reg);
  if (ret)
    return CMW_ERROR_COMPONENT_FAILURE;

  ret = IMX477_SetDigitalGain(&ctx->ctx_driver, FLOAT_TO_FP88(digital_linear_gain));
  if (ret)
    return CMW_ERROR_COMPONENT_FAILURE;

  return CMW_ERROR_NONE;
}

static int32_t CMW_IMX477_SetExposure(void *io_ctx, int32_t exposure)
{
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;
  int ret;

  ret = IMX477_SetExpo(&ctx->ctx_driver, exposure);

  return ret ? CMW_ERROR_WRONG_PARAM : CMW_ERROR_NONE;
}

static int32_t CMW_IMX477_Start(void *io_ctx)
{
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;
  int ret;

#if !defined (CMW_USE_WITHOUT_ISP)
  ret = ISP_Start(&ctx->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif

  ret = IMX477_Start(&ctx->ctx_driver);
  if (ret)
  {
    IMX477_DeInit(&ctx->ctx_driver);

    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
}

static int32_t CMW_IMX477_Stop(void *io_ctx)
{
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;
  int ret;

  ret = IMX477_Stop(&ctx->ctx_driver);

  return ret ? CMW_ERROR_PERIPH_FAILURE : CMW_ERROR_NONE;
}

static int32_t CMW_IMX477_Run(void *io_ctx)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX477_t *ctx = (CMW_IMX477_t *)io_ctx;
  int ret;
  ret = ISP_BackgroundProcess(&ctx->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif

  return CMW_ERROR_NONE;
}

static void CMW_IMX477_VsyncEventCallback(void *io_ctx, uint32_t pipe)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;

  /* Update the ISP frame counter and call its statistics handler */
  switch (pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&ctx->hIsp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&ctx->hIsp);
      ISP_GatherStatistics(&ctx->hIsp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&ctx->hIsp);
      break;
  }
#endif
}

static void CMW_IMX477_FrameEventCallback(void *io_ctx, uint32_t pipe)
{
  UNUSED(io_ctx);
  UNUSED(pipe);

  /*Nothing to do */;
}

static void CMW_IMX477_ShutdownPin(int value)
{
  HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void CMW_IMX477_EnablePin(int value)
{
  HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void IMX477_ShutdownPin(struct IMX477_Ctx *ctx, int value)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);

  p_ctx->ShutdownPin(value);
}

static int IMX477_Read8(struct IMX477_Ctx *ctx, uint16_t addr, uint8_t *value)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);

  return CMW_IMX477_Read8(p_ctx, addr, value);
}

static int IMX477_Read16(struct IMX477_Ctx *ctx, uint16_t addr, uint16_t *value)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);

  return CMW_IMX477_Read16(p_ctx, addr, value);
}

static int IMX477_Read32(struct IMX477_Ctx *ctx, uint16_t addr, uint32_t *value)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);

  return CMW_IMX477_Read32(p_ctx, addr, value);
}

static int IMX477_Write8(struct IMX477_Ctx *ctx, uint16_t addr, uint8_t value)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);

  return CMW_IMX477_Write8(p_ctx, addr, value);
}

static int IMX477_Write16(struct IMX477_Ctx *ctx, uint16_t addr, uint16_t value)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);

  return CMW_IMX477_Write16(p_ctx, addr, value);
}

static int IMX477_Write32(struct IMX477_Ctx *ctx, uint16_t addr, uint32_t value)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);

  return CMW_IMX477_Write32(p_ctx, addr, value);
}

static int IMX477_WriteArray(struct IMX477_Ctx *ctx, uint16_t addr, uint8_t *data, int data_len)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);
  const unsigned int chunk_size = 128;
  uint16_t sz;
  int ret;

  while (data_len) {
    sz = MIN(data_len, chunk_size);
    ret = p_ctx->WriteReg(p_ctx->Address, addr, data, sz);
    if (ret)
      return ret;
    data_len -= sz;
    addr += sz;
    data += sz;
  }

  return 0;
}

static void IMX477_Delay(struct IMX477_Ctx *ctx, uint32_t delay_in_ms)
{
  CMW_IMX477_t *p_ctx = container_of(ctx, CMW_IMX477_t, ctx_driver);

  p_ctx->Delay(delay_in_ms);
}

static void IMX477_Log(struct IMX477_Ctx *ctx, int lvl, const char *format, va_list ap)
{
#if 0
  const int current_lvl = IMX477_LVL_DBG(0);

  if (lvl > current_lvl)
    return ;

  vprintf(format, ap);
#endif
}

static IMX477_MirrorFlip_t CMW_IMX477_getMirrorFlipConfig(uint32_t Config)
{
  IMX477_MirrorFlip_t ret;

  switch (Config)
  {
    case CMW_MIRRORFLIP_NONE:
      ret = IMX477_MIRROR_FLIP;
      break;
    case CMW_MIRRORFLIP_FLIP:
      ret = IMX477_MIRROR_FLIP_NONE;
      break;
    case CMW_MIRRORFLIP_MIRROR:
      ret = IMX477_FLIP;
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
    default:
      ret = IMX477_MIRROR;
      break;
  }

  return ret;
}

static int32_t CMW_IMX477_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor, void *p_appliHelpers_ISP)
{
  CMW_IMX477_t *ctx = (CMW_IMX477_t *) io_ctx;
  IMX477_Ctx_t *drv_ctx = &ctx->ctx_driver;
  IMX477_Config_t cfg;
  int ret;

  assert(io_ctx);
  assert(initSensor);

  /* Init io */
  ret = ctx->Init();
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  drv_ctx->shutdown_pin = IMX477_ShutdownPin;
  drv_ctx->read8 = IMX477_Read8;
  drv_ctx->read16 = IMX477_Read16;
  drv_ctx->read32 = IMX477_Read32;
  drv_ctx->write8 = IMX477_Write8;
  drv_ctx->write16 = IMX477_Write16;
  drv_ctx->write32 = IMX477_Write32;
  drv_ctx->write_array = IMX477_WriteArray;
  drv_ctx->delay = IMX477_Delay;
  drv_ctx->log = IMX477_Log;

  cfg.resolution = get_res(initSensor->width, initSensor->height);
  cfg.frame_rate = initSensor->fps;
  cfg.link_freq = IMX477_LINK_450M;
  cfg.flip_mirror_mode = CMW_IMX477_getMirrorFlipConfig(initSensor->mirrorFlip);
  cfg.patgen = IMX477_PATGEN_DISABLE;

  ret = IMX477_Init(drv_ctx, &cfg);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

#if !defined (CMW_USE_WITHOUT_ISP)
  /* Statistic area is provided with null value so that it force the ISP Library to get the statistic
   * area information from the tuning file.
   */

  (void) ISP_IQParamCacheInit;
  ret = ISP_Init(&ctx->hIsp, ctx->hdcmipp, 0, p_appliHelpers_ISP, &ISP_IQParamCacheInit_IMX477);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = ISP_SetAEConvergenceSpeed(&ctx->hIsp, ISP_AE_CONVERGENCESPEED_MEDIUM);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = ISP_SetAWBConvergenceSpeed(&ctx->hIsp, ISP_AWB_CONVERGENCESPEED_MEDIUM);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_WRONG_PARAM;
  }
#endif

  return CMW_ERROR_NONE;
}

int32_t CMW_CAMERA_IMX477_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx, DCMIPP_HandleTypeDef *hdcmipp,
                               CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP)
{
  CMW_IMX477_t *IMX477_ctx = (CMW_IMX477_t *)sensor_ctx;
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };
  CMW_IMX477_config_t default_sensor_config;
  uint32_t dt_format = DCMIPP_CSI_DT_BPP10;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  CMW_IMX477_config_t *sensor_cfg;
  uint32_t dt = DCMIPP_DT_RAW10;
  int32_t ret;

  if ((camera_drv == NULL) || (IMX477_ctx == NULL) || (hdcmipp == NULL) || (initSensors_params == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  memset(IMX477_ctx, 0, sizeof(*IMX477_ctx));
  IMX477_ctx->Address     = CAMERA_IMX477_ADDRESS;
  IMX477_ctx->Init        = CMW_I2C_INIT;
  IMX477_ctx->DeInit      = CMW_I2C_DEINIT;
  IMX477_ctx->ReadReg     = CMW_I2C_READREG16;
  IMX477_ctx->WriteReg    = CMW_I2C_WRITEREG16;
  IMX477_ctx->GetTick     = BSP_GetTick;
  IMX477_ctx->Delay       = HAL_Delay;
  IMX477_ctx->ShutdownPin = CMW_IMX477_ShutdownPin;
  IMX477_ctx->EnablePin   = CMW_IMX477_EnablePin;
  IMX477_ctx->hdcmipp     = hdcmipp;

  if ((initSensors_params->width == 0U) || (initSensors_params->height == 0U))
  {
    initSensors_params->width = IMX477_MAX_WIDTH;
    initSensors_params->height = IMX477_MAX_HEIGHT;
  }

  CMW_IMX477_SetDefaultSensorValues(&default_sensor_config);
  initSensors_params->sensor_config = initSensors_params->sensor_config ? initSensors_params->sensor_config :
                                                                          &default_sensor_config;
  sensor_cfg = (CMW_IMX477_config_t *)initSensors_params->sensor_config;

  if (sensor_cfg->pixel_format != CMW_PIXEL_FORMAT_RAW10)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_900;
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

  memset(camera_drv, 0, sizeof(*camera_drv));
  camera_drv->DeInit = CMW_IMX477_DeInit;
  camera_drv->Start = CMW_IMX477_Start;
  camera_drv->Stop = CMW_IMX477_Stop;
  camera_drv->Run = CMW_IMX477_Run;
  camera_drv->VsyncEventCallback = CMW_IMX477_VsyncEventCallback;
  camera_drv->FrameEventCallback = CMW_IMX477_FrameEventCallback;
  camera_drv->SetGain = CMW_IMX477_SetGain;
  camera_drv->SetExposure = CMW_IMX477_SetExposure;
  camera_drv->GetSensorInfo = CMW_IMX477_GetSensorInfo;
  camera_drv->GetIspDecimationRatio = CMW_IMX477_GetIspDecimationRatio;
  camera_drv->SetWBRefMode = CMW_IMX477_SetWBRefMode;
  camera_drv->ListWBRefModes = CMW_IMX477_ListWBRefModes;

  ret = CMW_IMX477_Init(IMX477_ctx, initSensors_params, p_appliHelpers_ISP);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return CMW_ERROR_NONE;
}

void CMW_IMX477_SetDefaultSensorValues(void *sensor_config)
{
  CMW_IMX477_config_t *IMX477_config = (CMW_IMX477_config_t *)sensor_config;

  assert(sensor_config != NULL);
  IMX477_config->pixel_format = CMW_PIXEL_FORMAT_RAW10;
}
