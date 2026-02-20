/**
  ******************************************************************************
  * @file    cmw_vd55g1.c
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

#include "cmw_vd55g1.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "vd55g1.h"
#include "cmw_camera.h"
#include "cmw_io.h"

#define VD55G1_CHIP_ID 0x53354731

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))

#ifndef MIN
#define MIN(a, b)                           ((a) < (b) ?  (a) : (b))
#endif

#define MDECIBEL_TO_LINEAR(mdB)             (pow(10.0, ((double)(mdB) / 1000.0) / 20.0))
#define LINEAR_TO_MDECIBEL(linearValue)     (1000.0 * (20.0 * log10(linearValue)))

#define FLOAT_TO_FP58(x)                    ((((uint16_t)(x)) << 8) | ((uint16_t)(((x) - (uint16_t)(x)) * 256.0f) & 0xFFU))
#define FP58_TO_FLOAT(fp)                   (((fp) >> 8) + (((fp) & 0xFFU) / 256.0))

#define VD55G1_REG_MODEL_ID                           0x0000

static int CMW_VD55G1_Read8(CMW_VD55G1_t *pObj, uint16_t addr, uint8_t *value)
{
  return pObj->ReadReg(pObj->Address, addr, value, 1);
}

static int CMW_VD55G1_Read16(CMW_VD55G1_t *pObj, uint16_t addr, uint16_t *value)
{
  uint8_t data[2];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 2);
  if (ret)
    return ret;

  *value = (data[1] << 8) | data[0];

  return CMW_ERROR_NONE;
}

static int CMW_VD55G1_Read32(CMW_VD55G1_t *pObj, uint16_t addr, uint32_t *value)
{
  uint8_t data[4];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 4);
  if (ret)
    return ret;

  *value = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];

  return 0;
}

static int CMW_VD55G1_Write8(CMW_VD55G1_t *pObj, uint16_t addr, uint8_t value)
{
  return pObj->WriteReg(pObj->Address, addr, &value, 1);
}

static int CMW_VD55G1_Write16(CMW_VD55G1_t *pObj, uint16_t addr, uint16_t value)
{
  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value, 2);
}

static int CMW_VD55G1_Write32(CMW_VD55G1_t *pObj, uint16_t addr, uint32_t value)
{
  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value, 4);
}

static void VD55G1_ShutdownPin(struct VD55G1_Ctx *ctx, int value)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);

  p_ctx->ShutdownPin(value);
}

static int VD55G1_Read8(struct VD55G1_Ctx *ctx, uint16_t addr, uint8_t *value)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);

  return CMW_VD55G1_Read8(p_ctx, addr, value);
}

static int VD55G1_Read16(struct VD55G1_Ctx *ctx, uint16_t addr, uint16_t *value)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);

  return CMW_VD55G1_Read16(p_ctx, addr, value);
}

static int VD55G1_Read32(struct VD55G1_Ctx *ctx, uint16_t addr, uint32_t *value)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);

  return CMW_VD55G1_Read32(p_ctx, addr, value);
}

static int VD55G1_Write8(struct VD55G1_Ctx *ctx, uint16_t addr, uint8_t value)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);

  return CMW_VD55G1_Write8(p_ctx, addr, value);
}

static int VD55G1_Write16(struct VD55G1_Ctx *ctx, uint16_t addr, uint16_t value)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);

  return CMW_VD55G1_Write16(p_ctx, addr, value);
}

static int VD55G1_Write32(struct VD55G1_Ctx *ctx, uint16_t addr, uint32_t value)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);

  return CMW_VD55G1_Write32(p_ctx, addr, value);
}

static int VD55G1_WriteArray(struct VD55G1_Ctx *ctx, uint16_t addr, uint8_t *data, int data_len)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);
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

static void VD55G1_Delay(struct VD55G1_Ctx *ctx, uint32_t delay_in_ms)
{
  CMW_VD55G1_t *p_ctx = container_of(ctx, CMW_VD55G1_t, ctx_driver);

  p_ctx->Delay(delay_in_ms);
}

static void VD55G1_Log(struct VD55G1_Ctx *ctx, int lvl, const char *format, va_list ap)
{
#if 0
  const int current_lvl = VD55G1_LVL_DBG(0);

  if (lvl > current_lvl)
    return ;

  vprintf(format, ap);
#endif
}

/**
  * @brief  Get the sensor info
  * @param  pObj  pointer to component object
  * @param  pInfo pointer to sensor info structure
  * @retval Component status
  */
