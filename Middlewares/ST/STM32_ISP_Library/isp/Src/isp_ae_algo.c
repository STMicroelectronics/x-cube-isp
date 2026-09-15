/**
 ******************************************************************************
 * @file    isp_ae_algo.c
 * @author  AIS Application Team
 * @brief   ISP AE algorithm
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
#include "isp_core.h"
#include "isp_services.h"

/* Private types -------------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
#define AE_FINE_TOLERANCE                   5
#define AE_FINE_TOLERANCE_LOW_LUX(target)   ((target) < 15 ? 8 : 10)
#define AE_COARSE_TOLERANCE                 10
#define AE_COARSE_TOLERANCE_LOW_LUX(target) ((target) < 15 ? 10 : 15)
#define AE_TOLERANCE                        0.10f  /* % */
#define AE_TOLERANCE_LOW_LUX                0.15f  /* % */

#define AE_LOW_LUX_LIMIT                    50    /* lux */

#define AE_EXPOSURE_COARSE_INCREMENT        500   /* us */
#define AE_EXPOSURE_FINE_INCREMENT          150   /* us */

#define AE_GAIN_COARSE_INCREMENT            2000  /* mdB */

#define AE_MAX_GAIN_INCREMENT               10000 /* mdB */

#define MDB_TO_LINEAR(g)                    (pow(10, (double)(g) / 20000))

/* Private macro -------------------------------------------------------------*/
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static ISP_IQParamTypeDef *IQParamConfig;
static ISP_SensorInfoTypeDef *pSensorInfo;

/* Global variables ----------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  isp_ae_init
  *         This function initializes ISP parameters and sensor info
  *         that will be used to process AE algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *)
  * @retval None
  */
void isp_ae_init(ISP_HandleTypeDef *hIsp)
{
  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);
  pSensorInfo = &hIsp->sensorInfo;
}

/**
  * @brief  isp_ae_compute_antiflicker
  *         ONLY USEFUL WHEN WHEN ANTI-FLICKER IS ACTIVATED
  *         Otherwise exposure time and gain are unchanged
  *         This function will compute an exposure time that eliminates the flickering effect
  *         and compensates with gain to still achieve the desired brightness
  * @param  gain             : current sensor gain value (mdB)
  * @param  exposure         : current sensor exposure time value (us)
  * @param  adjusted_gain    : pointer to the new sensor gain value (mdB)
  * @param  adjusted_exposure: pointer to the new exposure time value (us)
  * @retval None
  */
static void isp_ae_compute_antiflicker(uint32_t gain, uint32_t exposure,
                                      uint32_t *adjusted_gain, uint32_t *adjusted_exposure)
{
  /* Get equivalent gain/exposure where exposure is a multiple of the flickering period */
  float compensation_gain;
  uint32_t compensation_gain_mdb, compat_period_us;
  uint32_t up_equiv_exposure;

  if (IQParamConfig->AECAlgo.antiFlickerFreq == 0)
  {
    *adjusted_gain = gain;
    *adjusted_exposure = exposure;
    return;
  }

  compat_period_us = 1000U * 1000U / (2U * IQParamConfig->AECAlgo.antiFlickerFreq);
  up_equiv_exposure = (1 + exposure / compat_period_us) * compat_period_us;
  if ((exposure > compat_period_us) && (exposure < 0.95 * up_equiv_exposure))
  {
    /* Make exposure a multiple of the flickering period in case exposure is higher than the
       flickering period and we are 5% under a multiple value */
    *adjusted_exposure = (exposure / compat_period_us) * compat_period_us;

    /* Increase the gain accordingly */
    compensation_gain = (float)exposure / (float)(*adjusted_exposure + 1);
    compensation_gain_mdb = (uint32_t)(20 * 1000 * log10(compensation_gain));
    *adjusted_gain = gain + compensation_gain_mdb;
    if (*adjusted_gain > pSensorInfo->gain_max)
    {
      *adjusted_gain = pSensorInfo->gain_max;
    }
  }
  else
  {
    /* Keep the initial values in case:
       - We cannot update the exposure because the values are under the flickering period (will flicker)
       - Or we are close to the upper multiple value (the acceptance criteria is set to 95% of the
         multiple value to ensure no flickering effect will be visible) */
    *adjusted_gain = gain;
    *adjusted_exposure = exposure;
  }
}

