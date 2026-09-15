/**
 ******************************************************************************
 * @file    utils.h
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

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void ComputePreviewSize(uint32_t cameraWidth, uint32_t cameraHeight, uint32_t maxPreviewWidth, uint32_t maxPreviewHeight,
                        uint32_t *previewWidth, uint32_t *previewHeight);

#endif
