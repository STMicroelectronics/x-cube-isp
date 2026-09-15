/**
 ******************************************************************************
 * @file    utils.c
 * @author  AIS Application Team
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "utils.h"

/**
  * @brief  Get the preview size to respect aspect ratio of the camera.
  * @param  cameraWidth      width of the camera frame
  * @param  cameraHeight     height of the camera frame
  * @param  maxPreviewWidth  max possible value for previewWidth
  * @param  maxPreviewHeight max possible value for previewHeight
  * @param  previewWidth     pointer on the calculated preview width
  * @param  previewHeight    pointer on the calculated preview height
  */
void ComputePreviewSize(uint32_t cameraWidth, uint32_t cameraHeight, uint32_t maxPreviewWidth, uint32_t maxPreviewHeight,
                        uint32_t *previewWidth, uint32_t *previewHeight)
{
  /* Camera frame aspect ratio */
  float aspectRatio = (float)cameraWidth / (float)cameraHeight;

  /* Keep camera aspect ratio */
  if (aspectRatio > 1.0f)
  { /* Landscape case */
    *previewWidth = maxPreviewWidth;
    *previewHeight = (uint32_t)(maxPreviewWidth / aspectRatio);
    if (*previewHeight > maxPreviewHeight)
    {
      *previewHeight = maxPreviewHeight;
      *previewWidth = (uint32_t)(maxPreviewHeight * aspectRatio);
    }
  }
  else
  { /* Portrait case */
    *previewHeight = maxPreviewHeight;
    *previewWidth = (uint32_t)(maxPreviewHeight * aspectRatio);
    if (*previewWidth > maxPreviewHeight)
    {
      *previewWidth = maxPreviewWidth;
      *previewHeight = (uint32_t)(maxPreviewWidth / aspectRatio);
    }
  }

  /* Ensure previewWidth is a multiple of 8, rounded down and not greater than previous value */
  *previewWidth &= (uint32_t)~7u;

  /* Adjust previewHeight to keep aspect ratio after width rounding */
  *previewHeight = (uint32_t)((float)(*previewWidth) / aspectRatio);

  /* Ensure previewHeight is even */
  if (*previewHeight & 1)
    *previewHeight -= 1;
}
