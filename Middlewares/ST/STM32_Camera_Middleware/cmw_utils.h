 /**
 ******************************************************************************
 * @file    cmw_utils.h
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

#ifndef CMW_UTILS
#define CMW_UTILS

#include "stm32n6xx_hal.h"
#include "cmw_camera.h"
#if !defined (CMW_USE_WITHOUT_ISP)
#include "isp_api.h"
#endif

int32_t CMW_UTILS_getClosest_HAL_PHYBitrate(uint32_t val);
void CMW_UTILS_GetPipeConfig(uint32_t cam_width, uint32_t cam_height, uint32_t ratio_h, uint32_t ratio_v, CMW_DCMIPP_Conf_t *p_conf,
                                    DCMIPP_CropConfTypeDef *crop, DCMIPP_DecimationConfTypeDef *dec,
                                    DCMIPP_DownsizeTypeDef *down);
int32_t CMW_UTILS_GetIspDecimationRatio_NoIsp(int32_t *ratio_h, int32_t *ratio_v);
#if !defined (CMW_USE_WITHOUT_ISP)
int32_t CMW_UTILS_GetIspDecimationRatio_WithIsp(ISP_HandleTypeDef *hIsp, int32_t *ratio_h, int32_t *ratio_v);
#endif

#endif