/**
  * @brief  isp_ae_reverse_antiflicker
  *         ONLY USEFUL WHEN ANTI-FLICKER IS ACTIVATED
  *         Otherwise exposure time and gain are unchanged
  *         This function computes the original exposure time and gain values
  *         from the adjusted values used to eliminate flickering and maintain brightness.
  * @param  gain             : current sensor gain value (mdB)
  * @param  exposure         : current sensor exposure time value (us)
  * @param  original_gain    : pointer to the new exposure time value (us)
  * @param  original_exposure: pointer to the new sensor gain value (mdB)
  * @retval None
  */
static void isp_ae_reverse_antiflicker(uint32_t gain, uint32_t exposure,
                                       uint32_t *original_gain, uint32_t *original_exposure)
{
  /* Get equivalent gain/exposure where exposure has a maximum value (reverse processing of the above function) */
  float compensation_gain;
  uint32_t global_exposure;

  /* In case the exposure time is set to the maximum value, no antiflicker compensation is detected.
   * Return the same sensor settings */
  if (exposure == pSensorInfo->exposure_max)
  {
    *original_exposure = exposure;
    *original_gain = gain;
    return;
  }

  /* Otherwise, compute adjusted gain and exposure even if antiflicker is disabled
   * This way, in case there is no light condition change, previous adjustment for antiflicker
   * will be removed */

  global_exposure = (uint32_t)(exposure * MDB_TO_LINEAR(gain));
  if (global_exposure < pSensorInfo->exposure_max)
  {
    *original_gain = 0;
    *original_exposure = global_exposure;
    /* Fix rounding error when close to max value */
    if (*original_exposure > 0.98 * pSensorInfo->exposure_max)
    {
      *original_exposure = pSensorInfo->exposure_max;
    }
  }
  else
  {
    /* Set exposure to max, and compute the compensation gain */
    *original_exposure = pSensorInfo->exposure_max;
    /* Increase the gain accordingly */
    compensation_gain = (float)global_exposure / (float)*original_exposure;
    *original_gain = (uint32_t)(20 * 1000 * log10(compensation_gain));
  }
}

/**
  * @brief  isp_ae_compute_lux_limits
  *         Computes the custom low and high lux limits used to select the estimation model.
  *         custom_low_lux_limit is the first multiple of AE_LOW_LUX_LIMIT above the mathematical
  *         limit of the Estimation 2 model.
  *         custom_high_lux_limit is the equivalent threshold based on HL references.
  * @param  pLowLimit : pointer to output custom_low_lux_limit value
  * @param  pHighLimit: pointer to output custom_high_lux_limit value
  * @retval None
  */
static void isp_ae_compute_lux_limits(uint32_t *pLowLimit, uint32_t *pHighLimit)
{
  *pLowLimit = ((uint32_t)(((double)IQParamConfig->AECAlgo.exposureTarget * (double)IQParamConfig->luxRef.calibFactor * (IQParamConfig->luxRef.LL_LuxRef *
      ((double)IQParamConfig->luxRef.LL_Expo1 / IQParamConfig->luxRef.LL_Lum1 -
       (double)IQParamConfig->luxRef.LL_Expo2 / IQParamConfig->luxRef.LL_Lum2) /
      ((double)IQParamConfig->luxRef.LL_Expo1 - IQParamConfig->luxRef.LL_Expo2))) / AE_LOW_LUX_LIMIT) + 1) * AE_LOW_LUX_LIMIT;

  *pHighLimit = ((uint32_t)(((double)IQParamConfig->AECAlgo.exposureTarget * (double)IQParamConfig->luxRef.calibFactor * (IQParamConfig->luxRef.HL_LuxRef *
      ((double)IQParamConfig->luxRef.HL_Expo1 / IQParamConfig->luxRef.HL_Lum1 -
       (double)IQParamConfig->luxRef.HL_Expo2 / IQParamConfig->luxRef.HL_Lum2) /
      ((double)IQParamConfig->luxRef.HL_Expo1 - IQParamConfig->luxRef.HL_Expo2))) / AE_LOW_LUX_LIMIT) + 1) * AE_LOW_LUX_LIMIT;
}

