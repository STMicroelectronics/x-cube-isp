/**
 ******************************************************************************
 * @file    isp_conf.c
 * @author  AIS Application Team
 * @brief   DCMIPP event callback implementation
  *         This template should be copied to the user folder,
  *         renamed and customized following user needs.
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

/* Let Camera Middleware handle this for simple preview application */
/* Includes ------------------------------------------------------------------*/
#include "isp_api.h"
#include "app.h"
#include "cmw_camera.h"

/* Private types -------------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern ISP_HandleTypeDef hIsp;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Vsync Event callback on pipe
 * @param  Pipe  Pipe receiving the callback
 * @retval None
 */
int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t Pipe)
{
  /* Update the frame counter and call the ISP statistics handler */
  if (Pipe == DCMIPP_PIPE1 )
  {
    /* Call the IPS statistics handler */
    ISP_IncMainFrameId(&hIsp);
    ISP_GatherStatistics(&hIsp);
    ISP_OutputMeta(&hIsp);
  }
  /* Additional applicative processing can be implemented from here */
  return 0;
}

/**
 * @brief  Frame Event callback on pipe
 * @param  Pipe  Pipe receiving the callback
 * @retval None
 */
int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t Pipe)
{
  main_PreviewPipeFrameEvent(Pipe);

  /* Update the frame counter for the pipes used to dump the frame */
  switch (Pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&hIsp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&hIsp);
      break;
    default:
      break;
  }
  /* Additional applicative processing can be implemented from here */
  return 0;
}
