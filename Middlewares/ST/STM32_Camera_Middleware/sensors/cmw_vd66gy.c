/**
  ******************************************************************************
  * @file    cmw_vd66gy.c
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

#include "cmw_vd66gy.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "cmw_camera.h"
#include "cmw_utils.h"
#include "cmw_io.h"
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_param_conf.h"
#endif

#include "vd6g.h"

#define VD66GY_REG_MODEL_ID                   0x0000

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))

#ifndef MIN
#define MIN(a, b)                           ((a) < (b) ?  (a) : (b))
#endif
#define MDECIBEL_TO_LINEAR(mdB)             (pow(10.0, (mdB / 1000.0) / 20.0))
#define LINEAR_TO_MDECIBEL(linearValue)     (1000 * (20.0 * log10(linearValue)))
#define FLOAT_TO_FP58(x)                    (((uint16_t)(x) << 8) | ((uint16_t)((x - (uint16_t)(x)) * 256.0f) & 0xFF))
#define FP58_TO_FLOAT(fp)                   (((fp) >> 8) + ((fp) & 0xFF) / 256.0f)

static int CMW_VD66GY_Read8(CMW_VD66GY_t *pObj, uint16_t addr, uint8_t *value)
{
  return pObj->ReadReg(pObj->Address, addr, value, 1);
}

static int CMW_VD66GY_Read16(CMW_VD66GY_t *pObj, uint16_t addr, uint16_t *value)
{
  uint8_t data[2];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 2);
  if (ret)
    return ret;

  *value = (data[1] << 8) | data[0];

  return CMW_ERROR_NONE;
}

static int CMW_VD66GY_Read32(CMW_VD66GY_t *pObj, uint16_t addr, uint32_t *value)
{
  uint8_t data[4];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 4);
  if (ret)
    return ret;

  *value = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];

  return 0;
}

static int CMW_VD66GY_Write8(CMW_VD66GY_t *pObj, uint16_t addr, uint8_t value)
{
  return pObj->WriteReg(pObj->Address, addr, &value, 1);
}

static int CMW_VD66GY_Write16(CMW_VD66GY_t *pObj, uint16_t addr, uint16_t value)
{
  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value, 2);
}

static int CMW_VD66GY_Write32(CMW_VD66GY_t *pObj, uint16_t addr, uint32_t value)
{
  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value, 4);
}

static void VD6G_ShutdownPin(struct VD6G_Ctx *ctx, int value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  p_ctx->ShutdownPin(value);
}

static int VD6G_Read8(struct VD6G_Ctx *ctx, uint16_t addr, uint8_t *value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Read8(p_ctx, addr, value);
}

static int VD6G_Read16(struct VD6G_Ctx *ctx, uint16_t addr, uint16_t *value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Read16(p_ctx, addr, value);
}

static int VD6G_Read32(struct VD6G_Ctx *ctx, uint16_t addr, uint32_t *value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Read32(p_ctx, addr, value);
}

static int VD6G_Write8(struct VD6G_Ctx *ctx, uint16_t addr, uint8_t value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Write8(p_ctx, addr, value);
}

static int VD6G_Write16(struct VD6G_Ctx *ctx, uint16_t addr, uint16_t value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Write16(p_ctx, addr, value);
}

static int VD6G_Write32(struct VD6G_Ctx *ctx, uint16_t addr, uint32_t value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Write32(p_ctx, addr, value);
}

static int VD6G_WriteArray(struct VD6G_Ctx *ctx, uint16_t addr, uint8_t *data, int data_len)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);
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

static void VD6G_Delay(struct VD6G_Ctx *ctx, uint32_t delay_in_ms)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  p_ctx->Delay(delay_in_ms);
}

static void VD6G_Log(struct VD6G_Ctx *ctx, int lvl, const char *format, va_list ap)
{
#if 0
  const int current_lvl = VD6G_LVL_DBG(0);

  if (lvl > current_lvl)
    return ;

  vprintf(format, ap);
#endif
}

static int CMW_VD66GY_GetResType(uint32_t width, uint32_t height, VD6G_Res_t *res)
{
  if (width == 320 && height == 240)
  {
    *res = VD6G_RES_QVGA_320_240;
  }
  else if (width == 640 && height == 480)
  {
    *res = VD6G_RES_VGA_640_480;
  }
  else if (width == 1024 && height == 768)
  {
    *res = VD6G_RES_XGA_1024_768;
  }
  else if (width == 1120 && height == 720)
  {
    *res = VD6G_RES_PORTRAIT_1120_720;
  }
  else if (width == 1120 && height == 1364)
  {
    *res = VD6G_RES_FULL_1120_1364;
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  return 0;
}

static VD6G_MirrorFlip_t CMW_VD66GY_getMirrorFlipConfig(uint32_t Config)
{
  VD6G_MirrorFlip_t ret;

  switch (Config)
  {
    case CMW_MIRRORFLIP_NONE:
      ret = VD6G_MIRROR_FLIP_NONE;
      break;
    case CMW_MIRRORFLIP_FLIP:
      ret = VD6G_FLIP;
      break;
    case CMW_MIRRORFLIP_MIRROR:
      ret = VD6G_MIRROR;
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
    default:
      ret = VD6G_MIRROR_FLIP;
      break;
  }

  return ret;
}

static int32_t CMW_VD66GY_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor, void *p_appliHelpers_ISP)
{
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  VD6G_Config_t config = { 0 };
  int ret;
  int i;
  CMW_VD66GY_config_t *sensor_config;
  sensor_config = (CMW_VD66GY_config_t*)(initSensor->sensor_config);
  assert(sensor_config != NULL);

  if (vd66gy_ctx->IsInitialized)
  {
    return CMW_ERROR_NONE;
  }

  config.frame_rate = initSensor->fps;
  ret = CMW_VD66GY_GetResType(initSensor->width, initSensor->height, &config.resolution);
  if (ret)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  switch (sensor_config->pixel_format)
  {
    case CMW_PIXEL_FORMAT_DEFAULT:
    case CMW_PIXEL_FORMAT_RAW10:
    {
      config.pixel_depth = 10;
      break;
    }
    case CMW_PIXEL_FORMAT_RAW8:
    {
      config.pixel_depth = 8;
      break;
    }
    default:
      return CMW_ERROR_COMPONENT_FAILURE;
      break;
  }

  config.ext_clock_freq_in_hz = CAMERA_VD66GY_FREQ_IN_HZ; /* Default clock frequency */
  config.line_len = sensor_config->line_len;
  config.out_itf.datalane_nb = 2;
  config.out_itf.clock_lane_swap_enable = 1;
  config.out_itf.data_lane0_swap_enable = 1;
  config.out_itf.data_lane1_swap_enable = 1;
  config.out_itf.data_lanes_mapping_swap_enable = 0;

  config.flip_mirror_mode = CMW_VD66GY_getMirrorFlipConfig(initSensor->mirrorFlip);
  config.patgen = VD6G_PATGEN_DISABLE;
  config.flicker = VD6G_FLICKER_FREE_NONE;

  for (i = 0; i < VD6G_GPIO_NB; i++)
  {
    config.gpio_ctrl[i] = VD6G_GPIO_GPIO_IN;
  }

  ret = VD6G_Init(&vd66gy_ctx->ctx_driver, &config);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  if (vd66gy_ctx->ctx_driver.bayer == VD6G_BAYER_NONE)
  {
    VD6G_DeInit(&vd66gy_ctx->ctx_driver);
    return CMW_ERROR_PERIPH_FAILURE;
  }

  vd66gy_ctx->IsInitialized = 1;

