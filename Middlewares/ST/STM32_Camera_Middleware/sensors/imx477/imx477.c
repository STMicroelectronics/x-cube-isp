/**
  ******************************************************************************
  * @file    imx477.c
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

#include "imx477.h"

#include <assert.h>
#include <stddef.h>

#define SENSOR_REG_LEN(_l_) ((_l_) << 16)

#define REG8(_r_)   ((_r_) | SENSOR_REG_LEN(8))
#define REG16(_r_)  ((_r_) | SENSOR_REG_LEN(16))
#define REG32(_r_)  ((_r_) | SENSOR_REG_LEN(32))

#define CCI_REG_ADDR(_r_) ((_r_) & 0xffff)
#define CCI_REG_TYPE(_r_) ((_r_) >> 16)

#define IMX477_REG_CHIP_ID            REG16(0x0016)
  #define IMX477_CHIP_ID              0x0477
#define IMX477_REG_MODE_SELECT        REG8(0x0100)
  #define IMX477_MODE_STANDBY         0x00
  #define IMX477_MODE_STREAMING       0x01
#define IMX477_REG_ORIENTATION        REG8(0x101)
#define IMX477_REG_CSI_DT_FMT_H       REG8(0x0112)
#define IMX477_REG_CSI_DT_FMT_L       REG8(0x0113)
#define IMX477_REG_EXPOSURE           REG16(0x0202)
  #define IMX477_EXPOSURE_OFFSET      22
  #define IMX477_EXPOSURE_MIN         5
  #define IMX477_EXPOSURE_MAX         (IMX477_FRAME_LENGTH_MAX - IMX477_EXPOSURE_OFFSET)
#define IMX477_REG_ANALOG_GAIN        REG16(0x0204)
#define IMX477_REG_DIGITAL_GAIN       REG16(0x020e)
#define IMX477_REG_IOP_PXCK_DIV       REG8(0x0309)
#define IMX477_REG_IOP_SYSCK_DIV      REG8(0x030b)
  #define IMX477_IOP_SYSCK_DIV        0x02
#define IMX477_REG_IOP_PREDIV         REG8(0x030d)
  #define IMX477_IOP_PREDIV           0x02
#define IMX477_REG_IOP_MPY            REG16(0x030e)
#define IMX477_REG_FRAME_LENGTH       REG16(0x0340)
  #define IMX477_VBLANK_MIN           48
  #define IMX477_FRAME_LENGTH_MAX     0xffdc
#define IMX477_REG_LINE_LENGTH        REG16(0x0342)
  #define IMX477_LINE_LENGTH_MAX      0xfff0
#define IMX477_REG_TEST_PATTERN       REG16(0x0600)
  #define IMX477_TEST_PATTERN_DISABLE     0
  #define IMX477_TEST_PATTERN_SOLID_COLOR 1
  #define IMX477_TEST_PATTERN_COLOR_BARS  2
  #define IMX477_TEST_PATTERN_GREY_COLOR  3
  #define IMX477_TEST_PATTERN_PN9         4
#define IMX477_REG_DPHY_CTRL          REG8(0x0808)
  #define IMX477_DPHY_CTRL_AUTO       0
  #define IMX477_DPHY_CTRL_UI         1
  #define IMX477_DPHY_CTRL_REGISTER   2
#define IMX477_REG_REQ_LINK_BIT_RATE  REG32(0x0820)
#define IMX477_REG_ADBIT_MODE         REG8(0x3f0d)

#define IMX477_PIXEL_RATE             840000000
#define IMX477_XCLK_FREQ              24000000
#define IMX477_BPP                    (10)

#define IMX477_DEFAULT_LINE_LENGTH_LINE_NB  (3*8400)


#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define IMX477_TraceError(_ctx_,_ret_) do { \
  if (_ret_) IMX477_error(_ctx_, "Error on %s:%d : %d\n", __func__, __LINE__, _ret_); \
  if (_ret_) return _ret_; \
} while(0)

#include "imx477_regs.c"

static void IMX477_dbg(IMX477_Ctx_t *ctx, int lvl, const char *format, ...)
{
  va_list ap;

  if (!ctx->log)
    return ;

  va_start(ap, format);
  ctx->log(ctx, IMX477_LVL_DBG(lvl), format, ap);
  va_end(ap);
}

static void IMX477_notice(IMX477_Ctx_t *ctx, const char *format, ...)
{
  va_list ap;

  if (!ctx->log)
    return ;

  va_start(ap, format);
  ctx->log(ctx, IMX477_LVL_NOTICE, format, ap);
  va_end(ap);
}

static void IMX477_warn(IMX477_Ctx_t *ctx, const char *format, ...)
{
  va_list ap;

  if (!ctx->log)
    return ;

  va_start(ap, format);
  ctx->log(ctx, IMX477_LVL_WARNING, format, ap);
  va_end(ap);
}

static void IMX477_error(IMX477_Ctx_t *ctx, const char *format, ...)
{
  va_list ap;

  if (!ctx->log)
    return ;

  va_start(ap, format);
  ctx->log(ctx, IMX477_LVL_ERROR, format, ap);
  va_end(ap);
}

static int IMX477_Read(IMX477_Ctx_t *ctx, uint32_t addr, uint32_t *value)
{
  int type = CCI_REG_TYPE(addr);
  uint32_t reg32;
  uint16_t reg16;
  uint8_t reg8;
  int ret;

  switch(type) {
  case 8:
    ret = ctx->read8(ctx, CCI_REG_ADDR(addr), &reg8);
    *value = reg8;
    break;
  case 16:
    ret = ctx->read16(ctx, CCI_REG_ADDR(addr), &reg16);
    *value = reg16;
    break;
  case 32:
    ret = ctx->read32(ctx, CCI_REG_ADDR(addr), &reg32);
    *value = reg32;
    break;
  default:
    ret = -1;
    assert(0);
  }

  IMX477_dbg(ctx, 2, "read%d : 0x%04x => 0x%08x (%d) / %d\n", type, CCI_REG_ADDR(addr), *value, *value, ret);

  return ret;
}

static int IMX477_Write(IMX477_Ctx_t *ctx, uint32_t addr, uint32_t value)
{
  int type = CCI_REG_TYPE(addr);
  int ret;

  switch(type) {
  case 8:
    ret = ctx->write8(ctx, CCI_REG_ADDR(addr), value);
    break;
  case 16:
    ret = ctx->write16(ctx, CCI_REG_ADDR(addr), value);
    break;
  case 32:
    ret = ctx->write32(ctx, CCI_REG_ADDR(addr), value);
    break;
  default:
    ret = -1;
    assert(0);
  }

  IMX477_dbg(ctx, 2, "write%d : 0x%04x => 0x%08x (%d) / %d\n", type, CCI_REG_ADDR(addr), value, value, ret);

  return ret;
}

static int IMX477_width(IMX477_Ctx_t *ctx)
{
  IMX477_Res_t res = ctx->ctx.config_save.resolution;

  switch(res) {
  case IMX477_RES_4056_3040:
    return 4056;
  case IMX477_RES_2028_1520:
    return 2028;
  default:
    return 0;
  }
}

static int IMX477_height(IMX477_Ctx_t *ctx)
{
  IMX477_Res_t res = ctx->ctx.config_save.resolution;

  switch(res) {
  case IMX477_RES_4056_3040:
    return 3040;
  case IMX477_RES_2028_1520:
    return 1520;
  default:
    return 0;
  }
}

static int IMX477_line_length(IMX477_Ctx_t *ctx)
{
  if (!ctx->ctx.line_length)
    return IMX477_DEFAULT_LINE_LENGTH_LINE_NB;

  return ctx->ctx.line_length;
}

static int IMX477_line_length_us(IMX477_Ctx_t *ctx)
{
  int line_length = IMX477_line_length(ctx);

  return line_length / (IMX477_PIXEL_RATE / 1000000);
}

static int IMX477_is_resolution_valid(IMX477_Res_t res)
{
  switch(res) {
  case IMX477_RES_4056_3040:
  case IMX477_RES_2028_1520:
    return 1;
  default:
    return 0;
  }
}

static int IMX477_is_link_freq_valid(IMX477_LinkFreq_t link_freq)
{
  switch(link_freq) {
  case IMX477_LINK_300M:
  case IMX477_LINK_450M:
  case IMX477_LINK_600M:
  case IMX477_LINK_750M:
  case IMX477_LINK_900M:
  case IMX477_LINK_1200M:
    return 1;
  default:
    return 0;
  }
}

static int IMX477_is_fps_valid(int frame_rate)
{
  return (frame_rate >= IMX477_MIN_FPS) && (frame_rate <= IMX477_MAX_FPS);
}

static int IMX477_is_flip_mirror_mode_valid(IMX477_MirrorFlip_t flip_mirror_mode)
{
  switch(flip_mirror_mode) {
  case IMX477_MIRROR_FLIP_NONE:
  case IMX477_FLIP:
  case IMX477_MIRROR:
  case IMX477_MIRROR_FLIP:
    return 1;
  default:
    return 0;
  }
}

static int IMX477_is_patgen_valid(IMX477_PatGen_t patgen)
{
  switch(patgen) {
  case IMX477_PATGEN_DISABLE:
  case IMX477_PATGEN_SOLID_COLOR:
  case IMX477_PATGEN_COLOR_BARS:
  case IMX477_PATGEN_GREY_COLOR:
    return 1;
  default:
    return 0;
  }
}

static int IMX477_is_config_valid(IMX477_Config_t *config)
{
  if (!IMX477_is_resolution_valid(config->resolution))
    return 0;
  if (!IMX477_is_link_freq_valid(config->link_freq))
    return 0;
  if (!IMX477_is_fps_valid(config->frame_rate))
    return 0;
  if (!IMX477_is_flip_mirror_mode_valid(config->flip_mirror_mode))
    return 0;
  if (!IMX477_is_patgen_valid(config->patgen))
    return 0;

  return 1;
}

static int IMX477_is_id_correct(IMX477_Ctx_t *ctx)
{
  uint32_t id = 0;
  int ret;

  ret = IMX477_Read(ctx, IMX477_REG_CHIP_ID, &id);
  IMX477_dbg(ctx, 0, "ret = %d / id : read 0x%04x / expected 0x%04x\n", ret, id, IMX477_CHIP_ID);

  return (ret == 0) && (id == IMX477_CHIP_ID);
}

static int IMX477_set_common_regs(IMX477_Ctx_t *ctx)
{
  int ret;
  int i;

  for (i = 0; i < ARRAY_SIZE(imx477_mode_common_regs); i++)
  {
    ret = IMX477_Write(ctx, REG8(imx477_mode_common_regs[i][0]), imx477_mode_common_regs[i][1]);
    IMX477_TraceError(ctx, ret);
  }

  IMX477_notice(ctx, "common regs written\n");

  return 0;
}

static int IMX477_get_link_pll_multiplier(IMX477_Ctx_t *ctx, uint32_t link_freq)
{
  uint64_t mpy = (uint64_t)link_freq * 2 * IMX477_IOP_SYSCK_DIV * IMX477_IOP_PREDIV;
  uint64_t tmp;

  mpy = mpy / IMX477_XCLK_FREQ;

  tmp = mpy * (IMX477_XCLK_FREQ / IMX477_IOP_PREDIV);
  tmp = tmp / (IMX477_IOP_SYSCK_DIV * 2);

  if (tmp != link_freq)
    return -1;

  return mpy;
}

static int IMX477_set_datalink_bitrate(IMX477_Ctx_t *ctx)
{
  uint32_t link_freq = ctx->ctx.config_save.link_freq;
  int iop_pll_mpy;
  int ret;

  /* Update the link frequency PLL multiplier register */
  iop_pll_mpy = IMX477_get_link_pll_multiplier(ctx, link_freq);
  IMX477_notice(ctx, "iop_pll_mpy = %d\n", iop_pll_mpy);
  if (iop_pll_mpy <= 0)
    return -1;

  ret = IMX477_Write(ctx, IMX477_REG_IOP_MPY, iop_pll_mpy);
  IMX477_TraceError(ctx, ret);

  /* Bit rate = link freq * 2 for DDR * 2 for num lanes. */
  /* Fmt is data rate in Mbps in fixed point 16.16 */
  ret = IMX477_Write(ctx, IMX477_REG_REQ_LINK_BIT_RATE, ((link_freq / 1000000) * 2 * 2) << 16);
  IMX477_TraceError(ctx, ret);

  IMX477_notice(ctx, "set link rate to %d Mbps\n", link_freq / 1000000);

  /* Set DPHY timings in auto */
  ret = IMX477_Write(ctx, IMX477_REG_DPHY_CTRL, IMX477_DPHY_CTRL_AUTO);
  IMX477_TraceError(ctx, ret);

  /* Update line_length to use */
  ctx->ctx.line_length = (IMX477_width(ctx) * IMX477_BPP * (uint64_t)IMX477_PIXEL_RATE) / (2 * 2 * (uint64_t)link_freq);
  ctx->ctx.line_length += 500;

  IMX477_notice(ctx, "set line length to %d clk (%d us) \n", ctx->ctx.line_length, IMX477_line_length_us(ctx));

  return 0;
}

