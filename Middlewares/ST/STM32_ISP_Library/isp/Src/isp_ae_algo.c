/**
 ******************************************************************************
 * @file    isp_ae_algo.c
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
#define AE_EXPOSURE_COARSE_DECREMENT        300   /* us */
#define AE_EXPOSURE_FINE_INCREMENT          150   /* us */
#define AE_EXPOSURE_FINE_DECREMENT          100   /* us */

#define AE_GAIN_COARSE_INCREMENT            2000  /* mdB */
#define AE_GAIN_COARSE_DECREMENT            1500  /* mdB */
#define AE_GAIN_FINE_INCREMENT              500   /* mdB */
#define AE_GAIN_FINE_DECREMENT              300   /* mdB */

#define AE_MAX_GAIN_INCREMENT               10000 /* mdB */
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
static uint32_t previous_lux = 0;

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
  * @brief  isp_ae_compute_antiflcker
  *         ONLY USEFUL WHEN WHEN ANTI-FLICKER IS ACTIVATED
  *         Otherwise exposure time and gain are unchanged
  *         This function will compute an exposure time that eliminates the flickering effect
  *         and compensates with gain to still achieve the desired brightness
  * @param  gain             : current sensor gain value (mdB)
  * @param  exposure         : current sensor exposure time value (us)
  * @param  adjusted_gain    : pointer to the new exposure time value (us)
  * @param  adjusted_exposure: pointer to the new sensor gain value (mdB)
  * @retval None
  */