#if !defined (CMW_USE_WITHOUT_ISP)
  /* Statistic area is provided with null value so that it force the ISP Library to get the statistic
    * area information from the tuning file.
    */
  (void) ISP_IQParamCacheInit; /* unused */
  ret = ISP_Init(&vd66gy_ctx->hIsp, vd66gy_ctx->hdcmipp, 0, (ISP_AppliHelpersTypeDef *)p_appliHelpers_ISP, &ISP_IQParamCacheInit_VD66GY);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = ISP_SetAEConvergenceSpeed(&vd66gy_ctx->hIsp, ISP_AE_CONVERGENCESPEED_MEDIUM);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = ISP_SetAWBConvergenceSpeed(&vd66gy_ctx->hIsp, ISP_AWB_CONVERGENCESPEED_MEDIUM);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_WRONG_PARAM;
  }
#endif

  return CMW_ERROR_NONE;
}

void CMW_VD66GY_SetDefaultSensorValues(void *sensor_config)
{
  assert(sensor_config != NULL);
  CMW_VD66GY_config_t *vd66gy_config = (CMW_VD66GY_config_t *)sensor_config;
  vd66gy_config->line_len = 0;
  vd66gy_config->pixel_format = CMW_PIXEL_FORMAT_RAW10;
}

static int32_t CMW_VD66GY_Start(void *io_ctx)
{
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

#if !defined (CMW_USE_WITHOUT_ISP)
  ret = ISP_Start(&vd66gy_ctx->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif


  ret = VD6G_Start(&vd66gy_ctx->ctx_driver);
  if (ret) {
    VD6G_DeInit(&vd66gy_ctx->ctx_driver);
    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
}

static int32_t CMW_VD66GY_Run(void *io_ctx)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  int ret;
  ret = ISP_BackgroundProcess(&vd66gy_ctx->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif

  return CMW_ERROR_NONE;
}

static int32_t CMW_VD66GY_Stop(void *io_ctx)
{
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  ret = VD6G_Stop(&vd66gy_ctx->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
}

static int32_t CMW_VD66GY_DeInit(void *io_ctx)
{
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

#if !defined (CMW_USE_WITHOUT_ISP)
  ret = ISP_DeInit(&vd66gy_ctx->hIsp);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
#endif

  ret = VD6G_DeInit(&vd66gy_ctx->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  vd66gy_ctx->IsInitialized = 0;

  return CMW_ERROR_NONE;
}

/**
  * @brief  Set the gain
  * @param  pObj  pointer to component object
  * @param  Gain Gain in mdB
  * @retval Component status
  */
static int32_t CMW_VD66GY_SetGain(void *io_ctx, int32_t gain)
{
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  int32_t ret;
  uint8_t again_regmin, again_regmax;
  uint16_t dgain_regmin, dgain_regmax;
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;
  double analog_linear_gain, digital_linear_gain;

  ret = VD6G_GetAnalogGainRegRange(&vd66gy_ctx->ctx_driver, &again_regmin, &again_regmax);
  if (ret)
    return ret;

  ret = VD6G_GetDigitalGainRegRange(&vd66gy_ctx->ctx_driver, &dgain_regmin, &dgain_regmax);
  if (ret)
    return ret;

  again_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(32 / (32 - again_regmin));
  again_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(32 / (32 - again_regmax));
  dgain_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(dgain_regmin));
  dgain_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(dgain_regmax));

  if ((gain < dgain_min_mdB + again_min_mdB)
      || (gain > dgain_max_mdB + again_max_mdB))
    return -1;

  if (gain <= again_max_mdB)
  {
    /* Use analog gain only and set digital gain to its minimum */
    analog_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - dgain_min_mdB));
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)dgain_min_mdB);
  }
  else
  {
    /* For higher gain values, add digital gain */
    analog_linear_gain = MDECIBEL_TO_LINEAR((double)again_max_mdB);
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - again_max_mdB));
  }

  ret = VD6G_SetAnalogGain(&vd66gy_ctx->ctx_driver, (int) (32 - (32 / analog_linear_gain)));
  if (ret)
    return ret;

  ret = VD6G_SetDigitalGain(&vd66gy_ctx->ctx_driver, FLOAT_TO_FP58(digital_linear_gain));
  if (ret)
    return ret;

  return 0;
}