static int *IMX477_get_mode_array(IMX477_Res_t res, int *array_len, char **mode_name)
{
  switch(res) {
  case IMX477_RES_4056_3040:
    *array_len = ARRAY_SIZE(imx477_mode_4056x3040_regs);
    *mode_name = "4056x3040";
    return (int *) imx477_mode_4056x3040_regs;
  case IMX477_RES_2028_1520:
    *array_len = ARRAY_SIZE(imx477_mode_2028x1520_regs);
    *mode_name = "2028x1520";
    return (int *) imx477_mode_2028x1520_regs;
  default:
    *array_len = 0;
    assert(0);
  }

  return NULL;
}

static int IMX477_set_mode_regs(IMX477_Ctx_t *ctx)
{
  char *mode_name;
  int array_len;
  int *array;
  int ret;
  int i;

  array = IMX477_get_mode_array(ctx->ctx.config_save.resolution, &array_len, &mode_name);
  if (!array)
    return -1;

  for (i = 0; i < array_len; i++)
  {
    ret = IMX477_Write(ctx, REG8(array[2 * i + 0]), array[2 * i + 1]);
    IMX477_TraceError(ctx, ret);
  }

  IMX477_notice(ctx, "mode %s regs written\n", mode_name);

  return 0;
}

static int IMX477_set_bpp(IMX477_Ctx_t *ctx)
{
  const int bpp = IMX477_BPP;
  int ret;

  ret = IMX477_Write(ctx, IMX477_REG_CSI_DT_FMT_H, bpp);
  IMX477_TraceError(ctx, ret);

  ret = IMX477_Write(ctx, IMX477_REG_CSI_DT_FMT_L, bpp);
  IMX477_TraceError(ctx, ret);

  ret = IMX477_Write(ctx, IMX477_REG_IOP_PXCK_DIV, bpp);
  IMX477_TraceError(ctx, ret);

  ret = IMX477_Write(ctx, IMX477_REG_ADBIT_MODE, bpp == 12 ? 1 : 0);
  IMX477_TraceError(ctx, ret);

  return 0;
}

