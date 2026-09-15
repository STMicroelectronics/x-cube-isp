/**
 ******************************************************************************
 * @file    app.c
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

#include "app.h"

#include <stdint.h>
#include <stdio.h>

#include "cmw_camera.h"
#include "isp_api.h"
#include "isp_services.h"
#include "isp_param_conf.h"
#include "main.h"
#include "stm32n6xx_hal.h"
#include "stm32n6570_discovery_lcd.h"
#include "utils.h"

#include "ispiqtune_logo_argb8888.h"

#include "tx_api.h"

#include "dump_spinner.h"

#ifdef ISP_ENABLE_UVC
#include "usbx_conf.h"
#endif /* ISP_ENABLE_UVC */

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* LCD Size */
#define LCD_WIDTH         RK050HR18_WIDTH
#define LCD_HEIGHT        RK050HR18_HEIGHT

/* Pixel Format Size */
#define BPP_RAW8       1
#define BPP_RAW10      2
#define BPP_RAW12      2
#define BPP_RAW14      2
#define BPP_RGB565     2
#define BPP_YUV422     2
#define BPP_RGB888     3
#define BPP_ARGB8888   4

/* Preview frame buffer */
#define MAX_PREVIEW_BUFFER_WIDTH    640
#define MAX_PREVIEW_BUFFER_HEIGHT   480

/* Dump frame buffer */
/* Size is adjusted for a maximum of 12MP image in dump pipe */
/* If sensor resolution is higher than 5MP, decimation is applied in the two other pipes */
#define MAX_DUMP_BUFFER_BYTES (12 * 1024 * 1024 * BPP_RAW14)

#define CAMERA_FLIP CMW_MIRRORFLIP_NONE /* CMW_MIRRORFLIP_NONE or CMW_MIRRORFLIP_FLIP or CMW_MIRRORFLIP_MIRROR or CMW_MIRRORFLIP_FLIP_MIRROR */

typedef struct
{
  char name[32];             /*!< Sensor name                                      */
  uint32_t CamImgWidth;      /*!< Sensor output width                              */
  uint32_t CamImgHeight;     /*!< Sensor output height                             */
  uint32_t PreviewWidth;     /*!< DCMIPP output width for preview                  */
  uint32_t PreviewHeight;    /*!< DCMIPP output height for preview                 */
  uint32_t SensorDataType;   /*!< CSI2 Datatype to be set for flow selection DTIDA */
  uint32_t BytePerPixel;     /*!< Pixel format size                                */
} CAMERA_SensorConfTypeDef;

typedef struct
{
  uint32_t X0;
  uint32_t Y0;
  uint32_t XSize;
  uint32_t YSize;
} Rectangle_TypeDef;

DCMIPP_HandleTypeDef *phDcmipp;
ISP_HandleTypeDef    hIsp;

static TX_THREAD spinner_thread;
static uint8_t spinner_thread_stack[1024];

static uint32_t DisplayFramePitch;

/* Camera_SensorConf will be filled when Camera_Config is called */
static CAMERA_SensorConfTypeDef Camera_SensorConf = {0};

static uint32_t preview_layer_x0 = 0;
static uint32_t preview_layer_y0 = 0;
/* LCD full display area */
static Rectangle_TypeDef display_area = {
  .X0 = 0,
  .Y0 = 0,
  .XSize = LCD_WIDTH,
  .YSize = LCD_HEIGHT,
};

static Rectangle_TypeDef stat_area = {0};
static uint8_t DecimationFactor = 1;

static int buffer_disp_idx = 1;

static volatile uint32_t preview_frame_count = 0;
static volatile uint32_t preview_ltdc_swap_enabled = 1;

/* Frame buffers */
/* Allocate the Main_DestBuffer (YUV422) in SRAM dedicated region */
__attribute__ ((section(".psram_bss")))
__attribute__ ((aligned (32)))
static uint8_t Main_DestBuffer[2][MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT * BPP_YUV422];
__attribute__ ((section (".psram_bss")))
__attribute__ ((aligned (32)))
static uint8_t Dump_DestBuffer[MAX_DUMP_BUFFER_BYTES];
extern uint32_t ISP_AncillaryPipe_FrameCount;
extern uint32_t ISP_DumpPipe_FrameCount;
__attribute__ ((section (".psram_bss")))
__attribute__ ((aligned (32)))
static uint8_t Overlay_Buffer[LCD_WIDTH * LCD_HEIGHT * BPP_ARGB8888];
__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
/* Keep spinner in internal SRAM so dump-mode UI refresh never reads XSPI/PSRAM. */
static uint8_t dump_spinner[DUMP_SPINNER_FRAME_SIZE];

static uint8_t *GetPreviewBuffer(void)
{
  return Main_DestBuffer[buffer_disp_idx];
}

/**
  * @brief  Configure the FIFO size and outsanting capability for IPPlug client of each pipe
  * @param  Pipe0_OW   Output width of dump pipe
  * @param  Pipe0_PFS  Pixel format size of dump pipe
  * @param  Pipe1_OW   Output width of main pipe
  * @param  Pipe1_PFS  Pixel format size of main pipe
  * @param  Pipe2_OW   Maximum output width of ancillary pipe
  * @param  Pipe2_PFS  Pixel format size of the maximum dump width on ancillary pipe
  * @retval ISP status operation
  */
static ISP_StatusTypeDef SetIPPlugConf(uint32_t Pipe0_OW, uint32_t Pipe0_PFS,
                                uint32_t Pipe1_OW, uint32_t Pipe1_PFS,
                                uint32_t Pipe2_OW, uint32_t Pipe2_PFS)
{
  uint32_t Client[DCMIPP_NUM_OF_PIPES] = {DCMIPP_CLIENT1,DCMIPP_CLIENT2,DCMIPP_CLIENT5}; /* AXI master client for Dump Pipe, Main Pipe in RGB and Ancillary pipe*/
  uint32_t PBP[DCMIPP_NUM_OF_PIPES] = {Pipe0_OW * Pipe0_PFS, Pipe1_OW * Pipe1_PFS, Pipe2_OW * Pipe2_PFS}; /* Peak Bandwidth proportion for each pipe*/
  uint32_t PBP_ALL = PBP[0] + PBP[1] + PBP[2];
  DCMIPP_IPPlugConfTypeDef IPPlugCfg = {0};
  uint8_t i;

  IPPlugCfg.MemoryPageSize = DCMIPP_MEMORY_PAGE_SIZE_256BYTES;
  IPPlugCfg.Traffic = DCMIPP_TRAFFIC_BURST_SIZE_128BYTES;
  IPPlugCfg.WLRURatio = 10;

  /* Configure IPPlug for each pipe */
  for (i = 0; i < DCMIPP_NUM_OF_PIPES; i++)
  {
    IPPlugCfg.Client = Client[i];
    IPPlugCfg.MaxOutstandingTransactions = ((16 * PBP[i]/PBP_ALL) >= 1)? (16 * PBP[i]/PBP_ALL) - 1 : 0;
    IPPlugCfg.DPREGStart = IPPlugCfg.DPREGEnd? IPPlugCfg.DPREGEnd + 1 : 0;
    IPPlugCfg.DPREGEnd = MIN((IPPlugCfg.DPREGStart + 640 * PBP[i]/PBP_ALL), 639);

    if (HAL_DCMIPP_SetIPPlugConfig(hIsp.hDcmipp, &IPPlugCfg) != HAL_OK)
    {
      return ISP_ERR_DCMIPP_CONFIGPIPE;
    }
  }

  return ISP_OK;
}

