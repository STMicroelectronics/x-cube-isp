/**
 ******************************************************************************
 * @file    isp_ae_algo.h
 * @author  AIS Application Team
 * @brief   Header file of ISP AE algorithm
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
#ifndef __ISP_AE_ALGO__H
#define __ISP_AE_ALGO__H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void isp_ae_init(ISP_HandleTypeDef *hIsp);
void isp_ae_get_new_exposure(uint32_t lux, uint32_t averageL, uint32_t *pExposure, uint32_t *pGain, uint32_t curExposure, uint32_t curGain);

/* Exported variables --------------------------------------------------------*/

#endif /* __ISP_AE_ALGO__H */