/**
  * @brief  Set the exposure
  * @param  pObj  pointer to component object
  * @param  Exposure Exposure in micro seconds
  * @retval Component status
  */
static int32_t CMW_VD66GY_SetExposure(void *io_ctx, int32_t exposure)
{
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;

  return VD6G_SetExposureTime(&vd66gy_ctx->ctx_driver, exposure);
}

/**
  * @brief  Set the exposure mode
  * @param  pObj  pointer to component object
  * @param  Exposure Exposure mode
  * @retval Component status
  */
static int32_t CMW_VD66GY_SetExposureMode(void *io_ctx, int32_t mode)
{
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  int ret = -1;

  switch (mode)
  {
    case CMW_EXPOSUREMODE_MANUAL:
      ret = VD6G_SetExposureMode(&vd66gy_ctx->ctx_driver, VD6G_EXPOSURE_MANUAL);
      break;
    case CMW_EXPOSUREMODE_AUTOFREEZE:
      ret = VD6G_SetExposureMode(&vd66gy_ctx->ctx_driver, VD6G_EXPOSURE_FREEZE_AEALGO);
      break;
    case CMW_EXPOSUREMODE_AUTO:
    default:
      ret = VD6G_SetExposureMode(&vd66gy_ctx->ctx_driver, VD6G_EXPOSURE_AUTO);
      break;
  }

  return (ret == 0) ? CMW_ERROR_NONE : CMW_ERROR_UNKNOWN_FAILURE;
}