/**
  * @brief  Set ipplug configuration for standard operation mode
  * @retval ISP status operation
  */
static int SetIPPlugDefaultConf()
{
    return SetIPPlugConf(0, 0,
                         Camera_SensorConf.PreviewWidth, BPP_YUV422,
                         Camera_SensorConf.PreviewWidth, BPP_RGB888);
}

/**
  * @brief  Configure the DCMIPP pipe2 for Live streaming dump
  */
static void DCMIPP_Pipe2Config_DumpLiveStreaming(void)
{
  CMW_DCMIPP_Conf_t dcmipp_conf = {0};
  uint32_t pitch;

  /* Configure Ancillary pipe with downscaling for Live streaming snapshot */
  dcmipp_conf.output_width = Camera_SensorConf.PreviewWidth;
  dcmipp_conf.output_height = Camera_SensorConf.PreviewHeight;
  dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1;
  dcmipp_conf.output_bpp = BPP_RGB888;
  dcmipp_conf.enable_swap = 0;
  dcmipp_conf.enable_gamma_conversion = -1; /* Let the ISP config file to decide */
  dcmipp_conf.mode = CMW_Aspect_ratio_fit;
  dcmipp_conf.isp_decimation_ratio_h = DecimationFactor;
  dcmipp_conf.isp_decimation_ratio_v = DecimationFactor;

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE2, &dcmipp_conf, &pitch) != 0)
  {
    printf("ERROR: can't configure pipe2 for Live streaming dump\r\n");
    Error_Handler();
  }
}

/**
  * @brief  Configure the DCMIPP pipe0 to dump native raw bayer from sensor
  */
static void DCMIPP_Pipe0Config_DumpSensorRaw(void)
{
  CMW_DCMIPP_Conf_t dcmipp_conf = {0};
  uint32_t pitch;

  /* Configure the dump pipe with the sensor full size */
  dcmipp_conf.output_width = Camera_SensorConf.CamImgWidth;
  dcmipp_conf.output_height = Camera_SensorConf.CamImgHeight;
  dcmipp_conf.isp_decimation_ratio_h = 1; // No decimation on raw bayer image
  dcmipp_conf.isp_decimation_ratio_v = 1; // No decimation on raw bayer image

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE0, &dcmipp_conf, &pitch) != 0)
  {
    printf("ERROR: can't configure pipe0\r\n");
    Error_Handler();
  }
}

/**
  * @brief  Draw IQTune Logo
  * @param  FramebufferAddress Buffer address
  * @param  DisplayArea        Coordinates of the display
  * @param  LogoArea           Coordinates of the logo area
  * @retval none
  */
static void Draw_ARGB8888_Logo(uint32_t FramebufferAddress, Rectangle_TypeDef DisplayArea, Rectangle_TypeDef LogoArea)
{
  uint32_t bpp = BPP_ARGB8888;
  unsigned char *buf;

  buf = (unsigned char *)FramebufferAddress;
  buf += DisplayArea.XSize * bpp * LogoArea.Y0 + LogoArea.X0 * bpp;
  for (unsigned int j = 0; j < LogoArea.YSize; j++)
    for (unsigned int i = 0; i < LogoArea.XSize; i++)
      ((uint32_t *)buf)[(j* DisplayArea.XSize) + i] = ((uint32_t *)ISPIQTune_logo_argb8888)[(j* LogoArea.XSize) + i];
}

/**
  * @brief  Display the statistic area on the preview
  * @param  void
  * @retval void
  */
static void Display_Logo(void)
{
  Rectangle_TypeDef LogoArea = {
    .X0 = (display_area.XSize - ISPIQTune_logo_argb8888_width - Camera_SensorConf.PreviewWidth) / 2,
    .Y0 = (display_area.YSize - ISPIQTune_logo_argb8888_height) / 2,
    .XSize = ISPIQTune_logo_argb8888_width,
    .YSize = ISPIQTune_logo_argb8888_height,
  };

  if (HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, (uint32_t) Overlay_Buffer, LTDC_LAYER_2) != HAL_OK)
  {
    printf("ERROR: Failed to update the logo area on the preview\r\n");
  }

  Draw_ARGB8888_Logo((uint32_t) Overlay_Buffer, display_area, LogoArea);

  if (HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, LTDC_LAYER_2) != HAL_OK)
  {
    printf("ERROR: Failed to display the logo area on the preview\r\n");
  }
}

/**
  * @brief  Draw stat window as a dotted rectangle
  * @param  FramebufferAddress Buffer address
  * @param  DisplayArea        Coordinates of the display
  * @param  StatArea           Coordinates of the stat area
  * @retval none
  */
static void Draw_ARGB8888_StatArea(uint32_t FramebufferAddress, Rectangle_TypeDef DisplayArea, Rectangle_TypeDef StatArea)
{
  uint32_t bpp = BPP_ARGB8888;
  uint32_t color = 0xFFFF0000; /* Red*/
  unsigned char *buf;

  Rectangle_TypeDef PreviewArea = {
    .X0 = (display_area.XSize - Camera_SensorConf.PreviewWidth),
    .Y0 = (display_area.YSize - Camera_SensorConf.PreviewHeight),
    .XSize = Camera_SensorConf.PreviewWidth,
    .YSize = Camera_SensorConf.PreviewHeight,
  };

  buf = (unsigned char *)FramebufferAddress;
  /* Erase previous statistic area (preview part only) */
  buf += (PreviewArea.X0 + DisplayArea.XSize * PreviewArea.Y0) * bpp;
  memset(buf, 0x00, PreviewArea.XSize * bpp);
  for (unsigned int i = PreviewArea.Y0; i < (PreviewArea.Y0 + PreviewArea.YSize); i++) {
    buf += DisplayArea.XSize * bpp;
    memset(buf, 0x00, PreviewArea.XSize * bpp);
  }

  /* Draw top horizontal line */
  buf = (unsigned char *)FramebufferAddress;
  buf += (PreviewArea.X0 + StatArea.X0 + (PreviewArea.Y0 + StatArea.Y0) * DisplayArea.XSize) * bpp;
  for (unsigned int i = 0; i < StatArea.XSize; i++)
    if ((i / 5) % 2)
      ((uint32_t *)buf)[i] = color;

  /* Draw vertical lines */
  for (unsigned int i = 0; i < StatArea.YSize ; i++) {
    if ((i / 5) % 2)
    {
      ((uint32_t *)buf)[0] = color;
      if (StatArea.XSize > 0)
      {
        ((uint32_t *)buf)[StatArea.XSize - 1] = color;
      }
    }
    buf += DisplayArea.XSize * bpp;
  }

  /* Draw bottom horizontal line */
  buf -= DisplayArea.XSize * bpp;
  for (unsigned int i = 0; i < StatArea.XSize; i++)
    if ((i / 5) % 2)
      ((uint32_t *)buf)[i] = color;
}

