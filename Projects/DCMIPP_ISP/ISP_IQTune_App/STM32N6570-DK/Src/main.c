/**
 ******************************************************************************
 * @file    main.c
 * @author  AIS Application Team
 * @brief   Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_bus.h"
#include "stm32n6570_discovery_lcd.h"
#include "stm32n6570_discovery_xspi.h"
#include "isp_api.h"
#include "isp_param_conf.h"
#include "cmw_camera.h"
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
#include "ispiqtune_logo_argb8888.h"
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

#include <stdio.h>

/* Global variables ----------------------------------------------------------*/
/* Device handles */
DCMIPP_HandleTypeDef *phDcmipp;
ISP_HandleTypeDef    hIsp;

/* External function prototypes ----------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
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

/* Private define ----------------------------------------------------------*/
#define USE_DCACHE

/* Pixel Format Size */
#define BPP_RAW8       1
#define BPP_RAW10      2
#define BPP_RAW12      2
#define BPP_RAW14      2
#define BPP_RGB565     2
#define BPP_RGB888     3
#define BPP_ARGB8888   4

/* LCD Size */
#define LCD_WIDTH         RK050HR18_WIDTH
#define LCD_HEIGHT        RK050HR18_HEIGHT

/* Preview frame buffer */
#define MAX_PREVIEW_BUFFER_WIDTH    640
#define MAX_PREVIEW_BUFFER_HEIGHT   480

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
/* Dump frame buffer */
#define MAX_DUMP_BUFFER_SIZE (5 * 1024 * 1024) /* 5 MP */
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

/* Private macro -------------------------------------------------------------*/
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

/* Private variables ---------------------------------------------------------*/
/* Frame buffers */
/* Allocate the Main_DestBuffer (RGB888) in SRAM dedicated region */
__attribute__ ((section(".buffRam")))
__attribute__ ((aligned (32)))
uint8_t Main_DestBuffer[MAX_PREVIEW_BUFFER_WIDTH * MAX_PREVIEW_BUFFER_HEIGHT * BPP_RGB888];
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
__attribute__ ((section (".psram_bss")))
__attribute__ ((aligned (32)))
uint8_t Dump_DestBuffer[MAX_DUMP_BUFFER_SIZE * BPP_RGB888];
extern uint32_t ISP_AncillaryPipe_FrameCount;
extern uint32_t ISP_DumpPipe_FrameCount;
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */
__attribute__ ((section (".psram_bss")))
__attribute__ ((aligned (32)))
uint8_t Overlay_Buffer[LCD_WIDTH * LCD_HEIGHT * BPP_ARGB8888];

/* Camera_SensorConf will be filled when Camera_Config is called */
CAMERA_SensorConfTypeDef Camera_SensorConf = {0};

/* LCD full display area */
Rectangle_TypeDef display_area = {
  .X0 = 0,
  .Y0 = 0,
  .XSize = LCD_WIDTH,
  .YSize = LCD_HEIGHT,
};
Rectangle_TypeDef stat_area = {0};
uint8_t DecimationFactor = 1;
uint32_t DisplayFramePitch;
static UART_HandleTypeDef huart1;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Security_Config();
static void IAC_Config();
static void Console_Config(void);
int Camera_Config(DCMIPP_HandleTypeDef **hDcmipp, uint32_t Instance);
void Display_Config();

ISP_StatusTypeDef GetSensorInfo(uint32_t camera_instance, ISP_SensorInfoTypeDef *info);
ISP_StatusTypeDef SetSensorGain(uint32_t camera_instance, int32_t gain);
ISP_StatusTypeDef GetSensorGain(uint32_t camera_instance, int32_t *gain);
ISP_StatusTypeDef SetSensorExposure(uint32_t camera_instance, int32_t exposure);
ISP_StatusTypeDef GetSensorExposure(uint32_t camera_instance, int32_t *exposure);

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
ISP_StatusTypeDef Check_StatAreaUpdate(void);
ISP_StatusTypeDef SetSensorTestPattern(uint32_t camera_instance, int32_t mode);
ISP_StatusTypeDef Camera_StartPreview(void *pHdcmipp);
ISP_StatusTypeDef Camera_StopPreview(void *pHdcmipp);
ISP_StatusTypeDef Camera_DumpFrame(void *pHdcmipp, uint32_t Pipe, ISP_DumpCfgTypeDef Config,
                                   uint32_t **pBuffer, ISP_DumpFrameMetaTypeDef *pMeta);
