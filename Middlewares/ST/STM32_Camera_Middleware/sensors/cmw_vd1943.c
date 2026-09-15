/**
  ******************************************************************************
  * @file    cmw_vd1943.c
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

#include "cmw_vd1943.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "cmw_camera.h"
#include "cmw_io.h"
#include "cmw_utils.h"
#include "vd1943.h"
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_param_conf.h"
#endif

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))

#ifndef MIN
#define MIN(a, b)                           ((a) < (b) ?  (a) : (b))
#endif

#define VD1943_REG_MODEL_ID                           0x0000

#define MDECIBEL_TO_LINEAR(mdB) (pow(10.0, (mdB / 1000.0) / 20.0))
#define LINEAR_TO_MDECIBEL(linearValue) (1000 * (20.0 * log10(linearValue)))
#define FLOAT_TO_FP58(x) (((uint16_t)(x) << 8) | ((uint16_t)((x - (uint16_t)(x)) * 256.0f) & 0xFF))
#define FP58_TO_FLOAT(fp) (((fp) >> 8) + ((fp) & 0xFF) / 256.0f)

static int CMW_VD1943_GetRegMode(cmw_vd1943_stream_mode_config_t *sensor_stream_mode, VD1943_MODE_t *mode_reg_value, CMW_PixelFormat_t *pixel_format)
{
  if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
      sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
      sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
      sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
      sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
      sensor_stream_mode->upscaling == false &&
      sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS1_NATIVE_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS1_NATIVE_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == true)
  {
    *mode_reg_value = VD1943_GS_SS1_SPLIT_NATIVE_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == true)
  {
    *mode_reg_value = VD1943_GS_SS1_SPLIT_NATIVE_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS1_RGB_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS1_RGB_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_2x2 &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS2_MONO_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_2x2 &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS2_MONO_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_4x4 &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS4_MONO_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_4x4 &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS4_MONO_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_32x32 &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS32_MONO_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_32x32 &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS32_MONO_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == true &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS1_IR_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == true &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_GS_SS1_IR_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == true &&
           sensor_stream_mode->split_exposure == true)
  {
    *mode_reg_value = VD1943_GS_SS1_SPLIT_IR_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_GLOBAL_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_IR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == true &&
           sensor_stream_mode->split_exposure == true)
  {
    *mode_reg_value = VD1943_GS_SS1_SPLIT_IR_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_SDR_NATIVE_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_SDR_NATIVE_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_12 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_SDR_NATIVE_12;
    *pixel_format = CMW_PIXEL_FORMAT_RAW12;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_8 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_SDR_RGB_8;
    *pixel_format = CMW_PIXEL_FORMAT_RAW8;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_SDR_RGB_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_12 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_SDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_SDR_RGB_12;
    *pixel_format = CMW_PIXEL_FORMAT_RAW12;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_HDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_HDR_NATIVE_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_12 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_RGBNIR &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_HDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_HDR_NATIVE_12;
    *pixel_format = CMW_PIXEL_FORMAT_RAW12;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_10 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_HDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_HDR_RGB_10;
    *pixel_format = CMW_PIXEL_FORMAT_RAW10;
    return CMW_ERROR_NONE;
  }
  else if (sensor_stream_mode->shutter_type == CMW_VD1943_ROLLING_SHUTTER &&
           sensor_stream_mode->pixel_depth == CMW_VD1943_SENSOR_PIXEL_DEPTH_12 &&
           sensor_stream_mode->pixel_pattern == CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG &&
           sensor_stream_mode->subsampling == CMW_VD1943_SENSOR_SUBSAMPLING_OFF &&
           sensor_stream_mode->dynamic_range == CMW_VD1943_DYNAMIC_RANGE_HDR &&
           sensor_stream_mode->upscaling == false &&
           sensor_stream_mode->split_exposure == false)
  {
    *mode_reg_value = VD1943_RS_HDR_RGB_12;
    *pixel_format = CMW_PIXEL_FORMAT_RAW12;
    return CMW_ERROR_NONE;
  }
  return CMW_ERROR_WRONG_PARAM;
}

static VD1943_MirrorFlip_t CMW_VD1943_getMirrorFlipConfig(uint32_t Config)
{
  switch (Config) {
    case CMW_MIRRORFLIP_NONE:
      return VD1943_MIRROR_FLIP_NONE;
      break;
    case CMW_MIRRORFLIP_FLIP:
      return VD1943_FLIP;
      break;
    case CMW_MIRRORFLIP_MIRROR:
      return VD1943_MIRROR;
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
      return VD1943_MIRROR_FLIP;
      break;
    default:
      /* Add assert here (but still keep return value in case of NDEBUG) once #68 is implemented */
      return VD1943_MIRROR_FLIP_NONE;
  }
}