/**
  * @brief  Display the statistic area on the preview
  * @param  void
  * @retval void
  */
static void Display_StatArea(void)
{
  Rectangle_TypeDef ScaledStatArea = {
    .X0 = (stat_area.X0 * Camera_SensorConf.PreviewWidth) / (Camera_SensorConf.CamImgWidth / DecimationFactor),
    .Y0 = (stat_area.Y0 * Camera_SensorConf.PreviewHeight) / (Camera_SensorConf.CamImgHeight / DecimationFactor),
    .XSize = (stat_area.XSize * Camera_SensorConf.PreviewWidth) / (Camera_SensorConf.CamImgWidth / DecimationFactor),
    .YSize = (stat_area.YSize * Camera_SensorConf.PreviewHeight) / (Camera_SensorConf.CamImgHeight / DecimationFactor),
  };

  if (HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, (uint32_t) Overlay_Buffer, LTDC_LAYER_2) != HAL_OK)
  {
    printf("ERROR: Failed to update the statistic area on the preview\r\n");
  }

  Draw_ARGB8888_StatArea((uint32_t) Overlay_Buffer, display_area, ScaledStatArea);

  if (HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, LTDC_LAYER_2) != HAL_OK)
  {
    printf("ERROR: Failed to display the statistic area on the preview\r\n");
  }
}

static HAL_StatusTypeDef LTDC_Layer1_SetPitchBytes_NoReload(uint32_t pitch_bytes)
{
  uint32_t fmt = hlcd_ltdc.LayerCfg[LTDC_LAYER_1].PixelFormat;

  /* LTDC pitch setter uses pixel units. Force 1-byte format so pixel == byte for exact pitch programming. */
  hlcd_ltdc.LayerCfg[LTDC_LAYER_1].PixelFormat = LTDC_PIXEL_FORMAT_L8;
  HAL_StatusTypeDef status = HAL_LTDC_SetPitch_NoReload(&hlcd_ltdc, pitch_bytes, LTDC_LAYER_1);
  hlcd_ltdc.LayerCfg[LTDC_LAYER_1].PixelFormat = fmt;

  return status;
}

/**
  * @brief  Configure the DCMIPP pipe2 for ISP pipeline RGB888 dump
  */
static void DCMIPP_Pipe2Config_DumpIspRGB888(void)
{
  CMW_DCMIPP_Conf_t dcmipp_conf = {0};
  uint32_t pitch;

  /* Configure Ancillary pipe for ISP output dump at full resolution (with decimation applied if any) */
  dcmipp_conf.output_width = Camera_SensorConf.CamImgWidth / DecimationFactor;
  dcmipp_conf.output_height = Camera_SensorConf.CamImgHeight / DecimationFactor;
  dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1;
  dcmipp_conf.output_bpp = BPP_RGB888;
  dcmipp_conf.enable_swap = 0;
  dcmipp_conf.enable_gamma_conversion = -1; /* Let the ISP config file to decide */
  dcmipp_conf.mode = CMW_Aspect_ratio_fit;
  dcmipp_conf.isp_decimation_ratio_h = DecimationFactor;
  dcmipp_conf.isp_decimation_ratio_v = DecimationFactor;

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE2, &dcmipp_conf, &pitch) != 0)
  {
    printf("ERROR: can't configure pipe2 for ISP RGB888 dump\r\n");
    Error_Handler();
  }
}

/**
  * @brief  Get the sensor info
  * @param  camera_instance: camera instance
  * @param  info: sensor info
  * @retval Operation result
  */
