/**
 ******************************************************************************
 * @file    dump_spinner.h
 * @author  AIS Application Team
 * @brief   Header file of dump_spinner.c
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

#ifndef DUMP_SPINNER_MODULE_H
#define DUMP_SPINNER_MODULE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Keep spinner buffer bounded to internal RAM budget. */
#define DUMP_SPINNER_WIDTH       640U
#define DUMP_SPINNER_HEIGHT      480U
#define DUMP_SPINNER_FRAME_SIZE  (DUMP_SPINNER_WIDTH * DUMP_SPINNER_HEIGHT * 2U)

void DumpSpinner_Start(uint8_t *buf, uint32_t width, uint32_t height, uint32_t pitch);
void DumpSpinner_InitBuffer(uint8_t *buf, uint32_t width, uint32_t height, uint32_t pitch);
void DumpSpinner_SetCenter(uint32_t x, uint32_t y);
void DumpSpinner_DrawLogo(uint32_t x0, uint32_t y0, const uint8_t *argb8888, uint32_t width, uint32_t height);
void DumpSpinner_Stop(void);
void DumpSpinner_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* DUMP_SPINNER_MODULE_H */