static int CMW_VD1943_Read8(CMW_VD1943_t *pObj, uint16_t addr, uint8_t *value)
{
  return pObj->ReadReg(pObj->Address, addr, value, 1);
}

static int CMW_VD1943_Read16(CMW_VD1943_t *pObj, uint16_t addr, uint16_t *value)
{
  uint8_t data[2];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 2);
  if (ret)
    return ret;

  *value = (data[1] << 8) | data[0];

  return CMW_ERROR_NONE;
}

static int CMW_VD1943_Read32(CMW_VD1943_t *pObj, uint16_t addr, uint32_t *value)
{
  uint8_t data[4];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 4);
  if (ret)
    return ret;

  *value = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];

  return 0;
}

static int CMW_VD1943_Write8(CMW_VD1943_t *pObj, uint16_t addr, uint8_t value)
{
  return pObj->WriteReg(pObj->Address, addr, &value, 1);
}

static int CMW_VD1943_Write16(CMW_VD1943_t *pObj, uint16_t addr, uint16_t value)
{
  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value, 2);
}

static int CMW_VD1943_Write32(CMW_VD1943_t *pObj, uint16_t addr, uint32_t value)
{
  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value, 4);
}

static void VD1943_ShutdownPin(struct VD1943_Ctx *ctx, int value)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);

  p_ctx->ShutdownPin(value);
}

static int VD1943_Read8(struct VD1943_Ctx *ctx, uint16_t addr, uint8_t *value)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);

  return CMW_VD1943_Read8(p_ctx, addr, value);
}

static int VD1943_Read16(struct VD1943_Ctx *ctx, uint16_t addr, uint16_t *value)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);

  return CMW_VD1943_Read16(p_ctx, addr, value);
}

static int VD1943_Read32(struct VD1943_Ctx *ctx, uint16_t addr, uint32_t *value)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);

  return CMW_VD1943_Read32(p_ctx, addr, value);
}

static int VD1943_Write8(struct VD1943_Ctx *ctx, uint16_t addr, uint8_t value)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);

  return CMW_VD1943_Write8(p_ctx, addr, value);
}

static int VD1943_Write16(struct VD1943_Ctx *ctx, uint16_t addr, uint16_t value)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);

  return CMW_VD1943_Write16(p_ctx, addr, value);
}

static int VD1943_Write32(struct VD1943_Ctx *ctx, uint16_t addr, uint32_t value)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);

  return CMW_VD1943_Write32(p_ctx, addr, value);
}

static int VD1943_WriteArray(struct VD1943_Ctx *ctx, uint16_t addr, uint8_t *data, int data_len)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);
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

static void VD1943_Delay(struct VD1943_Ctx *ctx, uint32_t delay_in_ms)
{
  CMW_VD1943_t *p_ctx = container_of(ctx, CMW_VD1943_t, ctx_driver);

  p_ctx->Delay(delay_in_ms);
}

static void VD1943_Log(struct VD1943_Ctx *ctx, int lvl, const char *format, va_list ap)
{
#if 0
  const int current_lvl = VD1943_LVL_DBG(2);

  if (lvl > current_lvl)
    return ;

  vprintf(format, ap);
#endif
}

/**
 * @brief  Set the gain
 * @param  pObj  pointer to component object
 * @param  Gain Gain in mdB
 * @retval Component status
 */