static ISP_StatusTypeDef GetSensorInfo(uint32_t camera_instance, ISP_SensorInfoTypeDef *info)
{
  UNUSED(camera_instance);
  CMW_Sensor_Info_t sensor_info;

  if (CMW_CAMERA_GetSensorInfo(&sensor_info) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORINFO;

  /* Convert from cmw typedef to isp typedef */
  strncpy(info->name, sensor_info.name, sizeof(info->name) - 1);
  info->name[sizeof(info->name) - 1] = '\0';
  info->bayer_pattern = sensor_info.bayer_pattern;
  info->color_depth = sensor_info.color_depth;
  info->width = sensor_info.width;
  info->height = sensor_info.height;
  info->gain_min = sensor_info.gain_min;
  info->gain_max = sensor_info.gain_max;
  info->again_max = sensor_info.again_max;
  info->exposure_min = sensor_info.exposure_min;
  info->exposure_max = sensor_info.exposure_max;

  return ISP_OK;
}

/**
  * @brief  Set the sensor gain
  * @param  camera_instance: camera instance
  * @param  gain: sensor gain to be applied
  * @retval Operation result
  */
static ISP_StatusTypeDef SetSensorGain(uint32_t camera_instance, int32_t gain)
{
  UNUSED(camera_instance);

  if (CMW_CAMERA_SetGain(gain) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORGAIN;

  return ISP_OK;
}

/**
  * @brief  Get the sensor gain
  * @param  camera_instance: camera instance
  * @param  gain: current sensor gain
  * @retval Operation result
  */
static ISP_StatusTypeDef GetSensorGain(uint32_t camera_instance, int32_t *gain)
{
  UNUSED(camera_instance);

  if (CMW_CAMERA_GetGain(gain) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORGAIN;

  return ISP_OK;
}

/**
  * @brief  Set the sensor exposure
  * @param  Instance: camera instance
  * @param  Gain: sensor exposure to be applied
  * @retval Operation result
  */
static ISP_StatusTypeDef SetSensorExposure(uint32_t camera_instance, int32_t exposure)
{
  UNUSED(camera_instance);

  if (CMW_CAMERA_SetExposure(exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}

/**
  * @brief  Get the sensor exposure
  * @param  camera_instance: camera instance
  * @param  gain: current sensor gain
  * @retval Operation result
  */
static ISP_StatusTypeDef GetSensorExposure(uint32_t camera_instance, int32_t *exposure)
{
  UNUSED(camera_instance);

  if (CMW_CAMERA_GetExposure(exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}

/**
  * @brief  Helper for ISP to start camera preview
  * @param  hDcmipp Pointer to the dcmipp device
  * @retval ISP status operation
  */
static ISP_StatusTypeDef Camera_StartPreview(void *pDcmipp)
{
  DCMIPP_HandleTypeDef *pHdcmipp = (DCMIPP_HandleTypeDef*)pDcmipp;

  /* Check handle validity */
  if (pHdcmipp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  if (pHdcmipp->PipeState[DCMIPP_PIPE1] == HAL_DCMIPP_PIPE_STATE_BUSY)
  {
    /* Already started */
    return ISP_OK;
  }

  if ((pHdcmipp->ErrorCode != 0) || (pHdcmipp->State != HAL_DCMIPP_STATE_READY))
  {
    /* Overrun run may occur just before the pipeline is started: clear the error */
    printf("WARNING: Overrun detected before preview start\r\n");
    pHdcmipp->ErrorCode = 0;
    pHdcmipp->State = HAL_DCMIPP_STATE_READY;
  }

  if (CMW_CAMERA_Resume(DCMIPP_PIPE1) != CMW_ERROR_NONE)
  {
    return ISP_ERR_DCMIPP_START;
  }

  /* Keep layer1 on spinner until pipe1 has produced fresh frames after resume. */
  uint32_t start_frame_count = preview_frame_count;
  uint32_t timeout = 0;

  preview_ltdc_swap_enabled = 0U;
  while ((preview_frame_count - start_frame_count) < 2U)
  {
    tx_thread_sleep(1);
    timeout++;
    if (timeout > 100U)
    {
      printf("WARNING: Timeout while waiting for fresh preview frames\r\n");
      break;
    }
  }

  /* Restore layer1 geometry and source: preview was switched to full-screen spinner in stop mode. */
  if (HAL_LTDC_SetWindowSize_NoReload(&hlcd_ltdc,
                                      Camera_SensorConf.PreviewWidth,
                                      Camera_SensorConf.PreviewHeight,
                                      LTDC_LAYER_1) != HAL_OK)
  {
    printf("ERROR: Failed to restore preview layer size\r\n");
    return ISP_ERR_DCMIPP_START;
  }

  if (HAL_LTDC_SetWindowPosition_NoReload(&hlcd_ltdc, preview_layer_x0, preview_layer_y0, LTDC_LAYER_1) != HAL_OK)
  {
    printf("ERROR: Failed to restore preview layer position\r\n");
    return ISP_ERR_DCMIPP_START;
  }

  if (LTDC_Layer1_SetPitchBytes_NoReload(DisplayFramePitch) != HAL_OK)
  {
    printf("ERROR: Failed to restore preview layer pitch\r\n");
    return ISP_ERR_DCMIPP_START;
  }

  if (HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, (uint32_t)GetPreviewBuffer(), LTDC_LAYER_1) != HAL_OK)
  {
    printf("ERROR: Failed to restore preview buffer address\r\n");
    return ISP_ERR_DCMIPP_START;
  }

  if (HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, LTDC_LAYER_1) != HAL_OK)
  {
    printf("ERROR: Failed to reload preview layer\r\n");
    return ISP_ERR_DCMIPP_START;
  }

  BSP_LCD_SetLayerVisible(0,LTDC_LAYER_2, ENABLE);
  preview_ltdc_swap_enabled = 1U;
  DumpSpinner_Stop();

  return ISP_OK;
}

/**
  * @brief  Helper for ISP to stop camera preview
  * @param  hDcmipp Pointer to the dcmipp device
  * @retval ISP status operation
  */
static ISP_StatusTypeDef Camera_StopPreview(void *pDcmipp)
{
  DCMIPP_HandleTypeDef *pHdcmipp = (DCMIPP_HandleTypeDef*)pDcmipp;

  /* Check handle validity */
  if (pHdcmipp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  if (CMW_CAMERA_Suspend(DCMIPP_PIPE1) != CMW_ERROR_NONE)
  {
    return ISP_ERR_DCMIPP_STOP;
  }

  /* Disable layer2 during dump mode to avoid XSPI/PSRAM accesses that can cause OVR/artifacts. */
  BSP_LCD_SetLayerVisible(0,LTDC_LAYER_2, DISABLE);
  preview_ltdc_swap_enabled = 0U;

  /* Keep layer1 active and render spinner in place of preview while pipe1 is suspended. */
  uint32_t spinner_cx = preview_layer_x0 + (Camera_SensorConf.PreviewWidth / 2U);
  uint32_t spinner_cy = preview_layer_y0 + (Camera_SensorConf.PreviewHeight / 2U);
  /* Spinner center is expressed in display coordinates and clamped in dump_spinner.c
   * to stay drawable inside the fixed 640x480 spinner canvas. */
  DumpSpinner_SetCenter(spinner_cx, spinner_cy);
  DumpSpinner_Start(dump_spinner,
                    DUMP_SPINNER_WIDTH,
                    DUMP_SPINNER_HEIGHT,
                    DUMP_SPINNER_WIDTH * BPP_YUV422);

  if (HAL_LTDC_SetWindowSize_NoReload(&hlcd_ltdc,
                                      DUMP_SPINNER_WIDTH,
                                      DUMP_SPINNER_HEIGHT,
                                      LTDC_LAYER_1) != HAL_OK)
  {
    printf("ERROR: Failed to set spinner layer size\r\n");
    return ISP_ERR_DCMIPP_STOP;
  }

  if (HAL_LTDC_SetWindowPosition_NoReload(&hlcd_ltdc, 0U, 0U, LTDC_LAYER_1) != HAL_OK)
  {
    printf("ERROR: Failed to set spinner layer position\r\n");
    return ISP_ERR_DCMIPP_STOP;
  }

  if (LTDC_Layer1_SetPitchBytes_NoReload(DUMP_SPINNER_WIDTH * BPP_YUV422) != HAL_OK)
  {
    printf("ERROR: Failed to set spinner layer pitch\r\n");
    return ISP_ERR_DCMIPP_STOP;
  }

  if (HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, (uint32_t)dump_spinner,
                                   LTDC_LAYER_1) != HAL_OK)
  {
    printf("ERROR: Failed to update preview buffer address\r\n");
    return ISP_ERR_DCMIPP_STOP;
  }

  if (HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, LTDC_LAYER_1) != HAL_OK)
  {
    printf("ERROR: Failed to reload preview layer\r\n");
    return ISP_ERR_DCMIPP_STOP;
  }

  if ((pHdcmipp->ErrorCode != 0) || (pHdcmipp->State != HAL_DCMIPP_STATE_READY))
  {
    /* Overrun run may occur just after the pipeline is stopped: clear the error */
    printf("WARNING: Overrun detected before preview stop\r\n");
    pHdcmipp->ErrorCode = 0;
    pHdcmipp->State = HAL_DCMIPP_STATE_READY;
  }

  return ISP_OK;
}

/**
  * @brief  Helper for ISP to dump a frame
  * @param  hDcmipp Pointer to the dcmipp device
  * @param  Pipe    Pipe where to perform the dump ('DUMP'(0) or 'ANCILLARY'(2))
  * @param  Config  Dump with the current pipe config, or without downsizing with
  *                 a specific pixel format.
  * @param  pBuffer Pointer to the address of the dumped buffer (output parameter)
  * @param  pMeta   Pointer to buffer meta data (output parameter)
  * @retval ISP status operation
  */
static ISP_StatusTypeDef Camera_DumpFrame(void *pDcmipp, uint32_t Pipe, ISP_DumpCfgTypeDef Config,
                                   uint32_t **pBuffer, ISP_DumpFrameMetaTypeDef *pMeta)
{
  DCMIPP_HandleTypeDef *pHdcmipp = (DCMIPP_HandleTypeDef*)pDcmipp;
  uint32_t lastId, timeout = 0;

  /* Check handle validity */
  if ((pHdcmipp == NULL) || (pBuffer == NULL) || (pMeta == NULL))
  {
    return ISP_ERR_EINVAL;
  }

  if (Pipe == DCMIPP_PIPE2)
  {
    /* Dump on pipe2 */
    if (Config == ISP_DUMP_CFG_FULLSIZE_RGB888)
    {
      /* Configure the pipe2 for ISP RGB888 dump */
      DCMIPP_Pipe2Config_DumpIspRGB888();

      HAL_Delay(1); // Add a small delay to ensure the pipe1 is fully suspended (CPTACT == 0) before changing the IPPlug configuration
      while ((pHdcmipp->Instance->CMSR1 & DCMIPP_CMSR1_P1CPTACT) != 0U){};

      /* Maximize IPPlug Fifo size for Ancillary pipe */
      if (SetIPPlugConf(0, 0, 0, 0, Camera_SensorConf.CamImgWidth, BPP_RGB888) != ISP_OK)
      {
        return ISP_ERR_DCMIPP_CONFIGPIPE;
      }
    }
    else if (Config == ISP_DUMP_CFG_DEFAULT)
    {
      /* Configure the pipe2 for live streaming dump */
      DCMIPP_Pipe2Config_DumpLiveStreaming();
    }
    else if (Config == ISP_DUMP_CFG_DUMP_PIPE_SENSOR)
    {
      /* Not supported  on that pipe */
      return ISP_ERR_DCMIPP_CONFIGPIPE;
    }

    /* Dump frame */
    lastId = ISP_GetAncillaryFrameId(&hIsp);
    if (CMW_CAMERA_Start(Pipe, (uint8_t *) Dump_DestBuffer, CMW_CAPTUREMODE_SNAPSHOT) != HAL_OK)
    {
      return ISP_ERR_DCMIPP_START;
    }

    while (ISP_GetAncillaryFrameId(&hIsp) == lastId)
    {
      HAL_Delay(1);
      timeout++;
      if (timeout > 1000)
      {
        return ISP_ERR_DCMIPP_DUMPTIMEOUT;
      }
    }

    /* Restore IPPlug Fifo size for each pipe */
    if (SetIPPlugDefaultConf() != ISP_OK)
    {
      return ISP_ERR_DCMIPP_CONFIGPIPE;
    }

    /* Set output parameters */
    *pBuffer = (uint32_t*)Dump_DestBuffer;
    /* Get frame configuration from hardware registers (no HAL API for that) */
    pMeta->width = (pHdcmipp->Instance->P2DSSZR & DCMIPP_P2DSSZR_HSIZE) >> DCMIPP_P2DSSZR_HSIZE_Pos;
    pMeta->height = (pHdcmipp->Instance->P2DSSZR & DCMIPP_P2DSSZR_VSIZE) >> DCMIPP_P2DSSZR_VSIZE_Pos;
    pMeta->pitch = (pHdcmipp->Instance->P2PPM0PR & DCMIPP_P2PPM0PR_PITCH) >> DCMIPP_P2PPM0PR_PITCH_Pos;
    pMeta->size = pMeta->height * pMeta->pitch;
    pMeta->format = ISP_FORMAT_RGB888;
  }
  else if (Pipe == DCMIPP_PIPE0)
  {
    /* Dump on pipe0 (raw) */
    if (Config != ISP_DUMP_CFG_DUMP_PIPE_SENSOR)
    {
      /* Not supported  on that pipe */
      return ISP_ERR_DCMIPP_CONFIGPIPE;
    }

    HAL_Delay(1); // Add a small delay to ensure the pipe1 is fully suspended (CPTACT == 0) before changing the IPPlug configuration
    while ((pHdcmipp->Instance->CMSR1 & DCMIPP_CMSR1_P1CPTACT) != 0U){};

    /* Maximize IPPlug Fifo size for dump pipe */
    if (SetIPPlugConf(Camera_SensorConf.CamImgWidth, Camera_SensorConf.BytePerPixel, 0, 0, 0, 0) != ISP_OK)
    {
      return ISP_ERR_DCMIPP_CONFIGPIPE;
    }

    lastId = ISP_GetDumpFrameId(&hIsp);
    if (CMW_CAMERA_Start(Pipe, (uint8_t *) Dump_DestBuffer, CMW_CAPTUREMODE_SNAPSHOT) != HAL_OK)
    {
      return ISP_ERR_DCMIPP_START;
    }

    while (ISP_GetDumpFrameId(&hIsp) == lastId)
    {
      HAL_Delay(1);
      timeout++;
      if (timeout > 1000)
      {
        return ISP_ERR_DCMIPP_DUMPTIMEOUT;
      }
    }

    /* Set output paramerters */
    *pBuffer = (uint32_t*)Dump_DestBuffer;

    pMeta->width = Camera_SensorConf.CamImgWidth;
    pMeta->height = Camera_SensorConf.CamImgHeight;
    pMeta->pitch = pMeta->width * Camera_SensorConf.BytePerPixel;
    pMeta->size = pMeta->height * pMeta->width * Camera_SensorConf.BytePerPixel;
    pMeta->format = (ISP_FormatTypeDef) (Camera_SensorConf.SensorDataType - DCMIPP_DT_RAW8 + 1);

    /* Restore IPPlug Fifo size for each pipe */
    if (SetIPPlugDefaultConf() != ISP_OK)
    {
      return ISP_ERR_DCMIPP_CONFIGPIPE;
    }
  }
  else
  {
    /* Unsupported pipe dump */
    return ISP_ERR_DCMIPP_CONFIGPIPE;
  }

  return ISP_OK;
}

/**
  * @brief  Set the sensor test pattern
  * @param  camera_instance: camera instance
  * @param  mode: sensor test pattern to be applied
  * @retval Operation result
  */
static ISP_StatusTypeDef SetSensorTestPattern(uint32_t camera_instance, int32_t mode)
{
  UNUSED(camera_instance);

  if (CMW_CAMERA_SetTestPattern(mode) != CMW_ERROR_NONE)
    return ISP_ERR_EINVAL;

  return ISP_OK;
}

/**
  * @brief  Check if the ISP Statistic Area need to be updated
  * @retval Operation result
  */
static ISP_StatusTypeDef Check_StatAreaUpdate(void)
{
  ISP_StatusTypeDef ret;
  ISP_StatAreaTypeDef area;
  ret = ISP_GetStatArea(&hIsp, &area);
  if (ret != ISP_OK)
  {
    printf("WARNING: Not able to retrieve the current statistic area. error %d (%s)\r\n", ret, ERROR_MESSAGE(ret));
  }

  if ((stat_area.X0 == area.X0) && (stat_area.Y0 == area.Y0) && (stat_area.XSize == area.XSize) && (stat_area.YSize == area.YSize))
  {
    return ISP_OK;
  }

  /* Update current statistic area and update display layers */
  stat_area.X0 = area.X0;
  stat_area.Y0 = area.Y0;
  stat_area.XSize = area.XSize;
  stat_area.YSize = area.YSize;

  Display_StatArea();

  return ISP_OK;
}

/**
  * @brief  Initialize the LTDC with a preview area (layer1) and a statistic area (layer2)
  * @param  none
  * @retval none
  */
static void Display_Config(void)
{
  /* Preview area at the right of the display area */
  Rectangle_TypeDef PreviewArea = {
    .X0 = (display_area.XSize - Camera_SensorConf.PreviewWidth),
    .Y0 = (display_area.YSize - Camera_SensorConf.PreviewHeight),
    .XSize = Camera_SensorConf.PreviewWidth,
    .YSize = Camera_SensorConf.PreviewHeight,
  };

  preview_layer_x0 = PreviewArea.X0;
  preview_layer_y0 = PreviewArea.Y0;

  /* Initialized LTDC */
  BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);

  /* Set configuration of the LTDC Layer 1 */
  /* Initialize the buffer (black color) */
  uint32_t pixel_count = Camera_SensorConf.PreviewWidth * Camera_SensorConf.PreviewHeight;
  uint32_t buffer_size = pixel_count * BPP_YUV422;
  for (uint32_t i = 0; i < pixel_count; ++i)
  {
      ((uint16_t *)Main_DestBuffer[0])[i] = 0x8000u;
      ((uint16_t *)Main_DestBuffer[1])[i] = 0x8000u;
  }

  uint32_t spinner_cx = preview_layer_x0 + (Camera_SensorConf.PreviewWidth / 2U);
  uint32_t spinner_cy = preview_layer_y0 + (Camera_SensorConf.PreviewHeight / 2U);
  /* Keep dump-mode logo aligned with the same left strip used in normal preview. */
  uint32_t left_strip_width = (display_area.XSize > Camera_SensorConf.PreviewWidth) ?
                              (display_area.XSize - Camera_SensorConf.PreviewWidth) : 0U;
  uint32_t logo_x0 = (left_strip_width > ISPIQTune_logo_argb8888_width) ?
                     ((left_strip_width - ISPIQTune_logo_argb8888_width) / 2U) : 0U;
  uint32_t logo_y0 = (display_area.YSize > ISPIQTune_logo_argb8888_height) ?
                     ((display_area.YSize - ISPIQTune_logo_argb8888_height) / 2U) : 0U;

  /* Build a static dump-mode background once (black + logo), then only animate the spinner arc. */
  DumpSpinner_SetCenter(spinner_cx, spinner_cy);
  DumpSpinner_InitBuffer(dump_spinner,
                         DUMP_SPINNER_WIDTH,
                         DUMP_SPINNER_HEIGHT,
                         DUMP_SPINNER_WIDTH * BPP_YUV422);
  DumpSpinner_DrawLogo(logo_x0, logo_y0,
                       ISPIQTune_logo_argb8888,
                       ISPIQTune_logo_argb8888_width,
                       ISPIQTune_logo_argb8888_height);
  /* Ensure LTDC sees composed logo immediately on first stop-preview transition. */
  SCB_CleanInvalidateDCache_by_Addr (dump_spinner, (int32_t)DUMP_SPINNER_FRAME_SIZE);

  SCB_CleanInvalidateDCache_by_Addr (Main_DestBuffer[0], (int32_t)buffer_size);
  SCB_CleanInvalidateDCache_by_Addr (Main_DestBuffer[1], (int32_t)buffer_size);

  /* Configure the YUV422 preview layer */
  LTDC_LayerFlexYUVCoPlanarTypeDef pLayerFlexYUVCoP = {0};
  pLayerFlexYUVCoP.Layer.WindowX0 = PreviewArea.X0;
  pLayerFlexYUVCoP.Layer.WindowX1 = PreviewArea.X0 + PreviewArea.XSize;
  pLayerFlexYUVCoP.Layer.WindowY0 = PreviewArea.Y0;
  pLayerFlexYUVCoP.Layer.WindowY1 = PreviewArea.Y0 + PreviewArea.YSize;
  pLayerFlexYUVCoP.Layer.Alpha = LTDC_LxCACR_CONSTA;
  pLayerFlexYUVCoP.Layer.Alpha0 = 0;
  pLayerFlexYUVCoP.Layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerFlexYUVCoP.Layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerFlexYUVCoP.Layer.ImageWidth = PreviewArea.XSize;
  pLayerFlexYUVCoP.Layer.ImageHeight = PreviewArea.YSize;
  pLayerFlexYUVCoP.Layer.Backcolor.Blue = 0;
  pLayerFlexYUVCoP.Layer.Backcolor.Green = 0;
  pLayerFlexYUVCoP.Layer.Backcolor.Red = 0;
  pLayerFlexYUVCoP.FlexYUV.YUVOrder = LTDC_YUV_ORDER_LUMINANCE_FIRST;
  pLayerFlexYUVCoP.FlexYUV.LuminanceOrder = LTDC_YUV_LUMINANCE_ORDER_EVEN_FIRST;
  pLayerFlexYUVCoP.FlexYUV.ChrominanceOrder = LTDC_YUV_CHROMIANCE_ORDER_U_FIRST;
  pLayerFlexYUVCoP.FlexYUV.LuminanceRescale = LTDC_YUV_LUMINANCE_RESCALE_DISABLE;
  pLayerFlexYUVCoP.YUVAddress = (uint32_t) Main_DestBuffer;
  pLayerFlexYUVCoP.ColorConverter = LTDC_YUV2RGBCONVERTOR_BT709_FULL_RANGE;

  HAL_LTDC_ConfigLayerFlexYUVCoPlanar(&hlcd_ltdc, &pLayerFlexYUVCoP, LTDC_LAYER_1);

  /* Set pitch to match with the sensor frame pitch */
  HAL_LTDC_SetPitchInBytes(&hlcd_ltdc, DisplayFramePitch, LTDC_LAYER_1);

  /* Set configuration of the LTDC Layer 2 */
  /* Initialize the buffer */
  memset(Overlay_Buffer, 0x00, display_area.XSize * display_area.YSize * BPP_ARGB8888);

  LTDC_LayerCfgTypeDef pLayerCfg ={0};
  pLayerCfg.WindowX0 = display_area.X0;
  pLayerCfg.WindowX1 = display_area.X0 + display_area.XSize;
  pLayerCfg.WindowY0 = display_area.Y0;
  pLayerCfg.WindowY1 = display_area.Y0 + display_area.YSize;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = (uint32_t) Overlay_Buffer;
  pLayerCfg.ImageWidth = display_area.XSize;
  pLayerCfg.ImageHeight = display_area.YSize;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;

  HAL_LTDC_ConfigLayer(&hlcd_ltdc, &pLayerCfg, LTDC_LAYER_2);

  Display_Logo();
  Display_StatArea();
}

/**
  * @brief  Configure the DCMIPP pipe1 for preview
  */
static void DCMIPP_Pipe1Config_Preview(void)
{
  CMW_DCMIPP_Conf_t dcmipp_conf = {0};

  /* Configure Pipe Main */
  dcmipp_conf.output_width = Camera_SensorConf.PreviewWidth;
  dcmipp_conf.output_height = Camera_SensorConf.PreviewHeight;
  dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_YUV422_1;
  dcmipp_conf.output_bpp = BPP_YUV422;
  dcmipp_conf.enable_swap = 0;
  dcmipp_conf.enable_gamma_conversion = -1; /* Let the ISP config file to decide */
  dcmipp_conf.mode = CMW_Aspect_ratio_fit;
  dcmipp_conf.isp_decimation_ratio_h = DecimationFactor;
  dcmipp_conf.isp_decimation_ratio_v = DecimationFactor;

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &DisplayFramePitch) != 0)
  {
    printf("ERROR: can't configure pipe1 for preview\r\n");
    Error_Handler();
  }
}

/**
  * @brief  Initialize the camera
  * @param  hDcmipp Pointer to the dcmipp device
  * @retval 0 if success, 1 otherwise
  */
static int Camera_Init(DCMIPP_HandleTypeDef **hDcmipp)
{
  CMW_CameraInit_t initConf = {0};

  initConf.width = 0;  /* width and height not specified => camera full resolution is set */
  initConf.height = 0; /* width and height not specified => camera full resolution is set */
  initConf.fps = 30;
  initConf.mirror_flip = CAMERA_FLIP;

  if (CMW_CAMERA_Init(&initConf, NULL) != CMW_ERROR_NONE)
  {
    printf("ERROR: Failed to Initialize camera\r\n");
    return 1;
  }

  *hDcmipp = CMW_CAMERA_GetDCMIPPHandle();

  return 0;
}

/**
  * @brief  Configure the camera
  * @param  Instance Camera instance
  * @retval 0 if success, 1 otherwise
  */
static int Camera_Config(uint32_t Instance)
{
  int32_t  ret;
  UNUSED(Instance);

  /* Make sure manual exposure is set on camera sensor side
   * This has no effect if the camera does not support it.
   */
  ret = CMW_CAMERA_SetExposureMode(CMW_EXPOSUREMODE_MANUAL); /* CMW_EXPOSUREMODE_AUTO or CMW_EXPOSUREMODE_AUTOFREEZE */
  if ((ret != CMW_ERROR_NONE) && (ret != CMW_ERROR_FEATURE_NOT_SUPPORTED))
  {
    printf("ERROR: Failed to set manual exposure\r\n");
    return 1;
  }

  /* Get the sensor information to fill the Camera_SensorConf structure */
  CMW_Sensor_Info_t info;
  ret = CMW_CAMERA_GetSensorInfo(&info);
  if (ret != CMW_ERROR_NONE)
  {
    printf("ERROR: Failed to get the sensor information\r\n");
    return 1;
  }
  strncpy(Camera_SensorConf.name, info.name, sizeof(Camera_SensorConf.name));
  Camera_SensorConf.CamImgWidth = info.width;
  Camera_SensorConf.CamImgHeight = info.height;
  ComputePreviewSize(Camera_SensorConf.CamImgWidth,
                     Camera_SensorConf.CamImgHeight,
                     MAX_PREVIEW_BUFFER_WIDTH,
                     MAX_PREVIEW_BUFFER_HEIGHT,
                     &Camera_SensorConf.PreviewWidth,
                     &Camera_SensorConf.PreviewHeight);
  switch (info.color_depth)
  {
  case 8:
    Camera_SensorConf.SensorDataType = DCMIPP_DT_RAW8;
    Camera_SensorConf.BytePerPixel = BPP_RAW8;
    break;
  case 10:
    Camera_SensorConf.SensorDataType = DCMIPP_DT_RAW10;
    Camera_SensorConf.BytePerPixel = BPP_RAW10;
    break;
  case 12:
    Camera_SensorConf.SensorDataType = DCMIPP_DT_RAW12;
    Camera_SensorConf.BytePerPixel = BPP_RAW12;
    break;
  case 14:
    Camera_SensorConf.SensorDataType = DCMIPP_DT_RAW14;
    Camera_SensorConf.BytePerPixel = BPP_RAW14;
    break;
  default:
    Camera_SensorConf.SensorDataType = DCMIPP_DT_RAW8;
    Camera_SensorConf.BytePerPixel = BPP_RAW8;
  }

  /* Configure the DCMIPP pipes */
  DCMIPP_Pipe1Config_Preview();
  DCMIPP_Pipe2Config_DumpLiveStreaming();
  DCMIPP_Pipe0Config_DumpSensorRaw();

  return 0;
}

/**
  * @brief  ThreadX spinner refresh thread.
  */
static void spinner_thread_fct(ULONG arg)
{
  UNUSED(arg);

  while (1)
  {
    DumpSpinner_Task();
    tx_thread_sleep(1);
  }
}

static void LTDC_Buffers_Swap(void)
{
  if (HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, (uint32_t)GetPreviewBuffer(),
                                   LTDC_LAYER_1) != HAL_OK)
  {
    printf("WARNING: Failed to update preview buffer address\r\n");
    return;
  }

  if (HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, LTDC_LAYER_1) != HAL_OK)
  {
    printf("WARNING: Failed to reload preview layer\r\n");
    return;
  }
}

