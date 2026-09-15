/**
  ******************************************************************************
  * @file    imx477.h
  * @author  AIS Application Team
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

#ifndef IMX477_H
#define IMX477_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

#define IMX477_LVL_ERROR 0
#define IMX477_LVL_WARNING 1
#define IMX477_LVL_NOTICE 2
#define IMX477_LVL_DBG(l) (3 + (l))

/* Sensor native resolution */
#define IMX477_MAX_WIDTH                                  4056
#define IMX477_MAX_HEIGHT                                 3040

/* Analog gain. gain = 1024 / (1024 - again_reg) */
#define IMX477_ANALOG_GAIN_MIN                            0
#define IMX477_ANALOG_GAIN_MAX                            978
/* Digital gain [1.00, 255.00] is coded as a Fixed Point 8.8
 * which corresponds to sensor values in the range [0x0100, 0xff00] */
#define IMX477_DIGITAL_GAIN_MIN                          0x100
#define IMX477_DIGITAL_GAIN_MAX                          0xff00

typedef enum {
  IMX477_RES_4056_3040 = 0,
  IMX477_RES_2028_1520 = 1,
} IMX477_Res_t;

typedef enum {
  IMX477_BAYER_RGGB,
  IMX477_BAYER_GRBG,
  IMX477_BAYER_GBRG,
  IMX477_BAYER_BGGR,
} IMX477_BayerType_t;

typedef enum {
  IMX477_MIRROR_FLIP_NONE,
  IMX477_FLIP,
  IMX477_MIRROR,
  IMX477_MIRROR_FLIP
} IMX477_MirrorFlip_t;

typedef enum {
  IMX477_PATGEN_DISABLE,
  IMX477_PATGEN_SOLID_COLOR,
  IMX477_PATGEN_COLOR_BARS,
  IMX477_PATGEN_GREY_COLOR,
} IMX477_PatGen_t;

enum {
  IMX477_MIN_FPS = 1,
  IMX477_MAX_FPS = 30,
};

/* This is clock frequency. Data bitrate per lane is twice this value due to ddr */
typedef enum {
  IMX477_LINK_300M  =  300000000,
  IMX477_LINK_450M  =  450000000,
  IMX477_LINK_600M  =  600000000,
  IMX477_LINK_750M  =  750000000,
  IMX477_LINK_900M  =  900000000,
  IMX477_LINK_1200M = 1200000000,
} IMX477_LinkFreq_t;

typedef struct {
  IMX477_Res_t resolution;
  int frame_rate;
  IMX477_LinkFreq_t link_freq;
  IMX477_MirrorFlip_t flip_mirror_mode;
  IMX477_PatGen_t patgen;
} IMX477_Config_t;

typedef struct IMX477_Ctx
{
  /* API client must set these values */
  void (*shutdown_pin)(struct IMX477_Ctx *ctx, int value);
  int (*read8)(struct IMX477_Ctx *ctx, uint16_t addr, uint8_t *value);
  int (*read16)(struct IMX477_Ctx *ctx, uint16_t addr, uint16_t *value);
  int (*read32)(struct IMX477_Ctx *ctx, uint16_t addr, uint32_t *value);
  int (*write8)(struct IMX477_Ctx *ctx, uint16_t addr, uint8_t value);
  int (*write16)(struct IMX477_Ctx *ctx, uint16_t addr, uint16_t value);
  int (*write32)(struct IMX477_Ctx *ctx, uint16_t addr, uint32_t value);
  int (*write_array)(struct IMX477_Ctx *ctx, uint16_t addr, uint8_t *data, int data_len);
  void (*delay)(struct IMX477_Ctx *ctx, uint32_t delay_in_ms);
  void (*log)(struct IMX477_Ctx *ctx, int lvl, const char *format, va_list ap);
  /* driver fill those values on IMX477_Init */
  IMX477_BayerType_t bayer;
  /* driver internals */
  struct drv_imx477_ctx {
    int is_streaming;
    int max_expo_line_nb;
    int line_length;
    IMX477_Config_t config_save;
  } ctx;
} IMX477_Ctx_t;

int IMX477_Init(IMX477_Ctx_t *ctx, IMX477_Config_t *config);
int IMX477_DeInit(IMX477_Ctx_t *ctx);
int IMX477_Start(IMX477_Ctx_t *ctx);
int IMX477_Stop(IMX477_Ctx_t *ctx);
/* Again valid in [IMX477_ANALOG_GAIN_MIN .. IMX477_ANALOG_GAIN_MAX] */
int IMX477_SetAnalogGain(IMX477_Ctx_t *ctx, unsigned int gain);
int IMX477_GetAnalogGain(IMX477_Ctx_t *ctx, unsigned int *gain);
/* Again valid in [IMX477_DIGITAL_GAIN_MIN .. IMX477_DIGITAL_GAIN_MAX] */
int IMX477_SetDigitalGain(IMX477_Ctx_t *ctx, unsigned int gain);
int IMX477_GetDigitalGain(IMX477_Ctx_t *ctx, unsigned int *gain);
int IMX477_SetExpo(IMX477_Ctx_t *ctx, unsigned int expo_in_us);
int IMX477_GetExpo(IMX477_Ctx_t *ctx, unsigned int *expo_in_us);
int IMX477_GetExposureRange(IMX477_Ctx_t *ctx, unsigned int *min_us, unsigned int *max_us);

#ifdef __cplusplus
}
#endif

#endif /* IMX477_H */