static int32_t CMW_VD1943_SetGain(void *io_ctx, int32_t gain)
{
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  int32_t ret;
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;
  double analog_linear_gain, digital_linear_gain;
  double again_reg_f;
  unsigned int again_reg;

  again_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(16.0 / (16.0 - (double)VD1943_ANALOG_GAIN_MIN));
  again_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(16.0 / (16.0 - (double)VD1943_ANALOG_GAIN_MAX));
  dgain_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(VD1943_DIGITAL_GAIN_MIN));
  dgain_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(VD1943_DIGITAL_GAIN_MAX));

  if ((gain < dgain_min_mdB + again_min_mdB) || (gain > dgain_max_mdB + again_max_mdB))
    return -1;

  if (gain <= again_max_mdB)
  {
    /* Use analog gain only and set digital gain to its minimum */
    analog_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - dgain_min_mdB));
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)dgain_min_mdB);
    /* Take care to rounding issue */
    again_reg_f = 16.0 - (16.0 / analog_linear_gain);
    again_reg = (unsigned int)(again_reg_f + 0.5);
    if (again_reg > VD1943_ANALOG_GAIN_MAX)
      again_reg = VD1943_ANALOG_GAIN_MAX;
  }
  else
  {
    /* Analog saturated, remainder goes to digital */
    again_reg = VD1943_ANALOG_GAIN_MAX;
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - again_max_mdB));
  }

  ret = VD1943_SetAnalogGain(&vd1943_ctx->ctx_driver, again_reg);
  if (ret)
    return ret;

  ret = VD1943_SetDigitalGain(&vd1943_ctx->ctx_driver,
                              FLOAT_TO_FP58(digital_linear_gain));
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
static int32_t CMW_VD1943_SetExposure(void *io_ctx, int32_t exposure)
{
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;

  return VD1943_SetExpo(&vd1943_ctx->ctx_driver, exposure);
}

/**
  * @brief  Set the sensor white balance mode
  * @param  io_ctx  pointer to component object
  * @param  Automatic automatic mode enable/disable
  * @param  RefColorTemp color temperature if automatic mode is disabled
  * @retval Component status
  */
static int32_t CMW_VD1943_SetWBRefMode(void *io_ctx, uint8_t Automatic, uint32_t RefColorTemp)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  ret = ISP_SetWBRefMode(&vd1943_ctx->hIsp, Automatic, RefColorTemp);
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
static int32_t CMW_VD1943_ListWBRefModes(void *io_ctx, uint32_t RefColorTemp[], uint32_t array_size)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  assert(array_size >= CMW_CAMERA_NB_WB_REF_MODES);

  ret = ISP_ListWBRefModes(&vd1943_ctx->hIsp, RefColorTemp);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
#else

  return CMW_ERROR_FEATURE_NOT_SUPPORTED;
#endif
}

static int32_t CMW_VD1943_GetIspDecimationRatio(void *io_ctx, int32_t *ratio_h, int32_t *ratio_v)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_VD1943_t *ctx = (CMW_VD1943_t *) io_ctx;

  return CMW_UTILS_GetIspDecimationRatio_WithIsp(&ctx->hIsp, ratio_h, ratio_v);
#else
  return CMW_UTILS_GetIspDecimationRatio_NoIsp(ratio_h, ratio_v);
#endif
}

/**
  * @brief  Get the sensor info
  * @param  pObj  pointer to component object
  * @param  pInfo pointer to sensor info structure
  * @retval Component status
  */