static int IMX477_set_flip_mode(IMX477_Ctx_t *ctx)
{
  struct drv_imx477_ctx *drv_ctx = &ctx->ctx;
  int hflip = 0;
  int vflip = 0;
  int ret;

  switch (drv_ctx->config_save.flip_mirror_mode) {
  case IMX477_MIRROR_FLIP_NONE:
    hflip = 0;
    vflip = 0;
    break;
  case IMX477_FLIP:
    hflip = 0;
    vflip = 1;
    break;
  case IMX477_MIRROR:
    hflip = 1;
    vflip = 0;
    break;
  case IMX477_MIRROR_FLIP:
    hflip = 1;
    vflip = 1;
    break;
  default:
    assert(0);
  }

  ret = IMX477_Write(ctx, IMX477_REG_ORIENTATION, hflip | (vflip << 1));
  IMX477_TraceError(ctx, ret);

  return 0;
}

static int IMX477_set_patgen_mode(IMX477_Ctx_t *ctx)
{
  struct drv_imx477_ctx *drv_ctx = &ctx->ctx;
  uint16_t patgen_mode;
  int ret;

  switch (drv_ctx->config_save.patgen) {
  case IMX477_PATGEN_DISABLE:
    patgen_mode = IMX477_TEST_PATTERN_DISABLE;
    break;
  case IMX477_PATGEN_SOLID_COLOR:
    patgen_mode = IMX477_TEST_PATTERN_SOLID_COLOR;
    break;
  case IMX477_PATGEN_COLOR_BARS:
    patgen_mode = IMX477_TEST_PATTERN_COLOR_BARS;
    break;
  case IMX477_PATGEN_GREY_COLOR:
    patgen_mode = IMX477_TEST_PATTERN_GREY_COLOR;
    break;
  default:
    patgen_mode = IMX477_TEST_PATTERN_DISABLE;
    assert(0);
  }

  ret =  IMX477_Write(ctx, IMX477_REG_TEST_PATTERN, patgen_mode);
  IMX477_TraceError(ctx, ret);

  return 0;
}

