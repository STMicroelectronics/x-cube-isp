 /**
 ******************************************************************************
 * @file    cmw_utils.c
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

#include "cmw_utils.h"

#include <assert.h>
#include "stm32n6xx_hal_dcmipp.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif






static void CMW_UTILS_get_crop_config(uint32_t cam_width, uint32_t cam_height, uint32_t pipe_width,
                                      uint32_t pipe_height, DCMIPP_CropConfTypeDef *crop);
static void CMW_UTILS_get_crop_config_from_manual(CMW_Manual_roi_area_t *conf, DCMIPP_CropConfTypeDef *crop);
static void CMW_UTILS_get_down_config(float ratio_width, float ratio_height, int width, int height,
                                      DCMIPP_DownsizeTypeDef *down);
static uint32_t CMW_UTILS_get_dec_ratio_and_update(float *ratio, int is_vertical);
static void CMW_UTILS_get_scale_configs(CMW_DCMIPP_Conf_t *p_conf, float ratio_width, float ratio_height,
                                       DCMIPP_DecimationConfTypeDef *dec, DCMIPP_DownsizeTypeDef *down);

void CMW_UTILS_GetPipeConfig(uint32_t cam_width, uint32_t cam_height, CMW_DCMIPP_Conf_t *p_conf,
                             DCMIPP_CropConfTypeDef *crop, DCMIPP_DecimationConfTypeDef *dec,
                             DCMIPP_DownsizeTypeDef *down)
{
  float ratio_height = 0;
  float ratio_width = 0;

  if (p_conf->mode == CMW_Aspect_ratio_crop)
  {
    CMW_UTILS_get_crop_config(cam_width, cam_height, p_conf->output_width, p_conf->output_height, crop);
    ratio_width = (float)crop->HSize / p_conf->output_width;
    ratio_height = (float)crop->VSize / p_conf->output_height;
  }
  else if (p_conf->mode == CMW_Aspect_ratio_fit)
  {
    ratio_width = (float)cam_width / p_conf->output_width;
    ratio_height = (float)cam_height / p_conf->output_height;
  }
  else if (p_conf->mode == CMW_Aspect_ratio_fullscreen)
  {
    ratio_height = (float) cam_height / p_conf->output_height;
    ratio_width = (float) ratio_height;
  }
  else
  {
    CMW_UTILS_get_crop_config_from_manual(&p_conf->manual_conf, crop);
    ratio_width = (float)crop->HSize / p_conf->output_width;
    ratio_height = (float)crop->VSize / p_conf->output_height;
  }

  CMW_UTILS_get_scale_configs(p_conf, ratio_width, ratio_height, dec, down);
}

int32_t CMW_UTILS_getClosest_HAL_PHYBitrate(uint32_t val)
{
  static const uint32_t PHYBitrate_in_mps[][2] =
  {
    {DCMIPP_CSI_PHY_BT_80 , 80000000},
    {DCMIPP_CSI_PHY_BT_90 , 90000000},
    {DCMIPP_CSI_PHY_BT_100, 100000000},
    {DCMIPP_CSI_PHY_BT_110, 110000000},
    {DCMIPP_CSI_PHY_BT_120, 120000000},
    {DCMIPP_CSI_PHY_BT_130, 130000000},
    {DCMIPP_CSI_PHY_BT_140, 140000000},
    {DCMIPP_CSI_PHY_BT_150, 150000000},
    {DCMIPP_CSI_PHY_BT_160, 160000000},
    {DCMIPP_CSI_PHY_BT_170, 170000000},
    {DCMIPP_CSI_PHY_BT_180, 180000000},
    {DCMIPP_CSI_PHY_BT_190, 190000000},
    {DCMIPP_CSI_PHY_BT_205, 205000000},
    {DCMIPP_CSI_PHY_BT_220, 220000000},
    {DCMIPP_CSI_PHY_BT_235, 235000000},
    {DCMIPP_CSI_PHY_BT_250, 250000000},
    {DCMIPP_CSI_PHY_BT_275, 275000000},
    {DCMIPP_CSI_PHY_BT_300, 300000000},
    {DCMIPP_CSI_PHY_BT_325, 325000000},
    {DCMIPP_CSI_PHY_BT_350, 350000000},
    {DCMIPP_CSI_PHY_BT_400, 400000000},
    {DCMIPP_CSI_PHY_BT_450, 450000000},
    {DCMIPP_CSI_PHY_BT_500, 500000000},
    {DCMIPP_CSI_PHY_BT_550, 550000000},
    {DCMIPP_CSI_PHY_BT_600, 600000000},
    {DCMIPP_CSI_PHY_BT_650, 650000000},
    {DCMIPP_CSI_PHY_BT_700, 700000000},
    {DCMIPP_CSI_PHY_BT_750, 750000000},
    {DCMIPP_CSI_PHY_BT_800, 800000000},
    {DCMIPP_CSI_PHY_BT_850, 850000000},
    {DCMIPP_CSI_PHY_BT_900, 900000000},
    {DCMIPP_CSI_PHY_BT_950, 950000000},
    {DCMIPP_CSI_PHY_BT_1000, 1000000000},
    {DCMIPP_CSI_PHY_BT_1050, 1050000000},
    {DCMIPP_CSI_PHY_BT_1100, 1100000000},
    {DCMIPP_CSI_PHY_BT_1150, 1150000000},
    {DCMIPP_CSI_PHY_BT_1200, 1200000000},
    {DCMIPP_CSI_PHY_BT_1250, 1250000000},
    {DCMIPP_CSI_PHY_BT_1300, 1300000000},
    {DCMIPP_CSI_PHY_BT_1350, 1350000000},
    {DCMIPP_CSI_PHY_BT_1400, 1400000000},
    {DCMIPP_CSI_PHY_BT_1450, 1450000000},
    {DCMIPP_CSI_PHY_BT_1500, 1500000000},
    {DCMIPP_CSI_PHY_BT_1550, 1550000000},
    {DCMIPP_CSI_PHY_BT_1600, 1600000000},
    {DCMIPP_CSI_PHY_BT_1650, 1650000000},
    {DCMIPP_CSI_PHY_BT_1700, 1700000000},
    {DCMIPP_CSI_PHY_BT_1750, 1750000000},
    {DCMIPP_CSI_PHY_BT_1800, 1800000000},
    {DCMIPP_CSI_PHY_BT_1850, 1850000000},
    {DCMIPP_CSI_PHY_BT_1900, 1900000000},
    {DCMIPP_CSI_PHY_BT_1950, 1950000000},
    {DCMIPP_CSI_PHY_BT_2000, 2000000000},
    {DCMIPP_CSI_PHY_BT_2050, 2050000000},
    {DCMIPP_CSI_PHY_BT_2100, 2100000000},
    {DCMIPP_CSI_PHY_BT_2150, 2150000000},
    {DCMIPP_CSI_PHY_BT_2200, 2200000000},
    {DCMIPP_CSI_PHY_BT_2250, 2250000000},
    {DCMIPP_CSI_PHY_BT_2300, 2300000000},
    {DCMIPP_CSI_PHY_BT_2350, 2350000000},
    {DCMIPP_CSI_PHY_BT_2400, 2400000000},
    {DCMIPP_CSI_PHY_BT_2450, 2450000000},
    {DCMIPP_CSI_PHY_BT_2500, 2500000000}
  };

  uint32_t closest_i = PHYBitrate_in_mps[0][0];
  int32_t min_diff = abs((int32_t)val - (int32_t)PHYBitrate_in_mps[0][1]);
  size_t size = sizeof(PHYBitrate_in_mps) / sizeof(PHYBitrate_in_mps[0]);

  if (size == 0)
  {
      return -1;
  }
  for (size_t i = 1; i < size; i++)
  {
      int32_t diff = abs((int32_t)val - (int32_t)PHYBitrate_in_mps[i][1]);
      if (diff < min_diff)
      {
          min_diff = diff;
          closest_i = PHYBitrate_in_mps[i][0];
      }
  }
  return closest_i;
}

static void CMW_UTILS_get_crop_config(uint32_t cam_width, uint32_t cam_height, uint32_t pipe_width, uint32_t pipe_height, DCMIPP_CropConfTypeDef *crop)
{
  const float ratio_width = (float)cam_width / pipe_width ;
  const float ratio_height = (float)cam_height / pipe_height;
  const float ratio = MIN(ratio_width, ratio_height);

  assert(ratio >= 1);
  assert(ratio < 64);

  crop->HSize = (uint32_t) MIN(pipe_width * ratio, cam_width);
  crop->VSize = (uint32_t) MIN(pipe_height * ratio, cam_height);
  crop->HStart = (cam_width - crop->HSize + 1) / 2;
  crop->VStart = (cam_height - crop->VSize + 1) / 2;
  crop->PipeArea = DCMIPP_POSITIVE_AREA;
}

static void CMW_UTILS_get_crop_config_from_manual(CMW_Manual_roi_area_t *roi, DCMIPP_CropConfTypeDef *crop)
{
  crop->HSize = roi->width;
  crop->VSize = roi->height;
  crop->HStart = roi->offset_x;
  crop->VStart = roi->offset_y;
}

static void CMW_UTILS_get_down_config(float ratio_width, float ratio_height, int width, int height, DCMIPP_DownsizeTypeDef *down)
{
  down->HRatio = (uint32_t) (8192 * ratio_width);
  down->VRatio = (uint32_t) (8192 * ratio_height);
  down->HDivFactor = (1024 * 8192 - 1) / down->HRatio;
  down->VDivFactor = (1024 * 8192 - 1) / down->VRatio;
  down->HSize = width;
  down->VSize = height;
}

static uint32_t CMW_UTILS_get_dec_ratio_from_decimal_ratio(int dec_ratio, int is_vertical)
{
  switch (dec_ratio) {
  case 1:
    return is_vertical ? DCMIPP_VDEC_ALL : DCMIPP_HDEC_ALL;
  case 2:
    return is_vertical ? DCMIPP_VDEC_1_OUT_2 : DCMIPP_HDEC_1_OUT_2;
  case 4:
    return is_vertical ? DCMIPP_VDEC_1_OUT_4 : DCMIPP_HDEC_1_OUT_4;
  case 8:
    return is_vertical ? DCMIPP_VDEC_1_OUT_8 : DCMIPP_HDEC_1_OUT_8;
  default:
    assert(0);
  }

  return is_vertical ? DCMIPP_VDEC_ALL : DCMIPP_HDEC_ALL;
}

static uint32_t CMW_UTILS_get_dec_ratio_and_update(float *ratio, int is_vertical)
{
  int dec_ratio = 1;

  while (*ratio >= 8) {
    dec_ratio *= 2;
    *ratio /= 2;
  }

  return CMW_UTILS_get_dec_ratio_from_decimal_ratio(dec_ratio, is_vertical);
}

static void CMW_UTILS_get_scale_configs(CMW_DCMIPP_Conf_t *p_conf, float ratio_width, float ratio_height,
                                       DCMIPP_DecimationConfTypeDef *dec, DCMIPP_DownsizeTypeDef *down)
{
  dec->HRatio = CMW_UTILS_get_dec_ratio_and_update(&ratio_width, 0);
  dec->VRatio = CMW_UTILS_get_dec_ratio_and_update(&ratio_height, 1);
  CMW_UTILS_get_down_config(ratio_width, ratio_height, p_conf->output_width, p_conf->output_height, down);
}

