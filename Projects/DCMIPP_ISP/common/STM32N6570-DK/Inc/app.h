/**
 ******************************************************************************
 * @file    app.h
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

#ifndef APP_H
#define APP_H

#include <stdint.h>

void App_Run(void);
void main_PreviewPipeFrameEvent(uint32_t pipe);

#endif