/**
  * @brief  isp_ae_split_global_exposure
  *         Clamps and splits a global exposure value into separate exposure time and gain.
  *         Optionally limits the digital gain increment to avoid oscillations.
  * @param  new_global_exposure: computed global exposure to split
  * @param  pExposure          : pointer to the output exposure time value (us)
  * @param  pGain              : pointer to the output gain value (mdB)
  * @param  limit_gain         : if non-zero, apply digital gain limiting using curGain as reference
  * @param  curGain            : current gain used as reference for gain limiting
  * @retval None
  */
static void isp_ae_split_global_exposure(double new_global_exposure, uint32_t *pExposure, uint32_t *pGain,
                                         uint8_t limit_gain, uint32_t curGain)
{
  if (new_global_exposure <= pSensorInfo->exposure_max)
  {
    *pGain = 0;
    *pExposure = (new_global_exposure < pSensorInfo->exposure_min) ? pSensorInfo->exposure_min : (uint32_t)new_global_exposure;
  }
  else
  {
    *pExposure = pSensorInfo->exposure_max;
    *pGain = (uint32_t)(20 * 1000 * log10(new_global_exposure / (double)(*pExposure)));

    /* Limit digital gain (lux value is very low and the gain is already very high and we need to avoid oscillations) */
    if (limit_gain && (*pGain > pSensorInfo->again_max) && (abs((int32_t)*pGain - (int32_t)curGain) > AE_MAX_GAIN_INCREMENT))
    {
      *pGain = *pGain < curGain ? (curGain < AE_MAX_GAIN_INCREMENT ? 0 : curGain - AE_MAX_GAIN_INCREMENT) : curGain + AE_MAX_GAIN_INCREMENT;
    }
    *pGain = (*pGain < pSensorInfo->gain_min) ? pSensorInfo->gain_min : (*pGain > pSensorInfo->gain_max) ? pSensorInfo->gain_max : *pGain;
  }
}

/**
  * @brief  isp_ae_handle_start_conditions
  *         Handles the initial sensor bootstrap when luminance is very low and
  *         sensor is at minimum settings. Slightly increases exposure to allow
  *         a first non-zero lux estimation.
  * @param  averageL : current average luminance statistic
  * @param  exposure : current sensor exposure time value (us)
  * @param  gain     : current sensor gain value (mdB)
  * @param  pExposure: pointer to the new exposure time value (us)
  * @param  pGain    : pointer to the new sensor gain value (mdB)
  * @retval 1 if start conditions were handled (caller should return), 0 otherwise
  */
static uint8_t isp_ae_handle_start_conditions(uint32_t averageL, uint32_t exposure, uint32_t gain,
                                              uint32_t *pExposure, uint32_t *pGain)
{
  double new_global_exposure;

  if (averageL <= 5 && exposure == pSensorInfo->exposure_min && gain == pSensorInfo->gain_min)
  {
    new_global_exposure = gain ? exposure * MDB_TO_LINEAR(gain + 3000) : exposure + 2000;
    if (new_global_exposure <= pSensorInfo->exposure_max)
    {
      *pGain = 0;
      *pExposure = (new_global_exposure < pSensorInfo->exposure_min) ? pSensorInfo->exposure_min : (uint32_t)new_global_exposure;
    }
    else
    {
      *pExposure = pSensorInfo->exposure_max;
      *pGain = (uint32_t)(20 * 1000 * log10((float)new_global_exposure / (float)(*pExposure)));
      *pGain = (*pGain < pSensorInfo->gain_min) ? pSensorInfo->gain_min : (*pGain > pSensorInfo->again_max) ? pSensorInfo->again_max : *pGain;
    }
    return 1;
  }
  return 0;
}