static int32_t CMW_VD1943_GetSensorInfo(void *io_ctx, CMW_Sensor_Info_t *info)
{
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;
  int ret;

  if ((!io_ctx) || (info == NULL))
    return CMW_ERROR_WRONG_PARAM;

  /* Get sensor name */
  if (sizeof(info->name) >= strlen(VD1943_NAME) + 1)
  {
    strcpy(info->name, VD1943_NAME);
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  /* Get bayer pattern info */
  switch (vd1943_ctx->ctx_driver.bayer)
  {
    case VD1943_BAYER_RGGB:
      info->bayer_pattern = CMW_BAYER_PATTERN_RGGB;
      break;
    case VD1943_BAYER_GRBG:
      info->bayer_pattern = CMW_BAYER_PATTERN_GRBG;
      break;
    case VD1943_BAYER_GBRG:
      info->bayer_pattern = CMW_BAYER_PATTERN_GBRG;
      break;
    case VD1943_BAYER_BGGR:
      info->bayer_pattern = CMW_BAYER_PATTERN_BGGR;
      break;
    case VD1943_BAYER_RGBNIR:
      info->bayer_pattern = CMW_BAYER_PATTERN_RGBNIR;
      break;
    case VD1943_BAYER_RGBNIR_MIRROR:
      info->bayer_pattern = CMW_BAYER_PATTERN_RGBNIR_MIRROR;
      break;
    case VD1943_BAYER_RGBNIR_FLIP:
      info->bayer_pattern = CMW_BAYER_PATTERN_RGBNIR_FLIP;
      break;
    case VD1943_BAYER_RGBNIR_FLIP_MIRROR:
      info->bayer_pattern = CMW_BAYER_PATTERN_RGBNIR_FLIP_MIRROR;
      break;
    default:
      return CMW_ERROR_WRONG_PARAM;
  }

  /* Get color depth */
  ret = VD1943_GetPixelDepth(&vd1943_ctx->ctx_driver,
                             (unsigned int *)&info->color_depth);
  if (ret)
    return ret;

  /* Return the default full resolution */
  info->width = VD1943_MAX_WIDTH;
  info->height = VD1943_MAX_HEIGHT;

  /* Get gain range */
  again_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(16.0 / (16.0 - (double)VD1943_ANALOG_GAIN_MIN));
  again_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(16.0 / (16.0 - (double)VD1943_ANALOG_GAIN_MAX));
  dgain_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(VD1943_DIGITAL_GAIN_MIN));
  dgain_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(VD1943_DIGITAL_GAIN_MAX));

  info->gain_min = again_min_mdB + dgain_min_mdB;
  info->gain_max = again_max_mdB + dgain_max_mdB;
  info->again_max = again_max_mdB;

  /* Get exposure range */
  ret = VD1943_GetExposureRange(&vd1943_ctx->ctx_driver,
                                (unsigned int *)&info->exposure_min,
                                (unsigned int *)&info->exposure_max);
  if (ret)
    return ret;

  return CMW_ERROR_NONE;
}