/**
  * @brief  Set the sensor white balance mode
  * @param  io_ctx  pointer to component object
  * @param  Automatic automatic mode enable/disable
  * @param  RefColorTemp color temperature if automatic mode is disabled
  * @retval Component status
  */
static int32_t CMW_VD66GY_SetWBRefMode(void *io_ctx, uint8_t Automatic, uint32_t RefColorTemp)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  ret = ISP_SetWBRefMode(&vd66gy_ctx->hIsp, Automatic, RefColorTemp);
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
static int32_t CMW_VD66GY_ListWBRefModes(void *io_ctx, uint32_t RefColorTemp[], uint32_t array_size)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  assert(array_size >= CMW_CAMERA_NB_WB_REF_MODES);

  ret = ISP_ListWBRefModes(&vd66gy_ctx->hIsp, RefColorTemp);
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
  * @brief  Get the sensor info
  * @param  pObj  pointer to component object
  * @param  pInfo pointer to sensor info structure
  * @retval Component status
  */
static int32_t CMW_VD66GY_GetSensorInfo(void *io_ctx, CMW_Sensor_Info_t *info)
{
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  uint8_t again_regmin, again_regmax;
  uint16_t dgain_regmin, dgain_regmax;
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;

  int ret;

  if ((!io_ctx) || (info == NULL))
    return CMW_ERROR_WRONG_PARAM;

  /* Get sensor name */
  if (sizeof(info->name) >= strlen(VD66GY_NAME) + 1)
  {
    strcpy(info->name, VD66GY_NAME);
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  /* Get bayer pattern info */
  switch (vd66gy_ctx->ctx_driver.bayer)
  {
    case VD6G_BAYER_RGGB:
      info->bayer_pattern = CMW_BAYER_PATTERN_RGGB;
      break;
    case VD6G_BAYER_GRBG:
      info->bayer_pattern = CMW_BAYER_PATTERN_GRBG;
      break;
    case VD6G_BAYER_GBRG:
      info->bayer_pattern = CMW_BAYER_PATTERN_GBRG;
      break;
    case VD6G_BAYER_BGGR:
      info->bayer_pattern = CMW_BAYER_PATTERN_BGGR;
      break;
    default:
      return CMW_ERROR_WRONG_PARAM;
  }

  /* Color depth derives from the current driver configuration */
  info->color_depth = vd66gy_ctx->ctx_driver.ctx.config_save.pixel_depth;

  /* Get resolution info */
  info->width = VD6G_MAX_WIDTH;
  info->height = VD6G_MAX_HEIGHT;

  /* Get gain range */
  ret = VD6G_GetAnalogGainRegRange(&vd66gy_ctx->ctx_driver, &again_regmin, &again_regmax);
  if (ret)
    return ret;

  ret = VD6G_GetDigitalGainRegRange(&vd66gy_ctx->ctx_driver, &dgain_regmin, &dgain_regmax);
  if (ret)
    return ret;

  again_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(32 / (32 - again_regmin));
  again_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(32 / (32 - again_regmax));
  dgain_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(dgain_regmin));
  dgain_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(dgain_regmax));

  info->gain_min = again_min_mdB + dgain_min_mdB;
  info->gain_max = again_max_mdB + dgain_max_mdB;
  info->again_max = again_max_mdB;

  /* Get exposure range */
  ret = VD6G_GetExposureRegRange(&vd66gy_ctx->ctx_driver, &info->exposure_min, &info->exposure_max);
  if (ret)
    return ret;

  return CMW_ERROR_NONE;
}

