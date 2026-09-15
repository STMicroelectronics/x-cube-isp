/**
 ******************************************************************************
 * @file    dump_spinner.c
 * @author  AIS Application Team
 * @brief   Implement functions for spinner display during full resolution dump
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

#include "dump_spinner.h"
#include "main.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SPINNER_RADIUS     24
#define SPINNER_THICKNESS  5

#define SPINNER_ARC_DEG    240
#define SPINNER_STEP_DEG   10
#define SPINNER_PERIOD_MS  50U

#define Y_BLACK            0x00
#define Y_WHITE            0xEB
#define UV_NEUTRAL         0x80

static uint8_t *g_dump_spinner_buf = 0;
static uint32_t g_dump_spinner_enabled = 0U;
static uint32_t g_dump_spinner_last_tick = 0U;
static uint32_t g_dump_spinner_width = 0U;
static uint32_t g_dump_spinner_height = 0U;
static uint32_t g_dump_spinner_pitch = 0U;
static uint32_t g_dump_spinner_frame_size = 0U;
static uint32_t g_spinner_center_x = DUMP_SPINNER_WIDTH / 2U;
static uint32_t g_spinner_center_y = DUMP_SPINNER_HEIGHT / 2U;
static int g_spinner_head_deg = 0;

static uint32_t DumpSpinner_AlignedSize(uint32_t size)
{
    return (size + 31U) & ~31U;
}

static uint8_t DumpSpinner_IsConfigValid(uint8_t *buf, uint32_t width, uint32_t height, uint32_t pitch)
{
    if ((buf == 0) ||
        (width == 0U) || (height == 0U) ||
        (width > DUMP_SPINNER_WIDTH) || (height > DUMP_SPINNER_HEIGHT) ||
        (pitch < (width * 2U))) {
        return 0U;
    }

    if ((pitch * height) > DUMP_SPINNER_FRAME_SIZE) {
        return 0U;
    }

    return 1U;
}

static void DumpSpinner_UpdateContext(uint8_t *buf, uint32_t width, uint32_t height, uint32_t pitch)
{
    const uint32_t margin = (uint32_t)(SPINNER_RADIUS + 2);

    g_dump_spinner_buf = buf;
    g_dump_spinner_width = width;
    g_dump_spinner_height = height;
    g_dump_spinner_pitch = pitch;
    g_dump_spinner_frame_size = pitch * height;

    if (width > (2U * margin)) {
        const uint32_t min_x = margin;
        const uint32_t max_x = width - 1U - margin;

        if (g_spinner_center_x < min_x) {
            g_spinner_center_x = min_x;
        }
        if (g_spinner_center_x > max_x) {
            g_spinner_center_x = max_x;
        }
    } else {
        g_spinner_center_x = width / 2U;
    }

    if (height > (2U * margin)) {
        const uint32_t min_y = margin;
        const uint32_t max_y = height - 1U - margin;

        if (g_spinner_center_y < min_y) {
            g_spinner_center_y = min_y;
        }
        if (g_spinner_center_y > max_y) {
            g_spinner_center_y = max_y;
        }
    } else {
        g_spinner_center_y = height / 2U;
    }
}

static void DumpSpinner_CleanDCache(void)
{
    if ((g_dump_spinner_buf == 0) || (g_dump_spinner_frame_size == 0U)) {
        return;
    }

    /* LTDC reads from memory directly: push CPU updates to RAM before next scanout. */
    SCB_CleanDCache_by_Addr(g_dump_spinner_buf, (int32_t)DumpSpinner_AlignedSize(g_dump_spinner_frame_size));
}

/* ---------- Low level framebuffer access ---------- */