static int CMW_VD1943_GetResType(uint32_t width, uint32_t height, VD1943_Res_t *res)
{
  if (width == 320 && height == 240)
  {
    *res = VD1943_RES_QVGA_320_240;
  }
  else if (width == 640 && height == 480)
  {
    *res = VD1943_RES_VGA_640_480;
  }
  else if (width == 800 && height == 600)
  {
    *res = VD1943_RES_SVGA_800_600;
  }
  else if (width == 1280 && height == 720)
  {
    *res = VD1943_RES_720P_1280_720;
  }
  else if (width == 1920 && height == 1080)
  {
    *res = VD1943_RES_1080P_1920_1080;
  }
  else if (width == 2560 && height == 1984)
  {
    *res = VD1943_RES_FULL_2560_1984;
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  return 0;
}

static int32_t CMW_VD1943_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor, void *p_appliHelpers_ISP)
{
  CMW_VD1943_config_t *sensor_config = (CMW_VD1943_config_t*)(initSensor->sensor_config);
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  CMW_PixelFormat_t pixel_format;
  VD1943_Config_t config = { 0 };
  VD1943_MODE_t mode;
  int ret;
  int i;

  assert(sensor_config != NULL);
  if (vd1943_ctx->IsInitialized)
  {
    return CMW_ERROR_NONE;
  }

  config.frame_rate = initSensor->fps;
  ret = CMW_VD1943_GetResType(initSensor->width, initSensor->height, &config.resolution);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = CMW_VD1943_GetRegMode(&sensor_config->sensor_stream_mode, &mode, &pixel_format);
  (void)pixel_format;
  if (ret != CMW_ERROR_NONE)
  {
    return ret;
  }

  config.image_processing_mode = mode;
  config.ext_clock_freq_in_hz = 25000000;
  config.flip_mirror_mode = CMW_VD1943_getMirrorFlipConfig(initSensor->mirrorFlip);
  config.patgen = VD1943_PATGEN_DISABLE;
  /* setup csi2 itf */
  config.out_itf.data_rate_in_mps = sensor_config->CSI_PHYBitrate;
  config.out_itf.datalane_nb = 2;
  config.out_itf.logic_lane_mapping[0] = 0;
  config.out_itf.logic_lane_mapping[1] = 1;
  config.out_itf.logic_lane_mapping[2] = 2;
  config.out_itf.logic_lane_mapping[3] = 3;
  config.out_itf.clock_lane_swap_enable = 1;
  config.out_itf.physical_lane_swap_enable[0] = 1;
  config.out_itf.physical_lane_swap_enable[1] = 1;
  /* gpios as input */
  for (i = 0; i < VD1943_GPIO_NB; i++)
  {
    config.gpios[i].gpio_ctrl = VD1943_GPIO_IN;
    config.gpios[i].enable = 0;
  }
  /* Default VT mode*/
  config.sync_mode = VD1943_MASTER;

  ret = VD1943_Init(&vd1943_ctx->ctx_driver, &config);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  if (vd1943_ctx->ctx_driver.bayer == VD1943_BAYER_NONE)
  {
    VD1943_DeInit(&vd1943_ctx->ctx_driver);
    return CMW_ERROR_PERIPH_FAILURE;
  }

  vd1943_ctx->IsInitialized = 1;

#if !defined (CMW_USE_WITHOUT_ISP)
  /* Statistic area is provided with null value so that it force the ISP Library to get the statistic
    * area information from the tuning file.
    */
  (void) ISP_IQParamCacheInit; /* unused */
  ret = ISP_Init(&vd1943_ctx->hIsp, vd1943_ctx->hdcmipp, 0, (ISP_AppliHelpersTypeDef *)p_appliHelpers_ISP, &ISP_IQParamCacheInit_VD1943);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = ISP_SetAEConvergenceSpeed(&vd1943_ctx->hIsp, ISP_AE_CONVERGENCESPEED_MEDIUM);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = ISP_SetAWBConvergenceSpeed(&vd1943_ctx->hIsp, ISP_AWB_CONVERGENCESPEED_MEDIUM);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_WRONG_PARAM;
  }
#endif

  return CMW_ERROR_NONE;
}

void CMW_VD1943_SetDefaultSensorValues(void *sensor_config)
{
  assert(sensor_config != NULL);
  CMW_VD1943_config_t *vd1943_config = (CMW_VD1943_config_t *)sensor_config;
  vd1943_config->CSI_PHYBitrate = VD1943_DEFAULT_DATARATE;
  vd1943_config->sensor_stream_mode.shutter_type = CMW_VD1943_ROLLING_SHUTTER;
  vd1943_config->sensor_stream_mode.pixel_depth = CMW_VD1943_SENSOR_PIXEL_DEPTH_12;
  vd1943_config->sensor_stream_mode.pixel_pattern = CMW_VD1943_SENSOR_PIXEL_PATTERN_GBRG;
  vd1943_config->sensor_stream_mode.subsampling = CMW_VD1943_SENSOR_SUBSAMPLING_OFF;
  vd1943_config->sensor_stream_mode.dynamic_range = CMW_VD1943_DYNAMIC_RANGE_SDR;
  vd1943_config->sensor_stream_mode.upscaling = false;
  vd1943_config->sensor_stream_mode.split_exposure = false;
}