static int IMX477_set_fps(IMX477_Ctx_t *ctx)
{
  const uint16_t line_length = IMX477_line_length(ctx);
  int frame_rate = ctx->ctx.config_save.frame_rate;
  uint32_t frame_length;
  int frame_rate_set;
  int ret;

  /* compute frame length to reach fps */
  frame_length = IMX477_PIXEL_RATE / (frame_rate * line_length);
  /* But clamp to safe values */
  if (frame_length > IMX477_FRAME_LENGTH_MAX)
    frame_length = IMX477_FRAME_LENGTH_MAX;
  if (frame_length < IMX477_height(ctx) + IMX477_VBLANK_MIN)
    frame_length = IMX477_height(ctx) + IMX477_VBLANK_MIN;
  frame_rate_set = IMX477_PIXEL_RATE / (frame_length * line_length);

  IMX477_notice(ctx, "Set frame_length to %d for fps = %d\n", frame_length, frame_rate);
  if (frame_rate_set != frame_rate)
    IMX477_warn(ctx, " !!! fps set to %d fps\n", frame_rate_set);

  ret = IMX477_Write(ctx, IMX477_REG_LINE_LENGTH, line_length);
  IMX477_TraceError(ctx, ret);

  ret = IMX477_Write(ctx, IMX477_REG_FRAME_LENGTH, frame_length);
  IMX477_TraceError(ctx, ret);

  ctx->ctx.max_expo_line_nb = frame_length - IMX477_EXPOSURE_OFFSET;

  return 0;
}

