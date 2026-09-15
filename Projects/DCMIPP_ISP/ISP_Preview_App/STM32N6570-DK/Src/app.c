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

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "cmw_camera.h"
#include "main.h"
#include "stm32n6570_discovery_lcd.h"
#include "stm32n6xx_hal_def.h"
#include "utils.h"

#include "tx_api.h"

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
#define MAX_PREVIEW_BUFFER_WIDTH    LCD_WIDTH
#define MAX_PREVIEW_BUFFER_HEIGHT   LCD_HEIGHT

typedef struct
{
  uint32_t CamImgWidth;      /*!< Sensor output width                              */
  uint32_t CamImgHeight;     /*!< Sensor output height                             */
  uint32_t PreviewWidth;     /*!< DCMIPP output width for preview                  */
  uint32_t PreviewHeight;    /*!< DCMIPP output height for preview                 */
} CAMERA_SensorConfTypeDef;

typedef struct
{
  uint32_t X0;
  uint32_t Y0;
  uint32_t XSize;
  uint32_t YSize;
} Rectangle_TypeDef;

DCMIPP_HandleTypeDef *phDcmipp;

/* LCD full display area */
const static Rectangle_TypeDef display_area = {
  .X0 = 0,
  .Y0 = 0,
  .XSize = LCD_WIDTH,
  .YSize = LCD_HEIGHT,
};
static int buffer_disp_idx = 1;
static uint32_t preview_frame_pitch;
/* Frame buffers */
__attribute__ ((section(".psram_bss")))
__attribute__ ((aligned (32)))
static uint8_t Main_DestBuffer[2][MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT * BPP_RGB888];

static uint8_t *GetPreviewBuffer(void)
{
  return Main_DestBuffer[buffer_disp_idx];
}

/**
  * @brief  Initialize the LTDC with a preview area (layer1) and a statistic area (layer2)
  * @param  none
  * @retval none
  */
static void Display_Config(CAMERA_SensorConfTypeDef *sensor_conf, uint32_t frame_pitch)
{
  /* Preview area at the middle of the display area */
  Rectangle_TypeDef PreviewArea = {
    .X0 = (display_area.XSize - sensor_conf->PreviewWidth) / 2,
    .Y0 = (display_area.YSize - sensor_conf->PreviewHeight) / 2,
    .XSize = sensor_conf->PreviewWidth,
    .YSize = sensor_conf->PreviewHeight,
  };
  LTDC_LayerCfgTypeDef pLayerCfg ={0};

  /* Initialized LTDC */
  BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);
  memset(Main_DestBuffer, 0, sizeof(Main_DestBuffer));
  SCB_CleanInvalidateDCache_by_Addr(Main_DestBuffer, sizeof(Main_DestBuffer));

  pLayerCfg.WindowX0 = PreviewArea.X0;
  pLayerCfg.WindowX1 = PreviewArea.X0 + PreviewArea.XSize;
  pLayerCfg.WindowY0 = PreviewArea.Y0;
  pLayerCfg.WindowY1 = PreviewArea.Y0 + PreviewArea.YSize;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB888;
  pLayerCfg.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = (uint32_t) Main_DestBuffer;
  pLayerCfg.ImageWidth = PreviewArea.XSize;
  pLayerCfg.ImageHeight = PreviewArea.YSize;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  HAL_LTDC_ConfigLayer(&hlcd_ltdc, &pLayerCfg, LTDC_LAYER_1);

  /* Set pitch to match with the sensor frame pitch */
  preview_frame_pitch = frame_pitch;
  HAL_LTDC_SetPitchInBytes(&hlcd_ltdc, frame_pitch, LTDC_LAYER_1);
}

/**
  * @brief  Configure the DCMIPP pipe1 for preview
  */
static void DCMIPP_Pipe1Config_Preview(CAMERA_SensorConfTypeDef *sensor_conf, uint32_t *frame_pitch)
{
  CMW_DCMIPP_Conf_t dcmipp_conf = {0};

  /* Configure Pipe Main */
  dcmipp_conf.output_width = sensor_conf->PreviewWidth;
  dcmipp_conf.output_height = sensor_conf->PreviewHeight;
  dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1;
  dcmipp_conf.output_bpp = BPP_RGB888;
  dcmipp_conf.enable_swap = 0;
  dcmipp_conf.enable_gamma_conversion = -1; /* Let the ISP config file to decide */
  dcmipp_conf.mode = CMW_Aspect_ratio_fit;

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, frame_pitch) != 0)
  {
    printf("ERROR: can't configure pipe1 for preview\r\n");
    Error_Handler();
  }
}