static void isp_ae_compute_antiflcker(uint32_t gain, uint32_t exposure,
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

  /* Compute adjusted_gain and exposure in any case even if antiflicker is disabled
   * This way, in case there is no light condition change, previous adjustment for antiflicker
   * will be removed */

  global_exposure = (uint32_t)(exposure * pow(10, (float)gain / (20 * 1000)));
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
  * @brief  isp_ae_get_new_exposure
  *         This function computes the new sensor exposure to be applied
  *         for a given lux and a given luminance target
  *         This function is implemented in accordance with the exposure estimation
  *         principles based on the estimated lux value of the current scene.
  *         There are 3 models of new sensor exposure estimation used in this AE algorithm:
  *         - Estimation 1 is used for values above 𝐿𝑢𝑥𝑅𝑒𝑓_𝐻𝐿
  *         - Estimation 2 is used for values below 𝐿𝑢𝑥𝑅𝑒𝑓_𝐻𝐿 and above 𝑐𝑢𝑠𝑡𝑜𝑚_𝑙𝑜𝑤_𝑙𝑢𝑥_𝑙𝑖𝑚𝑖𝑡
  *         - Estimation 3 is used for values from 0 to 𝑐𝑢𝑠𝑡𝑜𝑚_𝑙𝑜𝑤_𝑙𝑢𝑥_𝑙𝑖𝑚𝑖𝑡
  *           𝑐𝑢𝑠𝑡𝑜𝑚_𝑙𝑜𝑤_𝑙𝑢𝑥_𝑙𝑖𝑚𝑖𝑡 should be set to the first multiple value of 50 above
  *           𝑎_𝐿𝐿×𝑙𝑢𝑚_𝑡𝑎𝑟𝑔𝑒𝑡×𝑐𝑎𝑙𝑖𝑏𝐹𝑎𝑐𝑡𝑜𝑟 and is the mathematical limit for the previous model
  *         Two types of convergence are targeted:
  *         - COARSE CONVERGENCE: in the 1st iteration, the exposure calculation brings
  *           the luminance value close to the target
  *         - FINE CONVERGENCE: in the following iteration,  if fine convergence has not been
  *           achieved yet with the previous calculation, small increments are applied to
  *           reach high accuracy.
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
  double a, b, c, d;
  double cur_global_exposure = exposure * pow(10, (double)gain / 20000);
  double new_global_exposure;
  uint32_t custom_low_lux_limit = ((uint32_t)(((double)IQParamConfig->AECAlgo.exposureTarget * (double)IQParamConfig->luxRef.calibFactor * (IQParamConfig->luxRef.LL_LuxRef *
          ((double)IQParamConfig->luxRef.LL_Expo1 / IQParamConfig->luxRef.LL_Lum1 -
           (double)IQParamConfig->luxRef.LL_Expo2 / IQParamConfig->luxRef.LL_Lum2) /
          ((double)IQParamConfig->luxRef.LL_Expo1 - IQParamConfig->luxRef.LL_Expo2))) / AE_LOW_LUX_LIMIT) + 1) * AE_LOW_LUX_LIMIT;
  uint32_t custom_high_lux_limit = ((uint32_t)(((double)IQParamConfig->AECAlgo.exposureTarget * (double)IQParamConfig->luxRef.calibFactor * (IQParamConfig->luxRef.HL_LuxRef *
          ((double)IQParamConfig->luxRef.HL_Expo1 / IQParamConfig->luxRef.HL_Lum1 -
           (double)IQParamConfig->luxRef.HL_Expo2 / IQParamConfig->luxRef.HL_Lum2) /
          ((double)IQParamConfig->luxRef.HL_Expo1 - IQParamConfig->luxRef.HL_Expo2))) / AE_LOW_LUX_LIMIT) + 1) * AE_LOW_LUX_LIMIT;
  uint32_t curExposure, curGain;

  /**** Handle start conditions ****/
  /* This code aims to slightly deviate from the camera's minimum exposure settings
   * in order to capture enough light and make a first non-zero estimate of lux */
  if (averageL <= 5 && exposure == pSensorInfo->exposure_min && gain == pSensorInfo->gain_min)
  {
    new_global_exposure = gain ? exposure * pow(10, ((double)gain + 3000) / 20000) : exposure + 2000;
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
    return;
  }

  /**** Get equivalent sensor gain/exposure where exposure is at its max possible value ****/
  /* ONLY USEFUL WHEN WHEN ANTI-FLICKER IS ACTIVATED
   * In case, sensor exposure time has been compensated by sensor gain for anti-flicker,
   * calculate the value that would be applied without anti-flicker activated */
  isp_ae_reverse_antiflicker(gain, exposure, &curGain, &curExposure);

  /**** Check if coarse convergence is reached ****/
  /* Tolerance for coarse convergence is different in case we are in very low lux conditions (slightly increased) */
  if (((lux > AE_LOW_LUX_LIMIT) && (abs((int32_t)averageL - (int32_t)IQParamConfig->AECAlgo.exposureTarget) > MAX((float)IQParamConfig->AECAlgo.exposureTarget * AE_TOLERANCE, AE_COARSE_TOLERANCE))) ||
      ((lux <= AE_LOW_LUX_LIMIT) && (abs((int32_t)averageL - (int32_t)IQParamConfig->AECAlgo.exposureTarget) > MAX((float)IQParamConfig->AECAlgo.exposureTarget * AE_TOLERANCE_LOW_LUX, AE_COARSE_TOLERANCE_LOW_LUX(IQParamConfig->AECAlgo.exposureTarget)))))
  {
    /**** COARSE CONVERGENCE not reached ****/
    /* The AE algorithm process intends to calculate the sensor exposure needed to approach the luminance target */

    /**** Select the estimation model to apply according to the lux value ****/
    if ((lux <= IQParamConfig->luxRef.HL_LuxRef) || (lux <= custom_high_lux_limit))
    {
      /**** Apply ESTIMATION 2 model below 𝐿𝑢𝑥𝑅𝑒𝑓_𝐻𝐿 ****/
      /* Calculate a and b with the low lux references to improve precision when lux is under HL_LuxRef */
      a = (IQParamConfig->luxRef.LL_LuxRef *
          ((double)IQParamConfig->luxRef.LL_Expo1 / IQParamConfig->luxRef.LL_Lum1 -
           (double)IQParamConfig->luxRef.LL_Expo2 / IQParamConfig->luxRef.LL_Lum2)) /
          ((double)IQParamConfig->luxRef.LL_Expo1 - IQParamConfig->luxRef.LL_Expo2);

      b = (IQParamConfig->luxRef.LL_LuxRef * (double)IQParamConfig->luxRef.LL_Expo1 / IQParamConfig->luxRef.LL_Lum1) -
          (a * IQParamConfig->luxRef.LL_Expo1);
    }
    else
    {
      /**** Apply ESTIMATION 1 model above 𝐿𝑢𝑥𝑅𝑒𝑓_𝐻𝐿 ****/
      /* Calculate a and b with the high lux references for higher lux conditions*/
      a = (IQParamConfig->luxRef.HL_LuxRef *
          ((double)IQParamConfig->luxRef.HL_Expo1 / IQParamConfig->luxRef.HL_Lum1 -
           (double)IQParamConfig->luxRef.HL_Expo2 / IQParamConfig->luxRef.HL_Lum2)) /
          ((double)IQParamConfig->luxRef.HL_Expo1 - IQParamConfig->luxRef.HL_Expo2);

      b = (IQParamConfig->luxRef.HL_LuxRef * (double)IQParamConfig->luxRef.HL_Expo1 / IQParamConfig->luxRef.HL_Lum1) -
          (a * IQParamConfig->luxRef.HL_Expo1);
    }

    if (lux <= custom_low_lux_limit)
    {
      /**** Switch to ESTIMATION 3 model below 𝑐𝑢𝑠𝑡𝑜𝑚_𝑙𝑜𝑤_𝑙𝑢𝑥_𝑙𝑖𝑚𝑖𝑡 ****/
      /* Calculate coefficient for very low lux model as we reach the limit of the previous one */
      d = pSensorInfo->exposure_max * pow(10, (double)pSensorInfo->again_max / 20000);
      c = ((b / (((double)custom_low_lux_limit / ((double)IQParamConfig->AECAlgo.exposureTarget * (double)IQParamConfig->luxRef.calibFactor)) - (double)a)) - d) / custom_low_lux_limit;

      /* Compute new global exposure to apply in ESTIMATION 3 model*/
      new_global_exposure = (c * (double)lux) + d;
    }
    else
    {
      /**** Process ESTIMATION 1 or 2 model ****/
      /* Compute new global exposure to apply in ESTIMATION 1 or 2 model*/
      /* Else apply previous estimation model with a and b coefficients */
      new_global_exposure = b / (((double)lux / ((double)IQParamConfig->AECAlgo.exposureTarget * (double)IQParamConfig->luxRef.calibFactor)) - (double)a);
    }

    /**** Check validity of new global exposure ****/
    /* Check the different cases:
     * - CASE 1: when averageL is under the target, the exposure is supposed to be increased
     * - CASE 2: when averageL is above the target, the exposure is supposed to be decreased
     * If the new exposure is not valid, then we switch to a standard incremental method,
     * using the averageL statistic as a parameter to calculate the new exposure.
     * This will ensure that the target will be reach in any case.
     * */
    if (averageL < IQParamConfig->AECAlgo.exposureTarget)
    {
      /**** CASE 1 ****/
      /* Exposure should be increased */
      if (new_global_exposure <= cur_global_exposure)
      {
        /* Wrong estimation, to be corrected */
        new_global_exposure = -1; // reset to -1

        /* Same lux estimation but previous setting did not allow convergence, so we need to change it */
        if ((abs((int32_t)lux - (int32_t)previous_lux) <= (float)lux * 0.05f) && (lux != 0))
        {
          /* Use the luminance information for readjustment */
          new_global_exposure = cur_global_exposure * ((double)IQParamConfig->AECAlgo.exposureTarget / averageL);
        }

        if (new_global_exposure <= 0)
        {
          /* Increase exposure with small increment to get closer to target */
          if (cur_global_exposure <= pSensorInfo->exposure_max)
          {
            new_global_exposure = averageL ? cur_global_exposure * (double)IQParamConfig->AECAlgo.exposureTarget / averageL : cur_global_exposure + AE_EXPOSURE_COARSE_INCREMENT;
          }
          else
          {
            new_global_exposure = pSensorInfo->exposure_max * pow(10, ((double)curGain + AE_GAIN_COARSE_INCREMENT) / 20000);
          }
        }
      }

      /**** Additional safeguards to ensure the proper behavior of the algorithm ****/
      /* Check ratios are consistent when big or small changes are requested */
      if (cur_global_exposure != 0 && averageL != 0)
      {
        /* Compare exposure ratio and luminance ratio, to check the consistency of the results */
        if ((((new_global_exposure / cur_global_exposure) < 1.10) && (((double)IQParamConfig->AECAlgo.exposureTarget / averageL) > 1.40)) || /* new global exposure is very close to previous exposure while luminance is at least 40% higher */
            (((new_global_exposure / cur_global_exposure) < 1.50) && (((double)IQParamConfig->AECAlgo.exposureTarget / averageL) > 1.80)) || /* new global exposure is less than 50% higher while luminance is more than 80% higher */
            (((new_global_exposure / cur_global_exposure) > 1.65) && (((double)IQParamConfig->AECAlgo.exposureTarget / averageL) < 1.30)) || /* new global exposure is very high while luminance is less than 30% higher */
            (((new_global_exposure / cur_global_exposure) > 1.35) && (((double)IQParamConfig->AECAlgo.exposureTarget / averageL) < 1.10)) || /* new global exposure is too high when luminance is very close to the target */
            (new_global_exposure / cur_global_exposure) < 1.02) /* Or same exposure estimation but we are not within tolerance margin */
        {
          /* Apply the same ratio */
          new_global_exposure = cur_global_exposure * ((double)IQParamConfig->AECAlgo.exposureTarget / averageL);
        }
      }
    }
    else
    {
      /**** CASE 2 ****/
      /* Exposure should be decreased */
      if (new_global_exposure >= cur_global_exposure)
      {
        /* Wrong estimation, to be corrected */
        new_global_exposure = -1;

        /* Same lux estimation but previous setting did not allow convergence, so we need to change it */
        if ((abs((int32_t)lux - (int32_t)previous_lux) <= (float)lux * 0.05f) && (lux != 0))
        {
          /* Use the luminance information for readjustment */
          new_global_exposure = cur_global_exposure * ((double)IQParamConfig->AECAlgo.exposureTarget / averageL);
        }

        if (new_global_exposure <= 0) //small step to get closer to target
        {
          /* Decrease exposure with small decrement to get closer to target */
          if (cur_global_exposure <= pSensorInfo->exposure_max)
          {
            new_global_exposure = averageL ? cur_global_exposure * (double)IQParamConfig->AECAlgo.exposureTarget / averageL : cur_global_exposure - AE_EXPOSURE_COARSE_DECREMENT;
          }
          else
          {
            new_global_exposure = pSensorInfo->exposure_max * pow(10, ((double)curGain - AE_GAIN_COARSE_DECREMENT) / 20000);
          }
        }
      }

      /**** Additional safeguards to ensure the proper behavior of the algorithm ****/
      /* Check ratios are consistent when big or small changes are requested */
      if (cur_global_exposure != 0 && averageL != 0)
      {
          /* Compare exposure ratio and luminance ratio, to check the consistency of the results */
        if ((((new_global_exposure / cur_global_exposure) > 0.60) && (((double)IQParamConfig->AECAlgo.exposureTarget / averageL) < 0.40)) || /* new global exposure is at least 60% of the current exposure while luminance is less than 40% of the target */
            (((new_global_exposure / cur_global_exposure) < 0.45) && (((double)IQParamConfig->AECAlgo.exposureTarget / averageL) > 0.65)) || /* new global exposure is less than 45% of the current exposure while luminance is more than 65% of the target */
            (((new_global_exposure / cur_global_exposure) < 0.15) && (((double)IQParamConfig->AECAlgo.exposureTarget / averageL) > 0.50)) || /* new global exposure is very low while luminance is more than 50% of the target */
            (((new_global_exposure / cur_global_exposure) < 0.60) && (((double)IQParamConfig->AECAlgo.exposureTarget / averageL) > 0.85)) || /* new global exposure is less than 60% of the current exposure while luminance is very close to the target */
            (new_global_exposure / cur_global_exposure) > 0.98) /* Or same exposure estimation but we are not within tolerance margin */
        {
          /* Apply the same ratio */
          new_global_exposure = cur_global_exposure * ((double)IQParamConfig->AECAlgo.exposureTarget / averageL);
        }
      }
    }

    /**** Clamp and split exposure into exposure value and gain ****/
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
      if ((*pGain > pSensorInfo->again_max) && (abs((int32_t)*pGain - (int32_t)curGain) > AE_MAX_GAIN_INCREMENT))
      {
        *pGain = *pGain < curGain ? (curGain < AE_MAX_GAIN_INCREMENT ? 0 : curGain - AE_MAX_GAIN_INCREMENT) : curGain + AE_MAX_GAIN_INCREMENT;
      }
      *pGain = (*pGain < pSensorInfo->gain_min) ? pSensorInfo->gain_min : (*pGain > pSensorInfo->gain_max) ? pSensorInfo->gain_max : *pGain;
    }
  }
  else
  {

    /**** COARSE CONVERGENCE is reached ****/
    /* Check if fine convergence processing is required to reach high accuracy */

    /* Coarse convergence reached, refine convergence */
    if (((lux > AE_LOW_LUX_LIMIT) && (abs((int32_t)averageL - (int32_t)IQParamConfig->AECAlgo.exposureTarget) > AE_FINE_TOLERANCE)) ||
        ((lux <= AE_LOW_LUX_LIMIT) && (abs((int32_t)averageL - (int32_t)IQParamConfig->AECAlgo.exposureTarget) > AE_FINE_TOLERANCE_LOW_LUX(IQParamConfig->AECAlgo.exposureTarget))))
    {
      /**** FINE CONVERGENCE not reached ****/

      if (averageL < IQParamConfig->AECAlgo.exposureTarget)
      {
        /* Slightly increase exposure */
        if (cur_global_exposure <= pSensorInfo->exposure_max)
        {
          new_global_exposure = averageL ? cur_global_exposure * (double)IQParamConfig->AECAlgo.exposureTarget / averageL : cur_global_exposure + AE_EXPOSURE_FINE_INCREMENT;
        }
        else
        {
          new_global_exposure = pSensorInfo->exposure_max * pow(10, ((double)curGain + AE_GAIN_FINE_INCREMENT) / 20000);
        }
      }
      else
      {
        /* Slightly decrease exposure */
        if (cur_global_exposure <= pSensorInfo->exposure_max)
        {
          new_global_exposure = averageL ? cur_global_exposure * (double)IQParamConfig->AECAlgo.exposureTarget / averageL : cur_global_exposure - AE_EXPOSURE_FINE_DECREMENT;
        }
        else
        {
          new_global_exposure = pSensorInfo->exposure_max * pow(10, ((double)curGain - AE_GAIN_FINE_DECREMENT) / 20000);
        }
      }

      /* Clamp and split exposure into exposure value and gain */
      if (new_global_exposure <= pSensorInfo->exposure_max)
      {
        *pGain = 0;
        *pExposure = (new_global_exposure < pSensorInfo->exposure_min) ? pSensorInfo->exposure_min : (uint32_t)new_global_exposure;
      }
      else
      {
        *pExposure = pSensorInfo->exposure_max;
        *pGain = (uint32_t)(20 * 1000 * log10((float)new_global_exposure / (float)(*pExposure)));

        *pGain = (*pGain < pSensorInfo->gain_min) ? pSensorInfo->gain_min : (*pGain > pSensorInfo->gain_max) ? pSensorInfo->gain_max : *pGain;
      }
    }
    else
    {
      /**** FINE CONVERGENCE is reached ****/
      /* Converged */
      *pExposure = curExposure;
      *pGain = curGain;
    }
  }

  /**** Consider flickering period constraint ****/
  /* ONLY USEFUL WHEN WHEN ANTI-FLICKER IS ACTIVATED */
  isp_ae_compute_antiflcker(*pGain, *pExposure, &curGain, &curExposure);

  /* Return final value of sensor exposure time and gain */
  *pExposure = curExposure;
  *pGain = curGain;

  /* Store lux value to avoid making the same exposure estimation if it did not allow to reach convergence */
  previous_lux = lux;
}