/**
  * @brief  isp_ae_compute_model_coefficients
  *         Selects the estimation model (1 or 2) based on the lux value and computes
  *         the a and b coefficients used in the exposure estimation formula.
  *         - Estimation 2 (low lux references) is used when lux <= HL_LuxRef or lux <= custom_high_lux_limit
  *         - Estimation 1 (high lux references) is used otherwise
  * @param  lux               : current lux value of the captured scene
  * @param  custom_high_lux_limit: threshold for model selection
  * @param  pa                : pointer to output coefficient a
  * @param  pb                : pointer to output coefficient b
  * @retval None
  */
static void isp_ae_compute_model_coefficients(uint32_t lux, uint32_t custom_high_lux_limit,
                                              double *pa, double *pb)
{
  if ((lux <= IQParamConfig->luxRef.HL_LuxRef) || (lux <= custom_high_lux_limit))
  {
    /* ESTIMATION 2: use low lux references for improved precision */
    *pa = (IQParamConfig->luxRef.LL_LuxRef *
        ((double)IQParamConfig->luxRef.LL_Expo1 / IQParamConfig->luxRef.LL_Lum1 -
         (double)IQParamConfig->luxRef.LL_Expo2 / IQParamConfig->luxRef.LL_Lum2)) /
        ((double)IQParamConfig->luxRef.LL_Expo1 - IQParamConfig->luxRef.LL_Expo2);

    *pb = (IQParamConfig->luxRef.LL_LuxRef * (double)IQParamConfig->luxRef.LL_Expo1 / IQParamConfig->luxRef.LL_Lum1) -
        (*pa * IQParamConfig->luxRef.LL_Expo1);
  }
  else
  {
    /* ESTIMATION 1: use high lux references for higher lux conditions */
    *pa = (IQParamConfig->luxRef.HL_LuxRef *
        ((double)IQParamConfig->luxRef.HL_Expo1 / IQParamConfig->luxRef.HL_Lum1 -
         (double)IQParamConfig->luxRef.HL_Expo2 / IQParamConfig->luxRef.HL_Lum2)) /
        ((double)IQParamConfig->luxRef.HL_Expo1 - IQParamConfig->luxRef.HL_Expo2);

    *pb = (IQParamConfig->luxRef.HL_LuxRef * (double)IQParamConfig->luxRef.HL_Expo1 / IQParamConfig->luxRef.HL_Lum1) -
        (*pa * IQParamConfig->luxRef.HL_Expo1);
  }
}

/**
  * @brief  isp_ae_compute_coarse_exposure
  *         Computes the new global exposure using one of the 3 estimation models:
  *         - Estimation 3 for very low lux (below custom_low_lux_limit)
  *         - Estimation 1 or 2 for other conditions (using a, b coefficients)
  * @param  lux               : current lux value
  * @param  a                 : model coefficient a
  * @param  b                 : model coefficient b
  * @param  custom_low_lux_limit: threshold for very low lux model
  * @retval new global exposure value
  */