static int32_t CMW_VD66GY_GetIspDecimationRatio(void *io_ctx, int32_t *ratio_h, int32_t *ratio_v)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_VD66GY_t *ctx = (CMW_VD66GY_t *) io_ctx;

  return CMW_UTILS_GetIspDecimationRatio_WithIsp(&ctx->hIsp, ratio_h, ratio_v);
#else
  return CMW_UTILS_GetIspDecimationRatio_NoIsp(ratio_h, ratio_v);
#endif
}

static void CMW_VD66GY_VsyncEventCallback(void *io_ctx, uint32_t pipe)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  /* Update the ISP frame counter and call its statistics handler */
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)io_ctx;
  switch (pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&vd66gy_ctx->hIsp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&vd66gy_ctx->hIsp);
      ISP_GatherStatistics(&vd66gy_ctx->hIsp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&vd66gy_ctx->hIsp);
      break;
  }
#endif
}

static void CMW_VD66GY_FrameEventCallback(void *io_ctx, uint32_t pipe)
{
}

static int32_t VD66GY_RegisterBusIO(CMW_VD66GY_t *io_ctx)
{
  int ret;

  if (!io_ctx)
    return CMW_ERROR_COMPONENT_FAILURE;

  if (!io_ctx->Init)
    return CMW_ERROR_COMPONENT_FAILURE;

  ret = io_ctx->Init();

  return ret;
}

static int32_t VD66GY_ReadID(CMW_VD66GY_t *io_ctx, uint32_t *Id)
{
  uint16_t reg16;
  int32_t ret;

  ret = CMW_VD66GY_Read16(io_ctx, VD66GY_REG_MODEL_ID, &reg16);
  if (ret)
    return ret;

  *Id = reg16;

  return CMW_ERROR_NONE;
}

static void CMW_VD66GY_PowerOn(CMW_VD66GY_t *io_ctx)
{
  /* Camera sensor Power-On sequence */
  /* Assert the camera  NRST pins */
  io_ctx->EnablePin(1);
  io_ctx->ShutdownPin(0);  /* Disable MB1723 2V8 signal  */
  HAL_Delay(200);   /* NRST signals asserted during 200ms */
  /* De-assert the camera STANDBY pin (active high) */
  io_ctx->ShutdownPin(1);  /* Disable MB1723 2V8 signal  */
  HAL_Delay(20);     /* NRST de-asserted during 20ms */
}

static int CMW_VD66GY_Probe(CMW_VD66GY_t *io_ctx, CMW_Sensor_if_t *vd6g_if)
{
  int ret = CMW_ERROR_NONE;
  uint32_t id;

  io_ctx->ctx_driver.shutdown_pin = VD6G_ShutdownPin;
  io_ctx->ctx_driver.read8 = VD6G_Read8;
  io_ctx->ctx_driver.read16 = VD6G_Read16;
  io_ctx->ctx_driver.read32 = VD6G_Read32;
  io_ctx->ctx_driver.write8 = VD6G_Write8;
  io_ctx->ctx_driver.write16 = VD6G_Write16;
  io_ctx->ctx_driver.write32 = VD6G_Write32;
  io_ctx->ctx_driver.write_array = VD6G_WriteArray;
  io_ctx->ctx_driver.delay = VD6G_Delay;
  io_ctx->ctx_driver.log = VD6G_Log;

  CMW_VD66GY_PowerOn(io_ctx);

  ret = VD66GY_RegisterBusIO(io_ctx);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = VD66GY_ReadID(io_ctx, &id);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  if (id != VD66GY_CHIP_ID)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  memset(vd6g_if, 0, sizeof(*vd6g_if));
  vd6g_if->DeInit = CMW_VD66GY_DeInit;
  vd6g_if->Run = CMW_VD66GY_Run;
  vd6g_if->VsyncEventCallback = CMW_VD66GY_VsyncEventCallback;
  vd6g_if->FrameEventCallback = CMW_VD66GY_FrameEventCallback;
  vd6g_if->Start = CMW_VD66GY_Start;
  vd6g_if->Stop = CMW_VD66GY_Stop;
  vd6g_if->SetGain = CMW_VD66GY_SetGain;
  vd6g_if->SetExposure = CMW_VD66GY_SetExposure;
  vd6g_if->SetExposureMode = CMW_VD66GY_SetExposureMode;
  vd6g_if->SetWBRefMode = CMW_VD66GY_SetWBRefMode;
  vd6g_if->ListWBRefModes = CMW_VD66GY_ListWBRefModes;
  vd6g_if->GetSensorInfo = CMW_VD66GY_GetSensorInfo;
  vd6g_if->GetIspDecimationRatio = CMW_VD66GY_GetIspDecimationRatio;

  return ret;
}