static int IMX477_Setup(IMX477_Ctx_t *ctx)
{
  int ret;

  ret = IMX477_set_mode_regs(ctx);
  if (ret)
    return -1;

  ret = IMX477_set_bpp(ctx);
  if (ret)
    return -1;

  ret = IMX477_set_flip_mode(ctx);
  if (ret)
    return -1;

  ret = IMX477_set_patgen_mode(ctx);
  if (ret)
    return -1;

  return 0;
}

static int IMX477_set_state(IMX477_Ctx_t *ctx, int state)
{
  int ret;

  ret = IMX477_Write(ctx, IMX477_REG_MODE_SELECT, state);
  IMX477_TraceError(ctx, ret);

  return 0;
}

static int IMX477_set_bayer(IMX477_Ctx_t *ctx)
{
  struct drv_imx477_ctx *drv_ctx = &ctx->ctx;

  switch (drv_ctx->config_save.flip_mirror_mode) {
  case IMX477_MIRROR_FLIP_NONE:
    ctx->bayer = IMX477_BAYER_RGGB;
    break;
  case IMX477_FLIP:
    ctx->bayer = IMX477_BAYER_GBRG;
    break;
  case IMX477_MIRROR:
    ctx->bayer = IMX477_BAYER_GRBG;
    break;
  case IMX477_MIRROR_FLIP:
    ctx->bayer = IMX477_BAYER_BGGR;
    break;
  default:
    ctx->bayer = IMX477_BAYER_RGGB;
    assert(0);
  }

  return 0;
}