ISP_StatusTypeDef SetIPPlugConf(uint32_t Pipe0_OW, uint32_t Pipe0_PFS,
                                uint32_t Pipe1_OW, uint32_t Pipe1_PFS,
                                uint32_t Pipe2_OW, uint32_t Pipe2_PFS);
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

/**
  * @brief  Main program
  * @retval None
  */
int main(void)
{
  uint32_t camera_instance = 0;
  ISP_StatusTypeDef ret;

  /* Power on ICACHE */
  MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_ICACTIVE_Msk;

  /* Set back system and CPU clock source to HSI */
  __HAL_RCC_CPUCLK_CONFIG(RCC_CPUCLKSOURCE_HSI);
  __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);

  SCB_EnableICache();

#if defined(USE_DCACHE)
  /* Power on DCACHE */
  MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_DCACTIVE_Msk;
  SCB_EnableDCache();
#endif

  HAL_Init();

  /* Set the clock configuration */
  SystemClock_Config();

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  /* Initialize XSPI memory */
  /* Only needed to store the dump frame */
  BSP_XSPI_RAM_Init(0);
  BSP_XSPI_RAM_EnableMemoryMappedMode(0);
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

  /* Set all required IPs as secure privileged */
  Security_Config();
  IAC_Config();

  /* Configure UART for message log */
  Console_Config();


#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  printf("**** Initializing IQ Tuning application  ****\r\n");
#else
  printf("**** Initializing ISP preview application  ****\r\n");
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

  /* Initialize the DCMIPP device and the camera */
  if (Camera_Config(&phDcmipp, camera_instance) != 0)
  {
    printf("ERROR: can't configure camera\r\n");
    Error_Handler();
  }

  /* Configure the display */
  Display_Config();

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
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
  /* Get the sensor name connected:
   * - IMX335 config @ ISP_IQParamCacheInit[0]
   * - VD66GY config @ ISP_IQParamCacheInit[1]
   */
  uint8_t sensor_id = 0;
  if (strcmp(Camera_SensorConf.name, "IMX335") == 0)
  {
    sensor_id = 0;
  }
  else if (strcmp(Camera_SensorConf.name, "VD66GY") == 0)
  {
    sensor_id = 1;
  }
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

  /* Configure the IPPlug Fifo size for each pipe */
  ret = SetIPPlugConf(Camera_SensorConf.CamImgWidth, Camera_SensorConf.BytePerPixel,
                      Camera_SensorConf.PreviewWidth, BPP_RGB888,
                      Camera_SensorConf.CamImgWidth, BPP_RGB888);
  if (ret != ISP_OK)
  {
    printf("ERROR: Can't configure IPPlug (error %d)\r\n", ret);
    Error_Handler();
  }