/**
  * @brief  Get sensor id in the list of supported sensors
  * @param  None
  * @retval sensor id
  */
static uint8_t get_sensor_id(void)
{
  /* Get the sensor name connected:
   * - UNKNOWN config @ ISP_IQParamCacheInit[0]
   * - IMX335  config @ ISP_IQParamCacheInit[1]
   * - VD66GY  config @ ISP_IQParamCacheInit[2]
   * - VD5943  config @ ISP_IQParamCacheInit[3]
   * - VD1943  config @ ISP_IQParamCacheInit[4]
   * - VD65G4  config @ ISP_IQParamCacheInit[5]
   * - VD56G3  config @ ISP_IQParamCacheInit[6]
   * - IMX477  config @ ISP_IQParamCacheInit[7]
   */
  char *sensor_names[] = {"IMX335", "VD66GY", "VD5943", "VD1943", "VD65G4", "VD56G3", "IMX477"}; // Add new sensor in this list
  const char *sensor_name;
  uint8_t sensor_id = 0;
  int ret;
  int i;

  ret = CMW_CAMERA_GetSensorName(&sensor_name);
  assert(ret == 0);

  /* find sensor id */
  for (i = 0; i < ARRAY_SIZE(sensor_names); i++)
  {
    if (strcmp(sensor_name, sensor_names[i]))
      continue;

    sensor_id = i + 1;
    break;
  }

  if (sensor_id == 0)
  {
    printf("INFO: Unknown sensor, select untuned configuration for isp\r\n");
  }

  return sensor_id;
}