static double isp_ae_compute_coarse_exposure(uint32_t lux, double a, double b, uint32_t custom_low_lux_limit)
{
  double new_global_exposure;

  if (lux <= custom_low_lux_limit)
  {
    /* ESTIMATION 3: very low lux linear model */
    double d = pSensorInfo->exposure_max * MDB_TO_LINEAR(pSensorInfo->again_max);
    double c = ((b / (((double)custom_low_lux_limit / ((double)IQParamConfig->AECAlgo.exposureTarget * (double)IQParamConfig->luxRef.calibFactor)) - (double)a)) - d) / custom_low_lux_limit;
    new_global_exposure = (c * (double)lux) + d;
  }
  else
  {
    /* ESTIMATION 1 or 2: use a and b coefficients */
    new_global_exposure = b / (((double)lux / ((double)IQParamConfig->AECAlgo.exposureTarget * (double)IQParamConfig->luxRef.calibFactor)) - (double)a);
  }

  return new_global_exposure;
}

/**
  * @brief  isp_ae_apply_luminance_ratio
  *         Applies luminance ratio scaling on global exposure with an optional exponent.
  *         Formula: new = cur * (target / averageL)^exp, where exp is clamped to [0, 1].
  * @param  cur_global_exposure: current global exposure
  * @param  averageL           : current average luminance statistic
  * @param  convSpeedExp       : ratio exponent linked to convergence speed, in range [0, 1]
  * @retval scaled global exposure
  */
static double isp_ae_apply_luminance_ratio(double cur_global_exposure, uint32_t averageL, double convSpeedExp)
{
  double ratio;

  if (averageL == 0U)
  {
    return cur_global_exposure;
  }

  if (convSpeedExp < 0.0)
  {
    convSpeedExp = 0.0;
  }
  else if (convSpeedExp > 1.0)
  {
    convSpeedExp = 1.0;
  }

  ratio = (double)IQParamConfig->AECAlgo.exposureTarget / averageL;
  return cur_global_exposure * pow(ratio, convSpeedExp);
}

/**
  * @brief  isp_ae_validate_exposure_increase
  *         Validates and corrects the new global exposure when exposure should be increased
  *         (averageL < exposureTarget). Handles wrong estimation correction and applies
  *         ratio consistency safeguards.
  * @param  new_global_exposure: pointer to the computed global exposure (will be corrected in place)
  * @param  cur_global_exposure: current global exposure
  * @param  averageL           : current average luminance
  * @retval None
  */
static void isp_ae_validate_exposure_increase(double *new_global_exposure, double cur_global_exposure,
                                              uint32_t averageL)
{
  /* Check if estimation is valid (exposure should increase) */
  if (*new_global_exposure <= cur_global_exposure)
  {
    /* Wrong estimation, to be corrected */
    if (averageL != 0)
    {
      *new_global_exposure = isp_ae_apply_luminance_ratio(cur_global_exposure, averageL, 1.0);
    }
    else
    {
      /* averageL == 0, low lux or sensor exposure is set at its mininmal value */
      *new_global_exposure = (cur_global_exposure > pSensorInfo->exposure_min) ? cur_global_exposure * 1.1 : 2000;
    }
  }

  /* Additional safeguards: check ratios are consistent */
  if (cur_global_exposure != 0 && averageL != 0)
  {
    double expo_ratio = *new_global_exposure / cur_global_exposure;
    double lum_ratio = (double)IQParamConfig->AECAlgo.exposureTarget / averageL;

    if (((expo_ratio < 1.10) && (lum_ratio > 1.40)) ||
        ((expo_ratio < 1.50) && (lum_ratio > 1.80)) ||
        ((expo_ratio > 1.65) && (lum_ratio < 1.30)) ||
        ((expo_ratio > 1.35) && (lum_ratio < 1.10)) ||
        (expo_ratio < 1.02))
    {
      *new_global_exposure = cur_global_exposure * lum_ratio;
    }
  }
}

/**
  * @brief  isp_ae_validate_exposure_decrease
  *         Validates and corrects the new global exposure when exposure should be decreased
  *         (averageL >= exposureTarget). Handles wrong estimation correction and applies
  *         ratio consistency safeguards.
  * @param  new_global_exposure: pointer to the computed global exposure (will be corrected in place)
  * @param  cur_global_exposure: current global exposure
  * @param  averageL           : current average luminance
  * @retval None
  */