/**
  * @brief  Configure the camera
  * @param  hDcmipp Pointer to the dcmipp device
  * @param  Instance Camera instance
  * @retval 0 if success, 1 otherwise
  */
static int Camera_Config(DCMIPP_HandleTypeDef **hDcmipp, CAMERA_SensorConfTypeDef *sensor_conf, uint32_t *frame_pitch)
{
  CMW_CameraInit_t initConf = {0};
  int32_t ret;

  initConf.width = 0;  /* width and height not specified => camera full resolution is set */
  initConf.height = 0; /* width and height not specified => camera full resolution is set */
  initConf.fps = 30;
  initConf.mirror_flip = CMW_MIRRORFLIP_NONE; /* CMW_MIRRORFLIP_NONE or CMW_MIRRORFLIP_FLIP or CMW_MIRRORFLIP_MIRROR or CMW_MIRRORFLIP_FLIP_MIRROR */

  ret = CMW_CAMERA_Init(&initConf, NULL);
  if (ret != CMW_ERROR_NONE)
  {
    printf("ERROR: Failed to Initialize camera\r\n");
    return 1;
  }
  *hDcmipp = CMW_CAMERA_GetDCMIPPHandle();

  sensor_conf->CamImgWidth = initConf.width;
  sensor_conf->CamImgHeight = initConf.height;
  ComputePreviewSize(sensor_conf->CamImgWidth, sensor_conf->CamImgHeight, MAX_PREVIEW_BUFFER_WIDTH,
                     MAX_PREVIEW_BUFFER_HEIGHT, &sensor_conf->PreviewWidth, &sensor_conf->PreviewHeight);

  /* Configure the DCMIPP pipes */
  DCMIPP_Pipe1Config_Preview(sensor_conf, frame_pitch);

  return 0;
}

static void LTDC_Buffers_Swap()
{
  if (HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, (uint32_t)GetPreviewBuffer(),
                                   LTDC_LAYER_1) != HAL_OK)
  {
    printf("WARNING: Failed to update preview buffer address\r\n");
    return;
  }

  /* HAL_LTDC_SetAddress_NoReload() reprograms CFBLR from ImageWidth, so the DCMIPP pitch must be restored. */
  if (HAL_LTDC_SetPitchInBytes_NoReload(&hlcd_ltdc, preview_frame_pitch, LTDC_LAYER_1) != HAL_OK)
  {
    printf("WARNING: Failed to update preview buffer pitch\r\n");
    return;
  }

  if (HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, LTDC_LAYER_1) != HAL_OK)
  {
    printf("WARNING: Failed to reload preview layer\r\n");
    return;
  }
}

void App_Run()
{
  CAMERA_SensorConfTypeDef sensor_conf = {0};
  uint32_t frame_pitch;
  int ret;

  printf("**** Initializing ISP preview application  ****\r\n");
  /* Initialize the DCMIPP device and the camera */
  if (Camera_Config(&phDcmipp, &sensor_conf, &frame_pitch) != 0)
  {
    printf("ERROR: can't configure camera\r\n");
    Error_Handler();
  }

  /* Configure the display */
  Display_Config(&sensor_conf, frame_pitch);

  /* Start the main pipe */
  if (CMW_CAMERA_DoubleBufferStart(DCMIPP_PIPE1,
                                   (uint8_t *) Main_DestBuffer[0],
                                   (uint8_t *) Main_DestBuffer[1],
                                   CMW_CAPTUREMODE_CONTINUOUS) != CMW_ERROR_NONE)
  {
    printf("ERROR: Failed to start CAMERA\r\n");
    Error_Handler();
  }
  printf("Camera and ISP started\r\n");

  /* Application main loop */
  while (1)
  {
    tx_thread_sleep(1);
    ret = CMW_CAMERA_Run();
    assert(ret == CMW_ERROR_NONE);
  }
}

/**
  * @brief  Error callback on a pipe (typically triggered on overrun).
  * @param  pipe  Pipe receiving the callback
  * @retval None
  */
void CMW_CAMERA_PIPE_ErrorCallback(uint32_t pipe)
{
  /* Overrun detected: no recovery is done here.
   * If you must avoid overflow, use HAL_DCMIPP_SetIPPlugConfig()
   * to configure IPPlug, which controls DCMIPP AXI master sharing between pipes.
   */
  UNUSED(pipe);
}

int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe)
{
  main_PreviewPipeFrameEvent(pipe);

  return CMW_ERROR_NONE;
}

void main_PreviewPipeFrameEvent(uint32_t pipe)
{
  if (pipe != DCMIPP_PIPE1)
  {
    return;
  }

  buffer_disp_idx = (buffer_disp_idx + 1) % 2;

  LTDC_Buffers_Swap();
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