void App_Run()
{
  const UINT spinner_priority = TX_MAX_PRIORITIES / 2 - 2;
  uint32_t camera_instance = 0;
  int ret;

  ret = tx_thread_create(&spinner_thread, "spinner", spinner_thread_fct, 0,
                         spinner_thread_stack, sizeof(spinner_thread_stack),
                         spinner_priority, spinner_priority, TX_NO_TIME_SLICE, TX_AUTO_START);
  assert(ret == 0);

  printf("**** Initializing IQ Tuning application  ****\r\n");

  /* Do not rely on Camera Middleware to initialize the ISP device => initialize it by yourself
   * Provide ISP Library with helpers to control non-ISP features (application Helpers are mandatory
   * to get/set sensor exposure or gain)
   * Statistic area is provided with null value so that it forces the ISP Library to get the statistic
   * area information from the tuning file.
   */
  ISP_AppliHelpersTypeDef appliHelpers = {
    .GetSensorInfo = GetSensorInfo,
    .SetSensorGain = SetSensorGain,
    .GetSensorGain = GetSensorGain,
    .SetSensorExposure = SetSensorExposure,
    .GetSensorExposure = GetSensorExposure,
    .StartPreview = Camera_StartPreview,
    .StopPreview = Camera_StopPreview,
    .DumpFrame = Camera_DumpFrame,
    .SetSensorTestPattern = SetSensorTestPattern,
  };

  /* Initialize the camera */
  if (Camera_Init(&phDcmipp) != 0)
  {
    printf("ERROR: can't initialize camera\r\n");
    Error_Handler();
  }

  uint8_t sensor_id = get_sensor_id();

  ret = ISP_Init(&hIsp, phDcmipp, camera_instance, &appliHelpers, ISP_IQParamCacheInit[sensor_id]);
  if (ret != ISP_OK)
  {
    printf("ERROR: Can't init ISP (error %d)\r\n", ret);
    Error_Handler();
  }

  /* Get the ISP decimation factor applied in the ISP pipeline */
  ISP_DecimationTypeDef decimation;
  ISP_GetDecimationFactor(&hIsp, &decimation);
  DecimationFactor = decimation.factor;
  printf("INFO: Decimation factor is: %d\r\n", DecimationFactor);

  /* Configure the DCMIPP device and the camera */
  if (Camera_Config(camera_instance) != 0)
  {
    printf("ERROR: can't configure camera\r\n");
    Error_Handler();
  }

  /* Configure the display */
  Display_Config();

#ifdef ISP_ENABLE_UVC
  /* Define the preview size and fps for the UVC streaming */
  usb_uvc_init(Camera_SensorConf.PreviewWidth, Camera_SensorConf.PreviewHeight, 10/*fps*/);
#endif

  ret = ISP_SVC_Start_ToolCom();
  if (ret != ISP_OK)
  {
    printf("ERROR: Can't start tool com  (error %d)\r\n", ret);
    Error_Handler();
  }

  /* Configure the IPPlug Fifo size for each pipe */
  ret = SetIPPlugDefaultConf();
  if (ret != ISP_OK)
  {
    printf("ERROR: Can't configure IPPlug (error %d)\r\n", ret);
    Error_Handler();
  }

  /* Start the main pipe */
  if (CMW_CAMERA_DoubleBufferStart(DCMIPP_PIPE1,
                                   (uint8_t *) Main_DestBuffer[0],
                                   (uint8_t *) Main_DestBuffer[1],
                                   CMW_CAPTUREMODE_CONTINUOUS) != CMW_ERROR_NONE)
  {
    printf("ERROR: Failed to start CAMERA\r\n");
    Error_Handler();
  }

  /* Start the ISP device */
  ret = ISP_Start(&hIsp);
  if (ret != ISP_OK)
  {
    printf("ERROR: Can't start ISP (error %d)\r\n", ret);
    Error_Handler();
  }

  printf("Camera and ISP started\r\n");

  /* Application main loop */
  while (1)
  {
    tx_thread_sleep(1);
#ifdef ISP_ENABLE_UVC
    int32_t uvc_tx_size = DisplayFramePitch * Camera_SensorConf.PreviewHeight;
    usb_uvc_show_frame(GetPreviewBuffer(), uvc_tx_size);
#endif /* ISP_ENABLE_UVC */

    /* Call the ISP background task */
    ret = ISP_BackgroundProcess(&hIsp);
    if (ret != ISP_OK)
    {
      printf("WARNING: ISP Background process error %d (%s)\r\n", ret, ERROR_MESSAGE(ret));
    }

    /* Get the Current statistic area that can be updated by the ISP IQTune */
    Check_StatAreaUpdate();
    /* Additional applicative processing can be implemented from here */
  }
}

