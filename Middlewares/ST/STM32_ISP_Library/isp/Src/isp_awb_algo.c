/**
 ******************************************************************************
 * @file    isp_awb_algo.c
 * @author  AIS Application Team
 * @brief   ISP AWB algorithm
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

/* Includes ------------------------------------------------------------------*/
#include "isp_services.h"
#include <inttypes.h>

/* Private types -------------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Debug logs control */
//#define ALGO_AWB_DBG_LOGS

/* Interpolation ratio limit from where we decide to 'stick' to a reference profile */
#define ISP_AWB_STICKY         0.05
/* Stat update limit from where we consider that the frame does not change */
#define ISP_AWB_STAT_NO_CHANGE 1

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint32_t ISP_AWB_GetRB_Ratio(const ISP_StatisticsTypeDef *pStats);
static void ISP_AWB_SelectProfiles(uint32_t rb_ratio, int *pExactProfId, uint32_t *pUpProfId, uint32_t *pDownProfId,
                                        double *pInterpolRatio);
static void ISP_AWB_ApplyExactProfile(int exactProfId, uint32_t rb_ratio, ISP_ColorConvTypeDef *pColorConvConfig,
                                      ISP_ISPGainTypeDef *pISPGainConfig, uint32_t *pColorTemp);
static void ISP_AWB_ApplyInterpolatedProfile(uint32_t upProfId, uint32_t downProfId, double interpolRatio, uint32_t rb_ratio,
                                             ISP_ColorConvTypeDef *pColorConvConfig, ISP_ISPGainTypeDef *pISPGainConfig,
                                             uint32_t *pColorTemp);
/* Private variables ---------------------------------------------------------*/
static ISP_AWBAlgoTypeDef ISP_AWB_Config;
static uint32_t ISP_AWB_rbRatio[ISP_AWB_COLORTEMP_REF];
static uint32_t ISP_AWB_NbProfiles;
static ISP_StatisticsTypeDef ISP_AWB_CurrStats;
static uint32_t ISP_AWB_CurrTemp;
static ISP_ColorConvTypeDef ISP_AWB_CurrColorConv;
static ISP_ISPGainTypeDef ISP_AWB_CurrISPGain;
static uint32_t rb_ratio_previous;
static int error_previous;

/* Global variables ----------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static uint32_t ISP_AWB_GetRB_Ratio(const ISP_StatisticsTypeDef *pStats)
{
  return pStats->averageB != 0 ? (uint32_t)(pStats->averageR * 1000 / pStats->averageB) : ISP_AWB_rbRatio[0];
}

static void ISP_AWB_SelectProfiles(uint32_t rb_ratio, int *pExactProfId, uint32_t *pUpProfId, uint32_t *pDownProfId,
                                        double *pInterpolRatio)
{
  uint32_t profId;

  /* Find the two (up and down) profiles where R/B enclose the reported R/B */
  for (profId = 0; profId < ISP_AWB_NbProfiles; profId++)
  {
    if (rb_ratio > ISP_AWB_rbRatio[profId])
    {
      break;
    }
  }

  if (profId == 0)
  {
    /* R/B ratio is greater than the largest reference. Select the first profile (lowest color temp) */
    *pExactProfId = 0;
    return;
  }

  if (profId == ISP_AWB_NbProfiles)
  {
    /* R/B ratio is lower than the lowest reference. Select the last defined profile (highest color temp) */
    *pExactProfId = (int)profId - 1;
    return;
  }

  /* Between two profiles. Evaluate the interpolation ratio between these profiles */
  *pExactProfId = -1;
  *pUpProfId = profId;
  *pDownProfId = profId - 1;
  *pInterpolRatio = (double)(ISP_AWB_rbRatio[*pDownProfId] - rb_ratio) /
                    (ISP_AWB_rbRatio[*pDownProfId] - ISP_AWB_rbRatio[*pUpProfId]);

  /* Check if we are close to a profile, in which case we select it instead of interpolating */
  if (*pInterpolRatio <= ISP_AWB_STICKY)
  {
    /* Select the 'down' profile */
#ifdef ALGO_AWB_DBG_LOGS
    printf("R/B=%4"PRIu32"  -  Forcing down profile (ratio=%"PRIu16"%%)\r\n", rb_ratio, (int)(100 * (*pInterpolRatio)));
#endif
    *pExactProfId = (int)(*pDownProfId);
  }
  else if (*pInterpolRatio >= 1.0 - ISP_AWB_STICKY)
  {
    /* Select the 'up' profile */
#ifdef ALGO_AWB_DBG_LOGS
    printf("R/B=%4"PRIu32"  -  Forcing up profile (ratio=%"PRIu16"%%)\r\n", rb_ratio, (int)(100 * (*pInterpolRatio)));
#endif
    *pExactProfId = (int)(*pUpProfId);
  }
}