static int32_t CMW_VD55G1_GetSensorInfo(void *io_ctx, ISP_SensorInfoTypeDef *info)
{
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;
  uint32_t exposure_min, exposure_max;
  int ret;

  if ((io_ctx == NULL) || (info == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  if (sizeof(info->name) >= (strlen(VD55G1_NAME) + 1U))
  {
    strcpy(info->name, VD55G1_NAME);
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  info->bayer_pattern = ISP_DEMOS_TYPE_MONO;
  info->color_depth = ((CMW_VD55G1_t *)io_ctx)->ctx_driver.ctx.config_save.pixel_depth;
  info->width = VD55G1_MAX_WIDTH;
  info->height = VD55G1_MAX_HEIGHT;

  /* Add 0.5 before casting so we round to the closest mdB value instead of truncating. */
  again_min_mdB = (uint32_t)(LINEAR_TO_MDECIBEL(32.0 / (32.0 - (double)VD55G1_ANALOG_GAIN_MIN)) + 0.5);
  again_max_mdB = (uint32_t)(LINEAR_TO_MDECIBEL(32.0 / (32.0 - (double)VD55G1_ANALOG_GAIN_MAX)) + 0.5);
  dgain_min_mdB = (uint32_t)(LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(VD55G1_DIGITAL_GAIN_MIN)) + 0.5);
  dgain_max_mdB = (uint32_t)(LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(VD55G1_DIGITAL_GAIN_MAX)) + 0.5);

  info->gain_min = again_min_mdB + dgain_min_mdB;
  info->gain_max = again_max_mdB + dgain_max_mdB;
  info->again_max = again_max_mdB;

  ret = VD55G1_GetExposureRegRange(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, &exposure_min, &exposure_max);
  if (ret)
    return ret;

  info->exposure_min = exposure_min;
  info->exposure_max = exposure_max;

  return CMW_ERROR_NONE;
}

static int CMW_VD55G1_GetResType(uint32_t width, uint32_t height, VD55G1_Res_t *res)
{
  if (width == 320 && height == 240)
  {
    *res = VD55G1_RES_QVGA_320_240;
  }
  else if (width == 640 && height == 480)
  {
    *res = VD55G1_RES_VGA_640_480;
  }
  else if (width == 800 && height == 600)
  {
    *res = VD55G1_RES_SXGA_800_600;
  }
  else if (width == 804 && height == 704)
  {
      *res = VD55G1_RES_FULL_804_704;
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }
  return 0;
}

static VD55G1_MirrorFlip_t CMW_VD55G1_getMirrorFlipConfig(int32_t Config)
{
  VD55G1_MirrorFlip_t ret;

  switch (Config)
  {
    case CMW_MIRRORFLIP_NONE:
      ret = VD55G1_MIRROR_FLIP_NONE;
      break;
    case CMW_MIRRORFLIP_FLIP:
      ret = VD55G1_FLIP;
      break;
    case CMW_MIRRORFLIP_MIRROR:
      ret = VD55G1_MIRROR;
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
    default:
      ret = VD55G1_MIRROR_FLIP;
      break;
  }

  return ret;
}

static int32_t CMW_VD55G1_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor)
{
  VD55G1_Config_t config = { 0 };
  int ret;
  int i;
  CMW_VD55G1_config_t *sensor_config;
  sensor_config = (CMW_VD55G1_config_t*)(initSensor->sensor_config);
  if (sensor_config == NULL)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  assert(initSensor != NULL);

  if (((CMW_VD55G1_t *)io_ctx)->IsInitialized)
  {
    return CMW_ERROR_NONE;
  }

  config.frame_rate = initSensor->fps;
  ret = CMW_VD55G1_GetResType(initSensor->width, initSensor->height, &config.resolution);
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

  config.ext_clock_freq_in_hz = CAMERA_VD55G1_FREQ_IN_HZ;
  config.out_itf.data_rate_in_mps = sensor_config->CSI_PHYBitrate;
  config.out_itf.clock_lane_swap_enable = 1;
  config.out_itf.data_lane_swap_enable = 1;

  config.flip_mirror_mode = CMW_VD55G1_getMirrorFlipConfig(initSensor->mirrorFlip);
  config.patgen = VD55G1_PATGEN_DISABLE;
  config.flicker = VD55G1_FLICKER_FREE_NONE;
  config.awu.is_enable = 0;

  for (i = 0; i < VD55G1_GPIO_NB; i++)
  {
    config.gpio_ctrl[i] = VD55G1_GPIO_GPIO_IN;
  }

  ret = VD55G1_Init(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, &config);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ((CMW_VD55G1_t *)io_ctx)->IsInitialized = 1;
  return CMW_ERROR_NONE;
}