/**
  * @brief  Error callback on the pipe. Occurs when overrun occurs on the pipe.
  * @param  Pipe  Pipe receiving the callback
  * @retval None
  */
void CMW_CAMERA_PIPE_ErrorCallback(uint32_t pipe)
{
  printf("WARNING: Overrun detected on pipe %ld\r\n", pipe);

  /* Restart pipe 1 to clear all errors so that an overrun is treated as a warning */
  if (pipe == DCMIPP_PIPE1)
  {
    if (HAL_DCMIPP_CSI_PIPE_Stop(phDcmipp, pipe, DCMIPP_VIRTUAL_CHANNEL0) != HAL_OK)
    {
      printf("ERROR: failed to stop CSI on pipe 1\r\n");
      return;
    }
    if (HAL_DCMIPP_CSI_PIPE_DoubleBufferStart(phDcmipp, pipe, DCMIPP_VIRTUAL_CHANNEL0, (uint32_t)Main_DestBuffer[0],
                                              (uint32_t)Main_DestBuffer[1], DCMIPP_MODE_CONTINUOUS) != HAL_OK)
    {
      printf("ERROR: failed to restart CSI on pipe 1\r\n");
      return;
    }
  }
}

void main_PreviewPipeFrameEvent(uint32_t pipe)
{
  if (pipe != DCMIPP_PIPE1)
  {
    return;
  }

  preview_frame_count++;
  buffer_disp_idx = (buffer_disp_idx + 1) % 2;

  if (preview_ltdc_swap_enabled != 0U)
  {
    LTDC_Buffers_Swap();
  }
}

/* Keep MX_LTDC_ConfigLayer() empty so BSP_LCD_Init() does not initialize LTDC_LAYER_1.
 * The default MX_LTDC_ConfigLayer() implementation starts LTDC_LAYER_1 with a source buffer at LCD_LAYER_0_ADDRESS.
 * The default LCD_LAYER_0_ADDRESS value (0x34200000U) is the start of NPU RAM.
 * If there is a delay between BSP_LCD_Init() and HAL_LTDC_ConfigLayer(LTDC_LAYER_1) that exceeds one vblank,
 * LTDC may start fetching data from NPU RAM. Since NPU RAM is not initialized, LTDC can get stuck on those accesses.
 * This workaround is acceptable here because we do not call BSP_LCD_ConfigLayer() and instead call
 * HAL_LTDC_ConfigLayer() directly.
 */
HAL_StatusTypeDef MX_LTDC_ConfigLayer(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  return HAL_OK;
}