static void DumpSpinner_SetPixelY(uint8_t *buf, int x, int y, uint8_t yval)
{
    uint32_t pair_x;
    uint32_t idx;

    if ((buf == 0) ||
        (x < 0) || ((uint32_t)x >= g_dump_spinner_width) ||
        (y < 0) || ((uint32_t)y >= g_dump_spinner_height)) {
        return;
    }

    pair_x = (uint32_t)x & ~1U;
    idx = ((uint32_t)y * g_dump_spinner_pitch) + (pair_x * 2U);

    if ((x & 1) == 0) {
        buf[idx + 0] = yval;
    } else {
        buf[idx + 2] = yval;
    }

    buf[idx + 1] = UV_NEUTRAL;
    buf[idx + 3] = UV_NEUTRAL;
}

static void DumpSpinner_FillBlack(uint8_t *buf)
{
    uint16_t black_word;

    if (buf == 0) {
        return;
    }

    black_word = ((uint16_t)UV_NEUTRAL << 8) | (uint16_t)Y_BLACK; /* 0x8000 */

    for (uint32_t y = 0; y < g_dump_spinner_height; y++) {
        uint16_t *row = (uint16_t *)(buf + (y * g_dump_spinner_pitch));
        uint32_t words = g_dump_spinner_pitch / sizeof(uint16_t);

        for (uint32_t x = 0; x < words; x++) {
            row[x] = black_word;
        }
    }
}

static void DumpSpinner_ClearArcArea(uint8_t *buf)
{
    const int spinner_cx = (int)g_spinner_center_x;
    const int spinner_cy = (int)g_spinner_center_y;
    const int r_outer = SPINNER_RADIUS;
    const int xmin = spinner_cx - r_outer - 2;
    const int xmax = spinner_cx + r_outer + 2;
    const int ymin = spinner_cy - r_outer - 2;
    const int ymax = spinner_cy + r_outer + 2;

    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            const int dx = x - spinner_cx;
            const int dy = y - spinner_cy;
            const int d2 = dx * dx + dy * dy;

            if (d2 <= (r_outer * r_outer)) {
                DumpSpinner_SetPixelY(buf, x, y, Y_BLACK);
            }
        }
    }
}

static float DumpSpinner_NormalizeDeg(float a)
{
    while (a < 0.0f) {
        a += 360.0f;
    }
    while (a >= 360.0f) {
        a -= 360.0f;
    }
    return a;
}

static float DumpSpinner_TrailDistanceDeg(float a, float h)
{
    float d = h - a;
    while (d < 0.0f) {
        d += 360.0f;
    }
    while (d >= 360.0f) {
        d -= 360.0f;
    }
    return d;
}

static void DumpSpinner_DrawArc(uint8_t *buf, int head_deg, uint8_t erase_only)
{
    const int spinner_cx = (int)g_spinner_center_x;
    const int spinner_cy = (int)g_spinner_center_y;
    const int r_outer = SPINNER_RADIUS;
    const int r_inner = SPINNER_RADIUS - SPINNER_THICKNESS;
    const int xmin = spinner_cx - r_outer - 2;
    const int xmax = spinner_cx + r_outer + 2;
    const int ymin = spinner_cy - r_outer - 2;
    const int ymax = spinner_cy + r_outer + 2;

    const float head = DumpSpinner_NormalizeDeg((float)head_deg);

    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            const int dx = x - spinner_cx;
            const int dy_screen = y - spinner_cy;
            const int d2 = dx * dx + dy_screen * dy_screen;

            if ((d2 <= (r_outer * r_outer)) && (d2 >= (r_inner * r_inner))) {
                float angle_deg;
                float trail_deg;

                angle_deg = (float)(atan2((double)(-dy_screen), (double)dx) * 180.0 / M_PI);
                angle_deg = DumpSpinner_NormalizeDeg(angle_deg);

                trail_deg = DumpSpinner_TrailDistanceDeg(angle_deg, head);

                if (trail_deg <= (float)SPINNER_ARC_DEG) {
                    uint8_t yval;

                    if (erase_only != 0U) {
                        yval = Y_BLACK;
                    } else {
                        const float ratio = 1.0f - (trail_deg / (float)SPINNER_ARC_DEG);
                        const float gamma = ratio * ratio;
                        const int y_min = 0x35;
                        const int y_max = Y_WHITE;

                        yval = (uint8_t)(y_min + (int)((y_max - y_min) * gamma));

                        if (trail_deg < 18.0f) {
                            yval = Y_WHITE;
                        }
                    }

                    DumpSpinner_SetPixelY(buf, x, y, yval);
                }
            }
        }
    }
}