static void CMW_VD66GY_ShutdownPin(int value)
{
  HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void CMW_VD66GY_EnablePin(int value)
{
  HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int32_t CMW_CAMERA_VD66GY_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx,  DCMIPP_HandleTypeDef *hdcmipp,
                              CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP)
{
  int32_t ret = CMW_ERROR_NONE;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };
  uint32_t dt_format = 0;
  uint32_t dt = 0;
  CMW_VD66GY_config_t default_sensor_config;
  CMW_VD66GY_config_t *sensor_config;
  CMW_VD66GY_t *vd66gy_ctx = (CMW_VD66GY_t *)sensor_ctx;

  memset(vd66gy_ctx, 0, sizeof(*vd66gy_ctx));
  vd66gy_ctx->Address     = CAMERA_VD66GY_ADDRESS;
  vd66gy_ctx->Init        = CMW_I2C_INIT;
  vd66gy_ctx->DeInit      = CMW_I2C_DEINIT;
  vd66gy_ctx->ReadReg     = CMW_I2C_READREG16;
  vd66gy_ctx->WriteReg    = CMW_I2C_WRITEREG16;
  vd66gy_ctx->Delay       = HAL_Delay;
  vd66gy_ctx->ShutdownPin = CMW_VD66GY_ShutdownPin;
  vd66gy_ctx->EnablePin   = CMW_VD66GY_EnablePin;
  vd66gy_ctx->hdcmipp     = hdcmipp;

  ret = CMW_VD66GY_Probe(vd66gy_ctx, camera_drv);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Special case: when resolution is not specified take the full sensor resolution */
  if ((initSensors_params->width == 0) || (initSensors_params->height == 0))
  {
    initSensors_params->width = VD6G_MAX_WIDTH;
    initSensors_params->height = VD6G_MAX_HEIGHT;
  }

  CMW_VD66GY_SetDefaultSensorValues(&default_sensor_config);
  initSensors_params->sensor_config = initSensors_params->sensor_config ? initSensors_params->sensor_config : &default_sensor_config;
  sensor_config = (CMW_VD66GY_config_t*) (initSensors_params->sensor_config);

  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_800;
  ret = HAL_DCMIPP_CSI_SetConfig(hdcmipp, &csi_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

 switch (sensor_config->pixel_format)
  {
    case CMW_PIXEL_FORMAT_RAW8:
    {
      dt_format = DCMIPP_CSI_DT_BPP8;
      dt = DCMIPP_DT_RAW8;
      break;
    }
    case CMW_PIXEL_FORMAT_RAW10:
    case CMW_PIXEL_FORMAT_DEFAULT:
    {
      dt_format = DCMIPP_CSI_DT_BPP10;
      dt = DCMIPP_DT_RAW10;
      break;
    }
    default:
      return CMW_ERROR_COMPONENT_FAILURE;
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

  ret = CMW_VD66GY_Init(vd66gy_ctx, initSensors_params, p_appliHelpers_ISP);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return CMW_ERROR_NONE;
}