static void ISP_AWB_ApplyExactProfile(int exactProfId, uint32_t rb_ratio, ISP_ColorConvTypeDef *pColorConvConfig,
                                      ISP_ISPGainTypeDef *pISPGainConfig, uint32_t *pColorTemp)
{
  /* Use the config of this selected profile */
  memcpy(pColorConvConfig->coeff, ISP_AWB_Config.coeff[exactProfId], sizeof(pColorConvConfig->coeff));

  pISPGainConfig->ispGainR = ISP_AWB_Config.ispGainR[exactProfId];
  pISPGainConfig->ispGainG = ISP_AWB_Config.ispGainG[exactProfId];
  pISPGainConfig->ispGainB = ISP_AWB_Config.ispGainB[exactProfId];

  *pColorTemp = ISP_AWB_Config.referenceColorTemp[exactProfId];
#ifdef ALGO_AWB_DBG_LOGS
  printf("R/B=%4"PRIu32"  -  Profile exact:%"PRIu16"  -  ColorTemp = %"PRIu32"\r\n", rb_ratio, exactProfId, *pColorTemp);
#endif
}

static void ISP_AWB_ApplyInterpolatedProfile(uint32_t upProfId, uint32_t downProfId, double interpolRatio, uint32_t rb_ratio,
                                             ISP_ColorConvTypeDef *pColorConvConfig, ISP_ISPGainTypeDef *pISPGainConfig,
                                             uint32_t *pColorTemp)
{
  int64_t i64;
  int i, j;

  /* Compute the interpolated configurations */
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      i64 = (int64_t)(ISP_AWB_Config.coeff[downProfId][i][j] + interpolRatio *
            (ISP_AWB_Config.coeff[upProfId][i][j] - ISP_AWB_Config.coeff[downProfId][i][j]));
      pColorConvConfig->coeff[i][j] = (int32_t)i64;
    }
  }

  i64 = (int64_t)(ISP_AWB_Config.ispGainR[downProfId] + interpolRatio *
        ((int64_t)ISP_AWB_Config.ispGainR[upProfId] - ISP_AWB_Config.ispGainR[downProfId]));
  pISPGainConfig->ispGainR = (uint32_t)i64;

  i64 = (int64_t)(ISP_AWB_Config.ispGainG[downProfId] + interpolRatio *
        ((int64_t)ISP_AWB_Config.ispGainG[upProfId] - ISP_AWB_Config.ispGainG[downProfId]));
  pISPGainConfig->ispGainG = (uint32_t)i64;

  i64 = (int64_t)(ISP_AWB_Config.ispGainB[downProfId] + interpolRatio *
        ((int64_t)ISP_AWB_Config.ispGainB[upProfId] - ISP_AWB_Config.ispGainB[downProfId]));
  pISPGainConfig->ispGainB = (uint32_t)i64;

  *pColorTemp = (uint32_t)(ISP_AWB_Config.referenceColorTemp[downProfId] + interpolRatio *
                (ISP_AWB_Config.referenceColorTemp[upProfId] - ISP_AWB_Config.referenceColorTemp[downProfId]));
#ifdef ALGO_AWB_DBG_LOGS
  printf("R/B=%4"PRIu32"  -  Profile between %"PRIu32" and %"PRIu32" (%"PRIu16"%%)  -  ColorTemp = %"PRIu32"\r\n", rb_ratio,
         upProfId, downProfId, (int)(100 * interpolRatio), *pColorTemp);