/* ---------- Public API ---------- */

void DumpSpinner_Start(uint8_t *buf, uint32_t width, uint32_t height, uint32_t pitch)
{
    if (DumpSpinner_IsConfigValid(buf, width, height, pitch) == 0U) {
        return;
    }

    DumpSpinner_UpdateContext(buf, width, height, pitch);
    g_spinner_head_deg = 0;
    g_dump_spinner_last_tick = HAL_GetTick() - SPINNER_PERIOD_MS;

    /* Preserve static logo/background drawn at init; only clear/redraw spinner arc area. */
    DumpSpinner_ClearArcArea(buf);
    DumpSpinner_DrawArc(buf, g_spinner_head_deg, 0U);
    DumpSpinner_CleanDCache();

    g_dump_spinner_enabled = 1U;
    DumpSpinner_Task();
}

void DumpSpinner_InitBuffer(uint8_t *buf, uint32_t width, uint32_t height, uint32_t pitch)
{
    if (DumpSpinner_IsConfigValid(buf, width, height, pitch) == 0U) {
        return;
    }

    DumpSpinner_UpdateContext(buf, width, height, pitch);
    DumpSpinner_FillBlack(buf);
    DumpSpinner_CleanDCache();
}

void DumpSpinner_SetCenter(uint32_t x, uint32_t y)
{
    g_spinner_center_x = x;
    g_spinner_center_y = y;
}

void DumpSpinner_DrawLogo(uint32_t x0, uint32_t y0, const uint8_t *argb8888, uint32_t width, uint32_t height)
{
    if ((g_dump_spinner_buf == 0) || (argb8888 == 0) || (width == 0U) || (height == 0U)) {
        return;
    }

    for (uint32_t j = 0; j < height; j++) {
        for (uint32_t i = 0; i < width; i++) {
            const uint32_t dst_x = x0 + i;
            const uint32_t dst_y = y0 + j;
            const uint8_t *src = &argb8888[(j * width + i) * 4U];
            const uint8_t alpha = src[3];

            if ((dst_x >= g_dump_spinner_width) || (dst_y >= g_dump_spinner_height) || (alpha == 0U)) {
                continue;
            }

            /* Logo is white in ARGB8888, blend alpha over black YUV background. */
            uint8_t yval = (uint8_t)(((uint32_t)Y_WHITE * (uint32_t)alpha + 127U) / 255U);
            DumpSpinner_SetPixelY(g_dump_spinner_buf, (int)dst_x, (int)dst_y, yval);
        }
    }

    DumpSpinner_CleanDCache();
}

void DumpSpinner_Stop(void)
{
    g_dump_spinner_enabled = 0U;
}

void DumpSpinner_Task(void)
{
    uint32_t now;

    if ((g_dump_spinner_enabled == 0U) || (g_dump_spinner_buf == 0)) {
        return;
    }

    now = HAL_GetTick();

    if ((now - g_dump_spinner_last_tick) < SPINNER_PERIOD_MS) {
        return;
    }

    g_dump_spinner_last_tick = now;

    DumpSpinner_DrawArc(g_dump_spinner_buf, g_spinner_head_deg, 1U);

    g_spinner_head_deg += SPINNER_STEP_DEG;
    if (g_spinner_head_deg >= 360) {
        g_spinner_head_deg -= 360;
    }

    DumpSpinner_DrawArc(g_dump_spinner_buf, g_spinner_head_deg, 0U);
    DumpSpinner_CleanDCache();
}