void CMW_VD55G1_SetDefaultSensorValues( CMW_VD55G1_config_t *vd55g1_config)
{
  assert(vd55g1_config != NULL);
  vd55g1_config->pixel_format = CMW_PIXEL_FORMAT_RAW10;
  vd55g1_config->CSI_PHYBitrate = VD55G1_DEFAULT_DATARATE;
}

static int32_t CMW_VD55G1_Start(void *io_ctx)
{
  int ret = CMW_ERROR_NONE;
  ret = VD55G1_Start(&((CMW_VD55G1_t *)io_ctx)->ctx_driver);
  if (ret) {
    VD55G1_DeInit(&((CMW_VD55G1_t *)io_ctx)->ctx_driver);
    return CMW_ERROR_PERIPH_FAILURE;
  }
  return CMW_ERROR_NONE;
}

static int32_t CMW_VD55G1_Stop(void *io_ctx)
{
  int ret = CMW_ERROR_NONE;

  ret = VD55G1_Stop(&((CMW_VD55G1_t *)io_ctx)->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }
  return CMW_ERROR_NONE;
}

static int32_t CMW_VD55G1_DeInit(void *io_ctx)
{
  int ret = CMW_ERROR_NONE;

  ret = VD55G1_Stop(&((CMW_VD55G1_t *)io_ctx)->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = VD55G1_DeInit(&((CMW_VD55G1_t *)io_ctx)->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ((CMW_VD55G1_t *)io_ctx)->IsInitialized = 0;
  return CMW_ERROR_NONE;
}

static int32_t CMW_VD55G1_MirrorFlipConfig(void *io_ctx, uint32_t Config)
{
  int32_t ret = CMW_ERROR_NONE;

  switch (Config) {
    case CMW_MIRRORFLIP_NONE:
      ret = VD55G1_SetFlipMirrorMode(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, VD55G1_MIRROR_FLIP_NONE);
      break;
    case CMW_MIRRORFLIP_FLIP:
      ret = VD55G1_SetFlipMirrorMode(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, VD55G1_FLIP);
      break;
    case CMW_MIRRORFLIP_MIRROR:
      ret = VD55G1_SetFlipMirrorMode(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, VD55G1_MIRROR);
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
      ret = VD55G1_SetFlipMirrorMode(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, VD55G1_MIRROR_FLIP);
      break;
    default:
      ret = CMW_ERROR_PERIPH_FAILURE;
  }

  return ret;
}

int32_t CMW_VD55G1_SetGain(void *io_ctx, int32_t gain)
{
  CMW_VD55G1_t *ctx = (CMW_VD55G1_t *)io_ctx;
  uint32_t again_min_mdB = (uint32_t)(LINEAR_TO_MDECIBEL(32.0 / (32.0 - (double)VD55G1_ANALOG_GAIN_MIN)) + 0.5);
  uint32_t again_max_mdB = (uint32_t)(LINEAR_TO_MDECIBEL(32.0 / (32.0 - (double)VD55G1_ANALOG_GAIN_MAX)) + 0.5);
  uint32_t dgain_min_mdB = (uint32_t)(LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(VD55G1_DIGITAL_GAIN_MIN)) + 0.5);
  uint32_t dgain_max_mdB = (uint32_t)(LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(VD55G1_DIGITAL_GAIN_MAX)) + 0.5);
  double analog_linear_gain;
  double digital_linear_gain;
  int ret;
  int analog_reg;
  uint16_t digital_reg;

  if ((gain < (int32_t)(dgain_min_mdB + again_min_mdB)) ||
      (gain > (int32_t)(dgain_max_mdB + again_max_mdB)))
    return -1;

  if (gain <= (int32_t)again_max_mdB)
  {
    analog_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - (int32_t)dgain_min_mdB));
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)dgain_min_mdB);
  }
  else
  {
    analog_linear_gain = MDECIBEL_TO_LINEAR((double)again_max_mdB);
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - (int32_t)again_max_mdB));
  }

  if (analog_linear_gain < 1.0)
    analog_linear_gain = 1.0;

  analog_reg = (int)(32.0 - (32.0 / analog_linear_gain) + 0.5);
  if (analog_reg < VD55G1_ANALOG_GAIN_MIN)
    analog_reg = VD55G1_ANALOG_GAIN_MIN;
  if (analog_reg > VD55G1_ANALOG_GAIN_MAX)
    analog_reg = VD55G1_ANALOG_GAIN_MAX;

  ret = VD55G1_SetAnalogGain(&ctx->ctx_driver, analog_reg);
  if (ret)
    return ret;

  digital_reg = FLOAT_TO_FP58(digital_linear_gain);
  if (digital_reg < VD55G1_DIGITAL_GAIN_MIN)
    digital_reg = VD55G1_DIGITAL_GAIN_MIN;
  if (digital_reg > VD55G1_DIGITAL_GAIN_MAX)
    digital_reg = VD55G1_DIGITAL_GAIN_MAX;
  ret = VD55G1_SetDigitalGain(&ctx->ctx_driver, (int)digital_reg);
  if (ret)
    return ret;

  return 0;
}