#endif
}

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  ISP_AWB_Init
  *         Initialize and configure the AWB algo
  * @param  pAWBAlgo: Pointer to the AWB tuning configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_AWB_Init(ISP_AWBAlgoTypeDef *pAWBAlgo)
{
  uint32_t profId;

  /* Copy internally the AWB tuning parameters */
  ISP_AWB_Config = *pAWBAlgo;

  /* Reset internal variables */
  memset(&ISP_AWB_CurrStats, 0, sizeof(ISP_AWB_CurrStats));
  memset(ISP_AWB_rbRatio, 0, sizeof(ISP_AWB_rbRatio));
  ISP_AWB_CurrTemp = 0;
  ISP_AWB_CurrColorConv.enable = 0;
  ISP_AWB_CurrISPGain.enable = 0;
  ISP_AWB_NbProfiles = 0;
  rb_ratio_previous = 0;
  error_previous = 0;

  /* Check that the R/G/B references are not 0 */
  for (profId = 0; profId < ISP_AWB_COLORTEMP_REF && ISP_AWB_Config.referenceColorTemp[profId] != 0; profId++)
  {
    if ((ISP_AWB_Config.referenceRGB[profId][0] == 0) ||
        (ISP_AWB_Config.referenceRGB[profId][1] == 0) ||
        (ISP_AWB_Config.referenceRGB[profId][2] == 0))
    {
      return ISP_ERR_AWB;
    }
    ISP_AWB_rbRatio[profId] = (1000U * ISP_AWB_Config.referenceRGB[profId][0]) / ISP_AWB_Config.referenceRGB[profId][2];
  }
  ISP_AWB_NbProfiles = profId;

  /* Check that the R/B ratio are sorted in decreasing order */
  for (profId = 0; profId < ISP_AWB_NbProfiles - 1; profId++)
  {
    if (ISP_AWB_rbRatio[profId] <= ISP_AWB_rbRatio[profId + 1])
    {
      return ISP_ERR_AWB;
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_AWB_GetConfig
  *         Evaluate from the input statistics, the Color Temperature and the white-balanced
  *         ColorConv and Gain configuration.
  * @param  pStats: pointer to the current RGB statistics
  * @param  convSpeedGain: low pass filter divisor applied to the R/B ratio error. The larger, the slower the convergence
  * @param  config_changed: pointer to a boolean that is set to true if the computed config is different from the current applied config
  * @param  pColorConvConfig: pointer to the output Color Conversion configuration
  * @param  pISPGainConfig: pointer to the output ISP Gain configuration
  * @param  pColorTemp: pointer to the output estimated color temperature
  * @retval operation result
  */
 ISP_StatusTypeDef ISP_AWB_GetConfig(ISP_StatisticsTypeDef *pStats, int32_t convSpeedGain, bool *config_changed, ISP_ColorConvTypeDef *pColorConvConfig, ISP_ISPGainTypeDef *pISPGainConfig, uint32_t *pColorTemp)
 {
   int exactProfId = -1;
   double interpolRatio = 0.0;
   uint32_t rb_ratio, upProfId = 0, downProfId = 0;
   int error = 0;
   uint32_t rb_ratio_target = ISP_AWB_GetRB_Ratio(pStats);

  /* If the R/B ratio did not change much and the stats did not change much, we consider that the config does not need to be updated */
  if (abs((int32_t)(rb_ratio_previous - rb_ratio_target)) < 100)
  {
#ifdef ALGO_AWB_DBG_LOGS
    printf("R/B=%4"PRIu32"  -  No change  -  ColorTemp = %"PRIu32"\r\n", rb_ratio, ISP_AWB_CurrTemp);
#endif
    *pColorConvConfig = ISP_AWB_CurrColorConv;
    *pISPGainConfig = ISP_AWB_CurrISPGain;
    *pColorTemp = ISP_AWB_CurrTemp;
    *config_changed = false;
    return ISP_OK;
  }

  error = (int)(rb_ratio_target - rb_ratio_previous);
  rb_ratio = rb_ratio_previous + error / convSpeedGain;
  error_previous = error;

  if (abs((int32_t)(rb_ratio_target - rb_ratio)) < 100)
  {
    rb_ratio = rb_ratio_target;
  }
  rb_ratio_previous = rb_ratio;

  ISP_AWB_CurrStats = *pStats;

  ISP_AWB_SelectProfiles(rb_ratio, &exactProfId, &upProfId, &downProfId, &interpolRatio);

  /* Set the ColorConv and ISPGain configs */
  pColorConvConfig->enable = 1;
  pISPGainConfig->enable = 1;

  /* Check if an 'exact' profile was found */
  if (exactProfId != -1)
  {
    ISP_AWB_ApplyExactProfile(exactProfId, rb_ratio, pColorConvConfig, pISPGainConfig, pColorTemp);
  }
  else
  {
    ISP_AWB_ApplyInterpolatedProfile(upProfId, downProfId, interpolRatio, rb_ratio,
                                     pColorConvConfig, pISPGainConfig, pColorTemp);
  }

  /* keep a copy of the last reported config */
  ISP_AWB_CurrColorConv = *pColorConvConfig;
  ISP_AWB_CurrISPGain = *pISPGainConfig;
  ISP_AWB_CurrTemp = *pColorTemp;
  *config_changed = true;

  return ISP_OK;
}