static void isp_ae_validate_exposure_decrease(double *new_global_exposure, double cur_global_exposure,
                                              uint32_t averageL)
{
  /* Check if estimation is valid (exposure should decrease) */
  if (*new_global_exposure >= cur_global_exposure)
  {
    /* Wrong estimation, to be corrected */
    if (averageL != 0)
    {
      *new_global_exposure = isp_ae_apply_luminance_ratio(cur_global_exposure, averageL, 1.0);
    }
    else
    {
      /* averageL == 0 (should be filtered earlier): bump exposure to avoid div-by-zero */
      *new_global_exposure = cur_global_exposure * 0.9;
    }
  }

  /* Additional safeguards: check ratios are consistent */
  if (cur_global_exposure != 0 && averageL != 0)
  {
    double expo_ratio = *new_global_exposure / cur_global_exposure;
    double lum_ratio = (double)IQParamConfig->AECAlgo.exposureTarget / averageL;

    if (((expo_ratio > 0.60) && (lum_ratio < 0.40)) ||
        ((expo_ratio < 0.45) && (lum_ratio > 0.65)) ||
        ((expo_ratio < 0.15) && (lum_ratio > 0.50)) ||
        ((expo_ratio < 0.60) && (lum_ratio > 0.85)) ||
        (expo_ratio > 0.98))
    {
      *new_global_exposure = cur_global_exposure * lum_ratio;
    }
  }
}

/**
  * @brief  isp_ae_process_fine_convergence
  *         Applies small exposure adjustments when coarse convergence is reached
  *         but fine convergence is not yet achieved.
  * @param  lux               : current lux value
  * @param  averageL          : current average luminance
  * @param  cur_global_exposure: current global exposure
  * @param  curGain           : current gain after antiflicker reversal
  * @param  curExposure       : current exposure after antiflicker reversal
  * @param  pExposure         : pointer to the new exposure time value (us)
  * @param  pGain             : pointer to the new sensor gain value (mdB)
  * @retval None
  */
static void isp_ae_process_fine_convergence(uint32_t lux, uint32_t averageL,
                                            double cur_global_exposure, uint32_t curGain, uint32_t curExposure,
                                            uint32_t *pExposure, uint32_t *pGain)
{
  double new_global_exposure = cur_global_exposure;

  /* Check if fine convergence processing is required */
  if (((lux > AE_LOW_LUX_LIMIT) && (abs((int32_t)averageL - (int32_t)IQParamConfig->AECAlgo.exposureTarget) > AE_FINE_TOLERANCE)) ||
      ((lux <= AE_LOW_LUX_LIMIT) && (abs((int32_t)averageL - (int32_t)IQParamConfig->AECAlgo.exposureTarget) > AE_FINE_TOLERANCE_LOW_LUX(IQParamConfig->AECAlgo.exposureTarget))))
  {
    /* FINE CONVERGENCE not reached: apply small adjustments */
    if (averageL > 0)
    {
      /* Compute new global exposure based on the ratio of target to current average luminance */
      new_global_exposure = isp_ae_apply_luminance_ratio(cur_global_exposure, averageL, 1.0);
    }
    else
    {
      /* averageL == 0 (should be filtered earlier): bump exposure to avoid div-by-zero
         In this case, we have a dark frame and in any case, an increment should be added */
      new_global_exposure += AE_EXPOSURE_FINE_INCREMENT;
    }

    /* Clamp and split exposure into exposure value and gain */
    isp_ae_split_global_exposure(new_global_exposure, pExposure, pGain, 0, 0);
  }
  else
  {
    /* FINE CONVERGENCE is reached - converged */
    *pExposure = curExposure;
    *pGain = curGain;
  }
}