#else /* ISP_MW_TUNING_TOOL_SUPPORT */
  (void)ISP_IQParamCacheInit; /* unused */
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

  /* Start the main pipe */
  if (CMW_CAMERA_Start(DCMIPP_PIPE1, (uint8_t *) Main_DestBuffer, CMW_MODE_CONTINUOUS) != CMW_ERROR_NONE)
  {
    printf("ERROR: Failed to start CAMERA\r\n");
    Error_Handler();
  }

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  /* Start the ISP device */
  ret = ISP_Start(&hIsp);
  if (ret != ISP_OK)
  {
    printf("ERROR: Can't start ISP (error %d)\r\n", ret);
    Error_Handler();
  }
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

  printf("Camera and ISP started\r\n");

  /* Application main loop */
  while (1)
  {
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
    /* Call the ISP background task */
    ret = ISP_BackgroundProcess(&hIsp);
    if (ret != ISP_OK)
    {
      printf("WARNING: ISP Background process error %d (%s)\r\n", ret, ERROR_MESSAGE(ret));
    }


    /* Get the Current statistic area that can be updated by the ISP IQTune */
    Check_StatAreaUpdate();
#else
    ret = CMW_ERROR_NONE;
    ret = CMW_CAMERA_Run();
    assert(ret == CMW_ERROR_NONE);
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */
    /* Additional applicative processing can be implemented from here */
  }
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
  dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1;
  dcmipp_conf.output_bpp = BPP_RGB888;
  dcmipp_conf.enable_swap = 0;
  dcmipp_conf.enable_gamma_conversion = -1; /* Let the ISP config file to decide */
  dcmipp_conf.mode = CMW_Aspect_ratio_fullscreen;

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &DisplayFramePitch) != 0)
  {
    printf("ERROR: can't configure pipe1 for preview\r\n");
    Error_Handler();
  }
}

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
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
  dcmipp_conf.mode = CMW_Aspect_ratio_fullscreen;

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE2, &dcmipp_conf, &pitch) != 0)
  {
    printf("ERROR: can't configure pipe2 for Live streaming dump\r\n");
    Error_Handler();
  }
}

/**
  * @brief  Configure the DCMIPP pipe2 for ISP pipeline RGB888 dump
  */
static void DCMIPP_Pipe2Config_DumpIspRGB888(void)
{
  CMW_DCMIPP_Conf_t dcmipp_conf = {0};
  uint32_t pitch;

  /* Configure Ancillary pipe with downscaling for Live streaming snapshot */
  dcmipp_conf.output_width = Camera_SensorConf.CamImgWidth;
  dcmipp_conf.output_height = Camera_SensorConf.CamImgHeight;
  dcmipp_conf.output_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1;
  dcmipp_conf.output_bpp = BPP_RGB888;
  dcmipp_conf.enable_swap = 0;
  dcmipp_conf.enable_gamma_conversion = -1; /* Let the ISP config file to decide */
  dcmipp_conf.mode = CMW_Aspect_ratio_fullscreen;

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE2, &dcmipp_conf, &pitch) != 0)
  {
    printf("ERROR: can't configure pipe2 for ISP RGB888 dump\r\n");
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

  if (CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE0, &dcmipp_conf, &pitch) != 0)
  {
    printf("ERROR: can't configure pipe0\r\n");
    Error_Handler();
  }
}
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

/**
  * @brief  Get the preview size to respect aspect ratio of the camera.
  * @param  cameraWidth   width of the camera frame
  * @param  cameraHeight  height of the camera frame
  * @param  previewWidth  pointer on the calculated preview width
  * @param  previewHeight pointer on the calculated preview height
  */
static void ComputePreviewSize(uint32_t cameraWidth, uint32_t cameraHeight, uint32_t *previewWidth, uint32_t *previewHeight)
{
  /* Camera frame aspect ratio */
  float aspectRatio = (float)cameraWidth / (float)cameraHeight;

  /* Keep camera aspect ratio */
  if (aspectRatio > 1.0f)
  { /* Landscape case */
    *previewWidth = MAX_PREVIEW_BUFFER_WIDTH;
    *previewHeight = (uint32_t)(MAX_PREVIEW_BUFFER_WIDTH / aspectRatio);
    if (*previewHeight > MAX_PREVIEW_BUFFER_HEIGHT)
    {
      *previewHeight = MAX_PREVIEW_BUFFER_HEIGHT;
      *previewWidth = (uint32_t)(MAX_PREVIEW_BUFFER_HEIGHT * aspectRatio);
    }
  }
  else
  { /* Portrait case */
    *previewHeight = MAX_PREVIEW_BUFFER_HEIGHT;
    *previewWidth = (uint32_t)(MAX_PREVIEW_BUFFER_HEIGHT * aspectRatio);
    if (*previewWidth > MAX_PREVIEW_BUFFER_HEIGHT)
    {
      *previewWidth = MAX_PREVIEW_BUFFER_WIDTH;
      *previewHeight = (uint32_t)(MAX_PREVIEW_BUFFER_WIDTH / aspectRatio);
    }
  }
}