static int IMX477_expo_in_us_to_line(IMX477_Ctx_t *ctx, int expo_in_us)
{
  int line_length_us = IMX477_line_length_us(ctx);
  int expo_in_line = (expo_in_us + line_length_us / 2) / line_length_us;

  if (expo_in_line < IMX477_EXPOSURE_MIN)
    return -1;

  if (expo_in_line > ctx->ctx.max_expo_line_nb)
    return -1;

  return expo_in_line;
}

static int IMX477_expo_in_line_to_expo_in_us(IMX477_Ctx_t *ctx, int expo_in_line)
{
  int expo_in_us = expo_in_line * IMX477_line_length_us(ctx);

  return expo_in_us;
}

/* Public API */
int IMX477_Init(IMX477_Ctx_t *ctx, IMX477_Config_t *config)
{
  struct drv_imx477_ctx *drv_ctx = &ctx->ctx;
  int ret;

  assert(ctx);
  assert(config);

  ret = IMX477_is_config_valid(config);
  if (!ret)
    return -1;

  /* power off - on sequence */
  ctx->shutdown_pin(ctx, 0);
  ctx->delay(ctx, 10);
  ctx->shutdown_pin(ctx, 1);
  ctx->delay(ctx, 300);

  drv_ctx->config_save = *config;
  drv_ctx->is_streaming = 0;

  ret = IMX477_is_id_correct(ctx);
  if (!ret)
    goto deinit;

  ret = IMX477_set_common_regs(ctx);
  if (ret)
    goto deinit;

  ret = IMX477_set_datalink_bitrate(ctx);
  if (ret)
    goto deinit;

  ret = IMX477_set_bayer(ctx);
  if (ret)
    goto deinit;

  ret = IMX477_set_fps(ctx);
  if (ret)
    goto deinit;

  ret = IMX477_SetAnalogGain(ctx, IMX477_ANALOG_GAIN_MIN);
  if (ret)
    goto deinit;

  ret = IMX477_SetDigitalGain(ctx, IMX477_DIGITAL_GAIN_MIN);
  if (ret)
    goto deinit;

  return 0;

deinit:
  IMX477_DeInit(ctx);

  return -1;
}

int IMX477_DeInit(IMX477_Ctx_t *ctx)
{
  struct drv_imx477_ctx *drv_ctx = &ctx->ctx;

  if (drv_ctx->is_streaming)
    return -1;

  /* Turn off power */
  ctx->shutdown_pin(ctx, 0);
  ctx->delay(ctx, 1);

  return 0;
}

int IMX477_Start(IMX477_Ctx_t *ctx)
{
  struct drv_imx477_ctx *drv_ctx = &ctx->ctx;
  int ret;

  if (drv_ctx->is_streaming) {
    IMX477_warn(ctx, "Already start\n");
    return 0;
  }

  ret =IMX477_Setup(ctx);
  if (ret)
    return ret;

  /* let's go */
  ret = IMX477_set_state(ctx, IMX477_MODE_STREAMING);
  if (ret)
    return -1;

  drv_ctx->is_streaming = 1;

  IMX477_notice(ctx, "streaming active\n");

  return 0;
}

