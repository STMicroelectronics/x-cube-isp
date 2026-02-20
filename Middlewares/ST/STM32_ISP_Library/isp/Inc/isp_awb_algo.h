/**
 ******************************************************************************
 * @file    isp_awb_algo.h
 * @author  AIS Application Team
 * @brief   Header file of ISP AWB algorithm
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ISP_AWB_ALGO__H
#define __ISP_AWB_ALGO__H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
ISP_StatusTypeDef ISP_AWB_Init(ISP_AWBAlgoTypeDef *pAWBAlgo);
ISP_StatusTypeDef ISP_AWB_GetConfig(ISP_StatisticsTypeDef *pStats, ISP_ColorConvTypeDef *pColorConvConfig, ISP_ISPGainTypeDef *pISPGainConfig, uint32_t *pColorTemp);

/* Exported variables --------------------------------------------------------*/

#endif /* __ISP_AWB_ALGO__H */