static int32_t CMW_VD1943_Start(void *io_ctx)
{
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

#if !defined (CMW_USE_WITHOUT_ISP)
  ret = ISP_Start(&vd1943_ctx->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif


  ret = VD1943_Start(&vd1943_ctx->ctx_driver);
  if (ret) {
    VD1943_DeInit(&vd1943_ctx->ctx_driver);
    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
}

static int32_t CMW_VD1943_Run(void *io_ctx)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  int ret;
  ret = ISP_BackgroundProcess(&vd1943_ctx->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif

  return CMW_ERROR_NONE;
}

static int32_t CMW_VD1943_Stop(void *io_ctx)
{
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

  ret = VD1943_Stop(&vd1943_ctx->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  return CMW_ERROR_NONE;
}

static int32_t CMW_VD1943_DeInit(void *io_ctx)
{
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  int ret = CMW_ERROR_NONE;

#if !defined (CMW_USE_WITHOUT_ISP)
  ret = ISP_DeInit(&vd1943_ctx->hIsp);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
#endif

  ret = VD1943_DeInit(&vd1943_ctx->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  vd1943_ctx->IsInitialized = 0;

  return CMW_ERROR_NONE;
}

static void CMW_VD1943_VsyncEventCallback(void *io_ctx, uint32_t pipe)
{
#if !defined (CMW_USE_WITHOUT_ISP)
  /* Update the ISP frame counter and call its statistics handler */
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)io_ctx;
  switch (pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&vd1943_ctx->hIsp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&vd1943_ctx->hIsp);
      ISP_GatherStatistics(&vd1943_ctx->hIsp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&vd1943_ctx->hIsp);
      break;
  }
#endif
}

static void CMW_VD1943_FrameEventCallback(void *io_ctx, uint32_t pipe)
{
}

static int32_t VD1943_RegisterBusIO(CMW_VD1943_t *io_ctx)
{
  int ret;

  if (!io_ctx)
    return CMW_ERROR_COMPONENT_FAILURE;

  if (!io_ctx->Init)
    return CMW_ERROR_COMPONENT_FAILURE;

  ret = io_ctx->Init();

  return ret;
}

static int32_t VD1943_ReadID(CMW_VD1943_t *io_ctx, uint32_t *Id)
{
  uint32_t reg32;
  int32_t ret;

  ret = CMW_VD1943_Read32(io_ctx, VD1943_REG_MODEL_ID, &reg32);
  if (ret)
    return ret;

  *Id = reg32;

  return CMW_ERROR_NONE;
}

static void CMW_VD1943_PowerOn(CMW_VD1943_t *io_ctx)
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

static int CMW_VD1943_Probe(CMW_VD1943_t *io_ctx, CMW_Sensor_if_t *vd1943_if)
{
  int ret = CMW_ERROR_NONE;
  uint32_t id;

  io_ctx->ctx_driver.shutdown_pin = VD1943_ShutdownPin;
  io_ctx->ctx_driver.read8 = VD1943_Read8;
  io_ctx->ctx_driver.read16 = VD1943_Read16;
  io_ctx->ctx_driver.read32 = VD1943_Read32;
  io_ctx->ctx_driver.write8 = VD1943_Write8;
  io_ctx->ctx_driver.write16 = VD1943_Write16;
  io_ctx->ctx_driver.write32 = VD1943_Write32;
  io_ctx->ctx_driver.write_array = VD1943_WriteArray;
  io_ctx->ctx_driver.delay = VD1943_Delay;
  io_ctx->ctx_driver.log = VD1943_Log;

  CMW_VD1943_PowerOn(io_ctx);

  ret = VD1943_RegisterBusIO(io_ctx);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = VD1943_ReadID(io_ctx, &id);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  if (id != VD1943_CUT1_3_CHIP_ID && id != VD1943_CUT1_4_CHIP_ID)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  memset(vd1943_if, 0, sizeof(*vd1943_if));
  vd1943_if->DeInit = CMW_VD1943_DeInit;
  vd1943_if->Run = CMW_VD1943_Run;
  vd1943_if->VsyncEventCallback = CMW_VD1943_VsyncEventCallback;
  vd1943_if->FrameEventCallback = CMW_VD1943_FrameEventCallback;
  vd1943_if->Start = CMW_VD1943_Start;
  vd1943_if->Stop = CMW_VD1943_Stop;
  vd1943_if->GetSensorInfo = CMW_VD1943_GetSensorInfo;
  vd1943_if->SetGain = CMW_VD1943_SetGain;
  vd1943_if->SetExposure = CMW_VD1943_SetExposure;
  vd1943_if->SetWBRefMode = CMW_VD1943_SetWBRefMode;
  vd1943_if->ListWBRefModes = CMW_VD1943_ListWBRefModes;
  vd1943_if->GetIspDecimationRatio = CMW_VD1943_GetIspDecimationRatio;

  return ret;
}

static void CMW_VD1943_ShutdownPin(int value)
{
  HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void CMW_VD1943_EnablePin(int value)
{
  HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int32_t CMW_CAMERA_VD1943_Init(CMW_Sensor_if_t *camera_drv, void *sensor_ctx,  DCMIPP_HandleTypeDef *hdcmipp,
                              CMW_Sensor_Init_t *initSensors_params, void *p_appliHelpers_ISP)
{
  int32_t ret = CMW_ERROR_NONE;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };
  uint32_t dt_format = 0;
  uint32_t dt = 0;
  VD1943_MODE_t mode;
  CMW_PixelFormat_t pixel_format;
  CMW_VD1943_config_t default_sensor_config;
  int32_t csi_phybitrate_i = -1;
  CMW_VD1943_config_t *sensor_config;
  CMW_VD1943_t *vd1943_ctx = (CMW_VD1943_t *)sensor_ctx;

  memset(vd1943_ctx, 0, sizeof(*vd1943_ctx));
  vd1943_ctx->Address     = CAMERA_VD1943_ADDRESS;
  vd1943_ctx->Init        = CMW_I2C_INIT;
  vd1943_ctx->DeInit      = CMW_I2C_DEINIT;
  vd1943_ctx->WriteReg    = CMW_I2C_WRITEREG16;
  vd1943_ctx->ReadReg     = CMW_I2C_READREG16;
  vd1943_ctx->Delay       = HAL_Delay;
  vd1943_ctx->ShutdownPin = CMW_VD1943_ShutdownPin;
  vd1943_ctx->EnablePin   = CMW_VD1943_EnablePin;
  vd1943_ctx->hdcmipp     = hdcmipp;

  ret = CMW_VD1943_Probe(vd1943_ctx, camera_drv);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Special case: when resolution is not specified take the full sensor resolution */
  if ((initSensors_params->width == 0) || (initSensors_params->height == 0))
  {
    initSensors_params->width = VD1943_MAX_WIDTH;
    initSensors_params->height = VD1943_MAX_HEIGHT;
  }

  CMW_VD1943_SetDefaultSensorValues(&default_sensor_config);
  initSensors_params->sensor_config = initSensors_params->sensor_config ? initSensors_params->sensor_config : &default_sensor_config;
  sensor_config = (CMW_VD1943_config_t*) (initSensors_params->sensor_config);

  csi_phybitrate_i = CMW_UTILS_getClosest_HAL_PHYBitrate(sensor_config->CSI_PHYBitrate);
  if (csi_phybitrate_i < 0)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = csi_phybitrate_i;
  ret = HAL_DCMIPP_CSI_SetConfig(hdcmipp, &csi_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = CMW_VD1943_GetRegMode(&sensor_config->sensor_stream_mode, &mode, &pixel_format);
  (void)mode;
  if (ret != CMW_ERROR_NONE)
  {
    return ret;
  }

  switch (pixel_format)
  {
    case CMW_PIXEL_FORMAT_RAW8:
    {
      dt_format = DCMIPP_CSI_DT_BPP8;
      dt = DCMIPP_DT_RAW8;
      break;
    }
    case CMW_PIXEL_FORMAT_RAW10:
    {
      dt_format = DCMIPP_CSI_DT_BPP10;
      dt = DCMIPP_DT_RAW10;
      break;
    }
    case CMW_PIXEL_FORMAT_RAW12:
    case CMW_PIXEL_FORMAT_DEFAULT:
    {
      dt_format = DCMIPP_CSI_DT_BPP12;
      dt = DCMIPP_DT_RAW12;
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

  ret = CMW_VD1943_Init(vd1943_ctx, initSensors_params, p_appliHelpers_ISP);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return CMW_ERROR_NONE;
}
