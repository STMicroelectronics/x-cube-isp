/**
  ******************************************************************************
  * @file    stm32n6xx_it.c
  * @author  AIS Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_hal.h"
#include "stm32n6xx_it.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern DCMIPP_HandleTypeDef hcamera_dcmipp;
extern LTDC_HandleTypeDef   hlcd_ltdc;
extern PCD_HandleTypeDef    usbx_pcd_handle;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M55 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Secure Fault exception.
  * @param  None
  * @retval None
  */
void SecureFault_Handler(void)
{
  /* Go to infinite loop when Secure Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
  while (1)
  {
  }
}

/******************************************************************************/
/*                 STM32N6xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32n6xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles DCMIPP Handler.
  * @param  None
  * @retval None
  */
void CSI_IRQHandler(void)
{
  HAL_DCMIPP_CSI_IRQHandler(&hcamera_dcmipp);
}

/**
  * @brief  This function handles DCMIPP interrupt request.
  * @param  None
  * @retval None
  */
void DCMIPP_IRQHandler(void)
{
  HAL_DCMIPP_IRQHandler(&hcamera_dcmipp);
}

/**
  * @brief  This function handles LTDC low-layer interrupt request.
  * @param  None
  * @retval None
  */
void LTDC_LO_IRQHandler(void)
{
  HAL_LTDC_IRQHandler(&hlcd_ltdc);
}

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
/**
  * @brief  This function handles USB Device (event buffer) interrupt request.
  * @param  None
  * @retval None
  */
void USB1_OTG_HS_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&usbx_pcd_handle);
}
#endif