/**
  * @brief  isp_ae_is_coarse_convergence_not_reached
  *         Checks whether coarse convergence is not yet reached.
  *         Uses different tolerance thresholds depending on lux conditions.
  * @param  lux      : current lux value
  * @param  averageL : current average luminance statistic
  * @retval 1 if coarse convergence is NOT reached, 0 if reached
  */
static uint8_t isp_ae_is_coarse_convergence_not_reached(uint32_t lux, uint32_t averageL)
{
  uint32_t delta = abs((int32_t)averageL - (int32_t)IQParamConfig->AECAlgo.exposureTarget);

  if (lux > AE_LOW_LUX_LIMIT)
  {
    return (delta > MAX((float)IQParamConfig->AECAlgo.exposureTarget * AE_TOLERANCE,
                        AE_COARSE_TOLERANCE));
  }
  else
  {
    return (delta > MAX((float)IQParamConfig->AECAlgo.exposureTarget * AE_TOLERANCE_LOW_LUX,
                        AE_COARSE_TOLERANCE_LOW_LUX(IQParamConfig->AECAlgo.exposureTarget)));
  }
}

/**
  * @brief  isp_ae_get_new_exposure
  *         This function computes the new sensor exposure to be applied
  *         for a given lux and a given luminance target.
  *         This function is implemented in accordance with the exposure estimation
  *         principles based on the estimated lux value of the current scene.
  *         There are 3 models of new sensor exposure estimation used in this AE algorithm:
  *         - Estimation 1 is used for values above LuxRef_HL
  *         - Estimation 2 is used for values below LuxRef_HL and above custom_low_lux_limit
  *         - Estimation 3 is used for values from 0 to custom_low_lux_limit
  *         Two types of convergence are targeted:
  *         - COARSE CONVERGENCE: in the 1st iteration, the exposure calculation brings
  *           the luminance value close to the target
  *         - FINE CONVERGENCE: in the following iterations, small increments are applied
  *           to reach high accuracy.
  * @param  lux      : current lux value of the captured scene
  * @param  averageL : current average luminance statistic
  * @param  pExposure: pointer to the new exposure time value (us)
  * @param  pGain    : pointer to the new sensor gain value (mdB)
  * @param  exposure : current sensor exposure time value (us)
  * @param  gain     : current sensor gain value (mdB)
  * @retval None
  */
void isp_ae_get_new_exposure(uint32_t lux, uint32_t averageL, uint32_t *pExposure, uint32_t *pGain, uint32_t exposure, uint32_t gain)
{
  double a, b;
  double cur_global_exposure = exposure * MDB_TO_LINEAR(gain);
  double new_global_exposure;
  uint32_t custom_low_lux_limit, custom_high_lux_limit;
  uint32_t adjExposure, adjGain;

  /* Compute lux thresholds for model selection */
  isp_ae_compute_lux_limits(&custom_low_lux_limit, &custom_high_lux_limit);

  /**** Handle start conditions ****/
  if (isp_ae_handle_start_conditions(averageL, exposure, gain, pExposure, pGain))
  {
    return;
  }

  /**** Get equivalent sensor gain/exposure where exposure is at its max possible value ****/
  isp_ae_reverse_antiflicker(gain, exposure, &gain, &exposure);

  /**** Check if coarse convergence is reached ****/
  /* Tolerance for coarse convergence is different in case we are in very low lux conditions (slightly increased) */
  if (isp_ae_is_coarse_convergence_not_reached(lux, averageL))
  {
    /**** COARSE CONVERGENCE not reached ****/

    /* Select and compute model coefficients */
    isp_ae_compute_model_coefficients(lux, custom_high_lux_limit, &a, &b);

    /* Compute new global exposure from the estimation model */
    new_global_exposure = isp_ae_compute_coarse_exposure(lux, a, b, custom_low_lux_limit);

    /* Validate and correct the estimation */
    if (averageL < IQParamConfig->AECAlgo.exposureTarget)
    {
      isp_ae_validate_exposure_increase(&new_global_exposure, cur_global_exposure, averageL);
    }
    else
    {
      isp_ae_validate_exposure_decrease(&new_global_exposure, cur_global_exposure, averageL);
    }

    /* Clamp and split exposure into exposure value and gain (with digital gain limiting) */
    isp_ae_split_global_exposure(new_global_exposure, pExposure, pGain, 1, gain);
  }
  else
  {
    /**** COARSE CONVERGENCE is reached - process fine convergence ****/
    isp_ae_process_fine_convergence(lux, averageL, cur_global_exposure, gain, exposure, pExposure, pGain);
  }

  /**** Consider flickering period constraint ****/
  isp_ae_compute_antiflicker(*pGain, *pExposure, &adjGain, &adjExposure);

  /* Return final value of sensor exposure time and gain */
  *pExposure = adjExposure;
  *pGain = adjGain;
}