int IMX477_Stop(IMX477_Ctx_t *ctx)
{
  struct drv_imx477_ctx *drv_ctx = &ctx->ctx;
  int ret;

  if (!drv_ctx->is_streaming) {
    IMX477_warn(ctx, "Already stop\n");
    return 0;
  }

  ret = IMX477_set_state(ctx, IMX477_MODE_STANDBY);
  if (ret)
    return -1;

  drv_ctx->is_streaming = 0;

  IMX477_notice(ctx, "streaming inactive\n");

  return 0;
}

int IMX477_SetAnalogGain(IMX477_Ctx_t *ctx, unsigned int gain)
{
  int ret;

  IMX477_notice(ctx, "Set analog gain to 0x%04x\n", gain);

  if (gain > IMX477_ANALOG_GAIN_MAX)
    return -1;

  ret = IMX477_Write(ctx, IMX477_REG_ANALOG_GAIN, gain);
  IMX477_TraceError(ctx, ret);

  return 0;
}

int IMX477_GetAnalogGain(IMX477_Ctx_t *ctx, unsigned int *gain)
{
  uint32_t reg16;
  int ret;

  ret = IMX477_Read(ctx, IMX477_REG_ANALOG_GAIN, &reg16);
  IMX477_TraceError(ctx, ret);
  *gain = reg16;

  return 0;
}

int IMX477_SetDigitalGain(IMX477_Ctx_t *ctx, unsigned int gain)
{
  int ret;

  IMX477_notice(ctx, "Set digital gain to 0x%04x\n", gain);

  if (gain < IMX477_DIGITAL_GAIN_MIN)
    return -1;

  if (gain > IMX477_DIGITAL_GAIN_MAX)
    return -1;


  ret = IMX477_Write(ctx, IMX477_REG_DIGITAL_GAIN, gain);
  IMX477_TraceError(ctx, ret);

  return 0;
}


int IMX477_GetDigitalGain(IMX477_Ctx_t *ctx, unsigned int *gain)
{
  uint32_t reg16;
  int ret;

  ret = IMX477_Read(ctx, IMX477_REG_DIGITAL_GAIN, &reg16);
  IMX477_TraceError(ctx, ret);
  *gain = reg16;

  return 0;
}

int IMX477_SetExpo(IMX477_Ctx_t *ctx, unsigned int expo_in_us)
{
  int expo_line_nb;
  int ret;

  expo_line_nb = IMX477_expo_in_us_to_line(ctx, expo_in_us);
  if (expo_line_nb < 0)
    return -1;

  IMX477_notice(ctx, "Set expo to %d us => %d lines\n", expo_in_us, expo_line_nb);

  ret = IMX477_Write(ctx, IMX477_REG_EXPOSURE, expo_line_nb);
  IMX477_TraceError(ctx, ret);

  return 0;
}

int IMX477_GetExpo(IMX477_Ctx_t *ctx, unsigned int *expo_in_us)
{
  uint32_t reg16;
  int ret;

  ret = IMX477_Read(ctx, IMX477_REG_EXPOSURE, &reg16);
  IMX477_TraceError(ctx, ret);

  *expo_in_us = IMX477_expo_in_line_to_expo_in_us(ctx, reg16);

  return 0;
}

int IMX477_GetExposureRange(IMX477_Ctx_t *ctx, unsigned int *min_us, unsigned int *max_us)
{
  *min_us = IMX477_expo_in_line_to_expo_in_us(ctx, IMX477_EXPOSURE_MIN);
  *max_us = IMX477_expo_in_line_to_expo_in_us(ctx, ctx->ctx.max_expo_line_nb);

  return 0;
}