/**
  * @brief  Configure the camera
  * @param  hDcmipp Pointer to the dcmipp device
  * @param  Instance Camera instance
  * @retval 0 if success, 1 otherwise
  */
int Camera_Config(DCMIPP_HandleTypeDef **hDcmipp, uint32_t Instance)
{
  int32_t  ret;
  CMW_CameraInit_t initConf = {0};
  UNUSED(Instance);

  initConf.width = 0;  /* width and height not specified => camera full resolution is set */
  initConf.height = 0; /* width and height not specified => camera full resolution is set */
  initConf.fps = 30;
  initConf.mirror_flip = CMW_MIRRORFLIP_NONE; /* CMW_MIRRORFLIP_NONE or CMW_MIRRORFLIP_FLIP or CMW_MIRRORFLIP_MIRROR or CMW_MIRRORFLIP_FLIP_MIRROR */
  initConf.pixel_format = 0; /* Not yet implemented */
  initConf.anti_flicker = 0; /* Not yet implemented */

  ret = CMW_CAMERA_Init(&initConf);
  if (ret != CMW_ERROR_NONE)
  {
    printf("ERROR: Failed to Initialize camera\r\n");
    return 1;
  }

  *hDcmipp = CMW_CAMERA_GetDCMIPPHandle();

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
  ISP_SensorInfoTypeDef info;
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
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  DCMIPP_Pipe2Config_DumpLiveStreaming();
  DCMIPP_Pipe0Config_DumpSensorRaw();
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */

  return 0;
}

/**
  * @brief  Get the sensor info
  * @param  camera_instance: camera instance
  * @param  info: sensor info
  * @retval Operation result
  */
ISP_StatusTypeDef GetSensorInfo(uint32_t camera_instance, ISP_SensorInfoTypeDef *info)
{
  UNUSED(camera_instance);

  if (CMW_CAMERA_GetSensorInfo(info) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORINFO;

  return ISP_OK;
}

/**
  * @brief  Set the sensor gain
  * @param  camera_instance: camera instance
  * @param  gain: sensor gain to be applied
  * @retval Operation result
  */
ISP_StatusTypeDef SetSensorGain(uint32_t camera_instance, int32_t gain)
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
ISP_StatusTypeDef GetSensorGain(uint32_t camera_instance, int32_t *gain)
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
ISP_StatusTypeDef SetSensorExposure(uint32_t camera_instance, int32_t exposure)
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
ISP_StatusTypeDef GetSensorExposure(uint32_t camera_instance, int32_t *exposure)
{
  UNUSED(camera_instance);

  if (CMW_CAMERA_GetExposure(exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
/**
  * @brief  Draw IQTune Logo
  * @param  FramebufferAddress Buffer address
  * @param  DisplayArea        Coordinates of the display
  * @param  LogoArea           Coordinates of the logo area
  * @retval none
  */
void Draw_ARGB8888_Logo(uint32_t FramebufferAddress, Rectangle_TypeDef DisplayArea, Rectangle_TypeDef LogoArea)
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

  if (HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_LAYER_2, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK)
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
void Draw_ARGB8888_StatArea(uint32_t FramebufferAddress, Rectangle_TypeDef DisplayArea, Rectangle_TypeDef StatArea)
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
    .X0 = (stat_area.X0 * Camera_SensorConf.PreviewWidth) / Camera_SensorConf.CamImgWidth / DecimationFactor,
    .Y0 = (stat_area.Y0 * Camera_SensorConf.PreviewHeight) / Camera_SensorConf.CamImgHeight / DecimationFactor,
    .XSize = (stat_area.XSize * Camera_SensorConf.PreviewWidth) / Camera_SensorConf.CamImgWidth / DecimationFactor,
    .YSize = (stat_area.YSize * Camera_SensorConf.PreviewHeight) / Camera_SensorConf.CamImgHeight / DecimationFactor,
  };

  if (HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, (uint32_t) Overlay_Buffer, LTDC_LAYER_2) != HAL_OK)
  {
    printf("ERROR: Failed to update the statistic area on the preview\r\n");
  }

  Draw_ARGB8888_StatArea((uint32_t) Overlay_Buffer, display_area, ScaledStatArea);

  if (HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_LAYER_2, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK)
  {
    printf("ERROR: Failed to display the statistic area on the preview\r\n");
  }
}

/**
  * @brief  Check if the ISP Statistic Area need to be updated
  * @retval Operation result
  */
ISP_StatusTypeDef Check_StatAreaUpdate()
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
  * @brief  Set the sensor test pattern
  * @param  camera_instance: camera instance
  * @param  mode: sensor test pattern to be applied
  * @retval Operation result
  */
ISP_StatusTypeDef SetSensorTestPattern(uint32_t camera_instance, int32_t mode)
{
  UNUSED(camera_instance);

  if (CMW_CAMERA_SetTestPattern(mode) != CMW_ERROR_NONE)
    return ISP_ERR_EINVAL;

  return ISP_OK;
}

/**
  * @brief  Helper for ISP to start camera preview
  * @param  hDcmipp Pointer to the dcmipp device
  * @retval ISP status operation
  */
ISP_StatusTypeDef Camera_StartPreview(void *pDcmipp)
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

  /* Enable Layer 2 */
  BSP_LCD_SetLayerVisible(0,LTDC_LAYER_2, ENABLE);

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

  return ISP_OK;
}