/**
  * @brief  isp_ae_get_new_exposure_ratio
  *         Computes new sensor exposure from luminance ratio only:
  *         new = cur * (target / averageL)^convSpeedExp
  *         If averageL is 0, ratio cannot be applied. In this case exposure or gain is incremented.
  * @param  averageL   : current average luminance statistic
  * @param  convSpeedExp: ratio exponent in range [0, 1]
  * @param  pExposure  : pointer to the new exposure time value (us)
  * @param  pGain      : pointer to the new sensor gain value (mdB)
  * @param  exposure   : current sensor exposure time value (us)
  * @param  gain       : current sensor gain value (mdB)
  * @retval None
  */
void isp_ae_get_new_exposure_ratio(uint32_t averageL, double convSpeedExp,
                                   uint32_t *pExposure, uint32_t *pGain,
                                   uint32_t exposure, uint32_t gain)
{
  double new_global_exposure;
  double cur_global_exposure = exposure * MDB_TO_LINEAR(gain);
  uint32_t adjExposure, adjGain;

  /* Reuse the same bootstrap logic as lux-model AE to escape low-start conditions quickly. */
  if (isp_ae_handle_start_conditions(averageL, exposure, gain, pExposure, pGain))
  {
    isp_ae_compute_antiflicker(*pGain, *pExposure, &adjGain, &adjExposure);
    *pExposure = adjExposure;
    *pGain = adjGain;
    return;
  }

  if (averageL == 0U)
  {
    if (exposure >= pSensorInfo->exposure_max)
    {
      *pExposure = pSensorInfo->exposure_max;
      *pGain = gain + AE_GAIN_COARSE_INCREMENT;
      if (*pGain > pSensorInfo->gain_max)
      {
        *pGain = pSensorInfo->gain_max;
      }
    }
    else
    {
      *pExposure = exposure + AE_EXPOSURE_COARSE_INCREMENT;
      if (*pExposure > pSensorInfo->exposure_max)
      {
        *pExposure = pSensorInfo->exposure_max;
      }
      *pGain = gain;
    }
  }
  else if (abs((int32_t)averageL - (int32_t)IQParamConfig->AECAlgo.exposureTarget) <= AE_FINE_TOLERANCE)
  {
    /* Fine convergence reached: luminance is already within tolerance of the target.
       Keep the current exposure/gain to avoid a useless ratio reassessment that would
       otherwise apply a tiny correction each frame and make the image oscillate. */
    *pExposure = exposure;
    *pGain = gain;
  }
  else
  {
    new_global_exposure = isp_ae_apply_luminance_ratio(cur_global_exposure, averageL, convSpeedExp);
    isp_ae_split_global_exposure(new_global_exposure, pExposure, pGain, 0, 0);
  }

  /* Keep anti-flicker behavior aligned with standard AE flow. */
  isp_ae_compute_antiflicker(*pGain, *pExposure, &adjGain, &adjExposure);
  *pExposure = adjExposure;
  *pGain = adjGain;
}