int32_t CMW_VD55G1_SetExposure(void *io_ctx, int32_t exposure)
{
  return VD55G1_SetExposureTime(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, exposure);
}

int32_t CMW_VD55G1_SetExposureMode(void *io_ctx, int32_t mode)
{
  int ret = -1;

  switch (mode)
  {
    case CMW_EXPOSUREMODE_MANUAL:
      ret = VD55G1_SetExposureMode(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, VD55G1_EXPOSURE_MODE_MANUAL);
      break;
    case CMW_EXPOSUREMODE_AUTOFREEZE:
      ret = VD55G1_SetExposureMode(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, VD55G1_EXPOSURE_MODE_FREEZE);
      break;
    case CMW_EXPOSUREMODE_AUTO:
    default:
      ret = VD55G1_SetExposureMode(&((CMW_VD55G1_t *)io_ctx)->ctx_driver, VD55G1_EXPOSURE_MODE_AUTO);
      break;
  }

  return (ret == 0) ? CMW_ERROR_NONE : CMW_ERROR_UNKNOWN_FAILURE;
}

static int32_t VD55G1_RegisterBusIO(CMW_VD55G1_t *io_ctx)
{
  int ret;

  if (!io_ctx)
    return CMW_ERROR_COMPONENT_FAILURE;

  if (!io_ctx->Init)
    return CMW_ERROR_COMPONENT_FAILURE;

  ret = io_ctx->Init();

  return ret;
}

static int32_t VD55G1_ReadID(CMW_VD55G1_t *io_ctx, uint32_t *Id)
{
  uint32_t reg32;
  int32_t ret;

  ret = CMW_VD55G1_Read32(io_ctx, VD55G1_REG_MODEL_ID, &reg32);
  if (ret)
    return ret;

  *Id = reg32;

  return CMW_ERROR_NONE;
}

static void CMW_VD55G1_PowerOn(CMW_VD55G1_t *io_ctx)
{
  /* Camera sensor Power-On sequence */
  /* Assert the camera  NRST pins */
  io_ctx->ShutdownPin(0);  /* Disable MB1723 2V8 signal  */
  io_ctx->Delay(200); /* NRST signals asserted during 200ms */
  /* De-assert the camera STANDBY pin (active high) */
  io_ctx->ShutdownPin(1);  /* Disable MB1723 2V8 signal  */
  io_ctx->Delay(20); /* NRST de-asserted during 20ms */
}

int CMW_VD55G1_Probe(CMW_VD55G1_t *io_ctx, CMW_Sensor_if_t *vd55g1_if)
{
  int ret = CMW_ERROR_NONE;
  uint32_t id;

  io_ctx->ctx_driver.shutdown_pin = VD55G1_ShutdownPin;
  io_ctx->ctx_driver.read8 = VD55G1_Read8;
  io_ctx->ctx_driver.read16 = VD55G1_Read16;
  io_ctx->ctx_driver.read32 = VD55G1_Read32;
  io_ctx->ctx_driver.write8 = VD55G1_Write8;
  io_ctx->ctx_driver.write16 = VD55G1_Write16;
  io_ctx->ctx_driver.write32 = VD55G1_Write32;
  io_ctx->ctx_driver.write_array = VD55G1_WriteArray;
  io_ctx->ctx_driver.delay = VD55G1_Delay;
  io_ctx->ctx_driver.log = VD55G1_Log;

  CMW_VD55G1_PowerOn(io_ctx);

  ret = VD55G1_RegisterBusIO(io_ctx);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = VD55G1_ReadID(io_ctx, &id);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  if (id != VD55G1_CHIP_ID)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  memset(vd55g1_if, 0, sizeof(*vd55g1_if));
  vd55g1_if->Init = CMW_VD55G1_Init;
  vd55g1_if->DeInit = CMW_VD55G1_DeInit;
  vd55g1_if->Start = CMW_VD55G1_Start;
  vd55g1_if->Stop = CMW_VD55G1_Stop;
  vd55g1_if->SetMirrorFlip = CMW_VD55G1_MirrorFlipConfig;
  vd55g1_if->SetGain = CMW_VD55G1_SetGain;
  vd55g1_if->SetExposure = CMW_VD55G1_SetExposure;
  vd55g1_if->SetExposureMode = CMW_VD55G1_SetExposureMode;
  vd55g1_if->GetSensorInfo = CMW_VD55G1_GetSensorInfo;
  return ret;
}