/**
  * @brief  Helper for ISP to stop camera preview
  * @param  hDcmipp Pointer to the dcmipp device
  * @retval ISP status operation
  */
ISP_StatusTypeDef Camera_StopPreview(void *pDcmipp)
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

  /* Disable Layer 2 to avoid refresh from XSPI memory*/
  BSP_LCD_SetLayerVisible(0,LTDC_LAYER_2, DISABLE);

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
ISP_StatusTypeDef Camera_DumpFrame(void *pDcmipp, uint32_t Pipe, ISP_DumpCfgTypeDef Config,
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
    if (CMW_CAMERA_Start(Pipe, (uint8_t *) Dump_DestBuffer, CMW_MODE_SNAPSHOT) != HAL_OK)
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
    if (SetIPPlugConf(Camera_SensorConf.CamImgWidth, Camera_SensorConf.BytePerPixel,
                      Camera_SensorConf.PreviewWidth, BPP_RGB888,
                      Camera_SensorConf.CamImgWidth, BPP_RGB888) != ISP_OK)
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

    /* Maximize IPPlug Fifo size for dump pipe */
    if (SetIPPlugConf(Camera_SensorConf.CamImgWidth, Camera_SensorConf.BytePerPixel, 0, 0, 0, 0) != ISP_OK)
    {
      return ISP_ERR_DCMIPP_CONFIGPIPE;
    }

    lastId = ISP_GetDumpFrameId(&hIsp);
    if (CMW_CAMERA_Start(Pipe, (uint8_t *) Dump_DestBuffer, CMW_MODE_SNAPSHOT) != HAL_OK)
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
    if (SetIPPlugConf(Camera_SensorConf.CamImgWidth, Camera_SensorConf.BytePerPixel,
                      Camera_SensorConf.PreviewWidth, BPP_RGB888,
                      Camera_SensorConf.CamImgWidth, BPP_RGB888) != ISP_OK)
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
  * @brief  Configure the FIFO size and outsanting capability for IPPlug client of each pipe
  * @param  Pipe0_OW   Output width of dump pipe
  * @param  Pipe0_PFS  Pixel format size of dump pipe
  * @param  Pipe1_OW   Output width of main pipe
  * @param  Pipe1_PFS  Pixel format size of main pipe
  * @param  Pipe2_OW   Maximum output width of ancillary pipe
  * @param  Pipe2_PFS  Pixel format size of the maximum dump width on ancillary pipe
  * @retval ISP status operation
  */
ISP_StatusTypeDef SetIPPlugConf(uint32_t Pipe0_OW, uint32_t Pipe0_PFS,
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
#endif /* ISP_MW_TUNING_TOOL_SUPPORT */

/**
  * @brief  Initialize the LTDC with a preview area (layer1) and a statistic area (layer2)
  * @param  none
  * @retval none
  */
void Display_Config()
{
  MX_LTDC_LayerConfig_t config = {0};
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  /* Preview area at the right of the display area */
  Rectangle_TypeDef PreviewArea = {
    .X0 = (display_area.XSize - Camera_SensorConf.PreviewWidth),
    .Y0 = (display_area.YSize - Camera_SensorConf.PreviewHeight),
    .XSize = Camera_SensorConf.PreviewWidth,
    .YSize = Camera_SensorConf.PreviewHeight,
  };
#else
  /* Preview area at the middle of the display area */
  Rectangle_TypeDef PreviewArea = {
    .X0 = (display_area.XSize - Camera_SensorConf.PreviewWidth) / 2,
    .Y0 = (display_area.YSize - Camera_SensorConf.PreviewHeight) / 2,
    .XSize = Camera_SensorConf.PreviewWidth,
    .YSize = Camera_SensorConf.PreviewHeight,
  };
#endif /* ISP_MW_TUNING_TOOL_SUPPORT */

  BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);

  /* Preview layer Init */
  config.X0          = PreviewArea.X0;
  config.X1          = PreviewArea.X0 + PreviewArea.XSize;
  config.Y0          = PreviewArea.Y0;
  config.Y1          = PreviewArea.Y0 + PreviewArea.YSize;
  config.PixelFormat = LTDC_PIXEL_FORMAT_RGB888;
  config.Address     = (uint32_t) Main_DestBuffer;

  /* Initialize the buffer (grey color) */
  uint32_t buffer_size = Camera_SensorConf.PreviewWidth * Camera_SensorConf.PreviewHeight * BPP_RGB888;
  memset(Main_DestBuffer, 0x88, buffer_size);
  SCB_CleanDCache_by_Addr (Main_DestBuffer, (int32_t)buffer_size);

  BSP_LCD_ConfigLayer(0, LTDC_LAYER_1, &config);

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  /* Overlay layer Init */
  config.X0          = display_area.X0;
  config.X1          = display_area.X0 + display_area.XSize;
  config.Y0          = display_area.Y0;
  config.Y1          = display_area.Y0 + display_area.YSize;
  config.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  config.Address     = (uint32_t) Overlay_Buffer;

  /* Initialize the buffer */
  memset(Overlay_Buffer, 0x00, display_area.XSize * display_area.YSize * BPP_ARGB8888);

  BSP_LCD_ConfigLayer(0, LTDC_LAYER_2, &config);

  Display_Logo();
  Display_StatArea();
#endif  /* ISP_MW_TUNING_TOOL_SUPPORT */
}

/**
  * @brief  MX LTDC layer configuration.
  * @param  hltdc      LTDC handle
  * @param  LayerIndex Layer 0 or 1
  * @param  Config     Layer configuration
  * @note   Owerwrite weak function from lcd BSP
  * @retval HAL status
  */
HAL_StatusTypeDef MX_LTDC_ConfigLayer(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  HAL_StatusTypeDef ret;
  LTDC_LayerCfgTypeDef pLayerCfg ={0};

  pLayerCfg.WindowX0 = Config->X0;
  pLayerCfg.WindowX1 = Config->X1;
  pLayerCfg.WindowY0 = Config->Y0;
  pLayerCfg.WindowY1 = Config->Y1;
  pLayerCfg.PixelFormat = Config->PixelFormat;
  pLayerCfg.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = Config->Address;
  pLayerCfg.ImageWidth = (Config->X1 - Config->X0);
  pLayerCfg.ImageHeight = (Config->Y1 - Config->Y0);
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;

  ret = HAL_LTDC_ConfigLayer(hltdc, &pLayerCfg, LayerIndex);

  /* Set pitch to match with the sensor frame pitch */
  if ((LayerIndex == LTDC_LAYER_1) && (DisplayFramePitch != 0))
  {
    /* The LTDC HAL pitch API works with "number of pixel", not "number of bytes".
     * Hack : temporary set the pixel format to "1 byte per pixel", then configure the pitch
     * (unit = pixel = byte) and then restore the pixel format */
    uint32_t fmt;

    fmt = hltdc->LayerCfg[LayerIndex].PixelFormat;
    hltdc->LayerCfg[LayerIndex].PixelFormat = LTDC_PIXEL_FORMAT_L8;
    HAL_LTDC_SetPitch(hltdc, DisplayFramePitch, LayerIndex);
    hltdc->LayerCfg[LayerIndex].PixelFormat = fmt;
  }

  return ret;
}

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

  /* Ensure VDDCORE=0.9V before increasing the system frequency */
#if (STM32N6570_DK_REV == STM32N6570_DK_C01)
  BSP_SMPS_Init(SMPS_VOLTAGE_OVERDRIVE);
#else /* STM32N6570_DK_B01/STM32N6570_DK_A01/STM32N6570_DK_A03 */
  BSP_I2C2_Init();
  uint8_t tmp = 0x64;
  BSP_I2C2_WriteReg(0x49 << 1, 0x01, &tmp, 1);
  BSP_I2C2_DeInit();
  HAL_Delay(1); /* Assuming Voltage Ramp Speed of 1mV/us --> 100mV increase takes 100us */
#endif

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;

  /* PLL1 = 64 x 25 / 2 = 800MHz */
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL1.PLLM = 2;
  RCC_OscInitStruct.PLL1.PLLN = 25;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL1.PLLP1 = 1;
  RCC_OscInitStruct.PLL1.PLLP2 = 1;

  /* PLL2 = 64 x 125 / 8 = 1000MHz */
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL2.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL2.PLLM = 8;
  RCC_OscInitStruct.PLL2.PLLFractional = 0;
  RCC_OscInitStruct.PLL2.PLLN = 125;
  RCC_OscInitStruct.PLL2.PLLP1 = 1;
  RCC_OscInitStruct.PLL2.PLLP2 = 1;

  /* PLL3 = 64 x 225 / 16 = 900MHz */
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL3.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL3.PLLM = 16;
  RCC_OscInitStruct.PLL3.PLLN = 225;
  RCC_OscInitStruct.PLL3.PLLFractional = 0;
  RCC_OscInitStruct.PLL3.PLLP1 = 1;
  RCC_OscInitStruct.PLL3.PLLP2 = 1;

  /* PLL4 = 64 x 20 / 32 = 50MHz */
  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL4.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL4.PLLM = 32;
  RCC_OscInitStruct.PLL4.PLLFractional = 0;
  RCC_OscInitStruct.PLL4.PLLN = 20;
  RCC_OscInitStruct.PLL4.PLLP1 = 1;
  RCC_OscInitStruct.PLL4.PLLP2 = 1;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while(1);
  }

  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                                 RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK4 |
                                 RCC_CLOCKTYPE_PCLK5);

  /* CPU CLock (sysa_ck) = ic1_ck = PLL1 output/ic1_divider = 800 MHz */
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 1;

  /* AXI Clock (sysb_ck) = ic2_ck = PLL1 output/ic2_divider = 400 MHz */
  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider = 2;

  /* NPU Clock (sysc_ck) = ic6_ck = PLL2 output/ic6_divider = 1000 MHz */
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL2;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 1;

  /* AXISRAM3/4/5/6 Clock (sysd_ck) = ic11_ck = PLL3 output/ic11_divider = 900 MHz */
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL3;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 1;

  /* HCLK = sysb_ck / HCLK divider = 200 MHz */
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;

  /* PCLKx = HCLK / PCLKx divider = 200 MHz */
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
  {
    while(1);
  }

  RCC_PeriphCLKInitStruct.PeriphClockSelection = 0;

  /* XSPI1 kernel clock (ck_ker_xspi1) = HCLK = 200MHz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_XSPI1;
  RCC_PeriphCLKInitStruct.Xspi1ClockSelection = RCC_XSPI1CLKSOURCE_HCLK;

  /* XSPI2 kernel clock (ck_ker_xspi1) = HCLK =  200MHz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_XSPI2;
  RCC_PeriphCLKInitStruct.Xspi2ClockSelection = RCC_XSPI2CLKSOURCE_HCLK;

  if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
  {
    while (1);
  }
}

static void Security_Config()
{
  __HAL_RCC_RIFSC_CLK_ENABLE();
  RIMC_MasterConfig_t RIMC_master = {0};
  RIMC_master.MasterCID = RIF_CID_1;
  RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DMA2D, &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DCMIPP, &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC1 , &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC2 , &RIMC_master);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DMA2D , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_CSI    , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DCMIPP , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDC   , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL1 , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL2 , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

static void IAC_Config(void)
{
/* Configure IAC to trap illegal access events */
  __HAL_RCC_IAC_CLK_ENABLE();
  __HAL_RCC_IAC_FORCE_RESET();
  __HAL_RCC_IAC_RELEASE_RESET();
}

void IAC_IRQHandler(void)
{
  while (1)
  {
  }
}

/**
  * @brief  Implement transmission on UART function
  * @retval Transmitted data length
  */
#if defined (__ICCARM__)
int __write(int fd, char * ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t *) ptr, len, ~0);
  return len;
}
#elif   defined (__GNUC__)
int _write(int fd, char * ptr, int len)
{
  UNUSED(fd);
  HAL_UART_Transmit(&huart1, (uint8_t *) ptr, (uint16_t)len, ~(uint32_t)0);
  return len;
}
#endif /* (__GNUC__) */

/**
  * @brief  UART configuration for output Console
  * @retval None
  */
static void Console_Config(void)
{
  GPIO_InitTypeDef gpio_init;

  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

/* DISCO & NUCLEO USART1 (PE5/PE6) */
  gpio_init.Mode      = GPIO_MODE_AF_PP;
  gpio_init.Pull      = GPIO_PULLUP;
  gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio_init.Pin       = GPIO_PIN_5 | GPIO_PIN_6;
  gpio_init.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOE, &gpio_init);

  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = 115200;
  huart1.Init.Mode         = UART_MODE_TX_RX;
  huart1.Init.Parity       = UART_PARITY_NONE;
  huart1.Init.WordLength   = UART_WORDLENGTH_8B;
  huart1.Init.StopBits     = UART_STOPBITS_1;
  huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_8;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    while (1);
  }
}

/**
  * @brief  DCMIPP Clock Config for DCMIPP.
  * @param  hdcmipp DCMIPP Handle
  * @retval HAL_status
  */
HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);
  HAL_StatusTypeDef   status =  HAL_OK;
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

  /* Configure DCMIPP clocks */
  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DCMIPP;
  RCC_PeriphCLKInitStruct.DcmippClockSelection = RCC_DCMIPPCLKSOURCE_IC17;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockSelection = RCC_ICCLKSOURCE_PLL2;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockDivider = 3;
  if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
  {
    status = HAL_ERROR;
  }

  /* Configure CSI clocks */
  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CSI;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockDivider = 40;
  if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  return status;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(file);
  UNUSED(line);

  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  printf("FAIL on file %s on line %lu\r\n", file, line);
  __BKPT(0);
  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
}
