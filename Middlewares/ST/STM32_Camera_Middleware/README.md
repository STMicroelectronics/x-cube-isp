# Camera Middleware

## Software Architecture Document

- [Software Architecture Document](#software-architecture-document)
- [Purpose](#purpose)
- [Use cases](#use-cases)
- [Software architecture](#software-architecture)
- [Sensors drivers](#sensors-drivers)
- [APIs](#apis)
  - [Initialization](#initialization)
  - [Configure camera pipe helpers](#configure-camera-pipe-helpers)
  - [Configure camera pipe manually in your app](#configure-camera-pipe-manually-in-your-app)
  - [Start streaming sensors data](#start-streaming-sensors-data)
  - [Configure sensor characteristics](#configure-sensor-characteristics)
  - [Configure clocks](#configure-clocks)
  - [Callbacks](#callbacks)
  - [API driver sensors (Template)](#api-driver-sensors-template)

## Purpose

This document provides an overview of the Camera Middleware.
The Camera Middleware serves as an intermediary layer between camera sensor drivers and user applications. It provides a streamlined interface for initializing sensors, configuring camera pipelines, and managing data streaming. This middleware simplifies the development process for applications that require camera functionality by abstracting hardware-specific details.
`cmw_camera_conf_template.h` must be copied in user application as `cmw_camera_conf.h` with optional configuration update.

## Use cases

- **Sensor Initialization**: Developers can initialize a sensor with a single API call, simplifying the setup process.
- **Camera Characteristic Configuration**: Allows for the configuration of camera parameters such as brightness and saturation.
- **Dynamic Configuration**: The middleware supports on-the-fly reconfiguration of the DCMIPP to change settings without restarting the application.
- **Automatic Sensor Detection**: Proposes a mechanism for dynamic sensor detection, potentially with a default streaming configuration.
- **Camera Pipeline Helpers**: Provides simple APIs to assist in configuring the DCMIPP pipeline, such as setting resolution.
- **Multiple Camera Support**: Addresses the possibility of supporting multiple cameras, detailing configurations for different scenarios.
- **Sensor Capabilities Retrieval**: Enables applications to query the capabilities of the connected sensor.
- **ISP Configuration Management**: The middleware assumes control of ISP

## Software architecture

![Architecture](_htmresc/CMW_Architecture.png)

Supported sensors:
- `ST vd1943`
- `ST vd55g1`
- `ST vd56g3`
- `ST vd5943`
- `ST vd65g4`
- `ST vd66gy`
- `ov5640`
- `imx335`

**Note**: Some sensors use the STM32 ISP library, while others do not.

## Sensors drivers

Sensor drivers facilitate communication between the Camera Middleware and the physical camera sensors. The following camera modules are currently supported:

- MB1854B IMX335 (Default)
- ST VD66GY Camera module
- ST VD55G1 Camera module
- ST STEVAL-1943-MC1 Camera module
- OV5640 Camera module

## APIs

### Initialization

```C
typedef enum {
  CMW_UNKNOWN_Sensor = 0x0,
  CMW_VD66GY_Sensor,
  CMW_VD56G3_Sensor,
  CMW_IMX335_Sensor,
  CMW_OV5640_Sensor,
  CMW_VD55G1_Sensor,
  CMW_VD65G4_Sensor,
  CMW_VD1943_Sensor,
  CMW_VD5943_Sensor
} CMW_Sensor_Name_t;

typedef struct
{
  uint32_t pixel_format;  /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  int line_len;
} CMW_VD66GY_config_t;

typedef struct
{
  uint32_t pixel_format;  /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  int line_len;
} CMW_VD56G3_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD55G1_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD65G4_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD1943_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
  uint32_t CSI_PHYBitrate;
} CMW_VD5943_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
} CMW_IMX335_config_t;

typedef struct
{
  uint32_t pixel_format; /*!< This parameter can be a value from @ref CMW_PIXEL_FORMAT */
} CMW_OV5640_config_t;

typedef struct
{
  CMW_Sensor_Name_t selected_sensor;
  union {
    CMW_IMX335_config_t imx335_config;
    CMW_VD66GY_config_t vd66gy_config;
    CMW_VD56G3_config_t vd56g3_config;
    CMW_VD55G1_config_t vd55g1_config;
    CMW_VD65G4_config_t vd65g4_config;
    CMW_VD1943_config_t vd1943_config;
    CMW_VD5943_config_t vd5943_config;
    CMW_OV5640_config_t ov5640_config;
  } config_sensor;
} CMW_Advanced_Config_t;


typedef struct {
  /* Camera settings */
  uint32_t width;
  uint32_t height;
  int fps;
  int mirror_flip;
} CMW_CameraInit_t;

```

```C
/**
 * @brief  Fill the sensor configuration structure with default values.
 * @param  advanced_config  Pointer to the sensor configuration structure
 * @retval CMW status
 */
int32_t CMW_CAMERA_SetDefaultSensorValues(CMW_Advanced_Config_t *advanced_config);
```

```C
/**
  * @brief  Initializes the camera.
  * @param  initConf  Mandatory: General camera config
  * @param  advanced_config  Optional: Sensor specific configuration; NULL if you want to let CMW configure for you
  * @retval CMW status
  */
int32_t CMW_CAMERA_Init(CMW_CameraInit_t *init_conf, CMW_Advanced_Config_t *advanced_config);
int32_t CMW_CAMERA_DeInit();
```

```C
int32_t CMW_CAMERA_GetSensorName(CMW_Sensor_Name_t *sensorName);
```

### Configure camera pipe helpers

The framework enables an easy to use API to configure the resizing of your camera images

```C
typedef enum {
  CMW_Aspect_ratio_crop = 0x0,
  CMW_Aspect_ratio_fit,
  CMW_Aspect_ratio_fullscreen,
  CMW_Aspect_ratio_manual_roi,
} CMW_Aspect_Ratio_Mode_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t offset_x;
  uint32_t offset_y;
} CMW_Manual_roi_area_t;

typedef struct {
  /* pipe output settings */
  uint32_t output_width;
  uint32_t output_height;
  int output_format;
  int output_bpp;
  int enable_swap;
  int enable_gamma_conversion;
  /*Output buffer of the pipe*/
  int mode;
  /* You must fill manual_conf when mode is CMW_Aspect_ratio_manual_roi */
  CMW_Manual_roi_area_t manual_conf;
} CMW_DCMIPP_Conf_t;
```

```C
int32_t CMW_CAMERA_SetPipeConfig(uint32_t Pipe, DCMIPP_Conf_t *p_conf)
```

### Configure camera pipe manually in your app

```C
DCMIPP_HandleTypeDef* CMW_CAMERA_GetDCMIPPHandle();
```

### Start streaming sensors data

```C
/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff pointer to the camera output buffer
  * @param  mode  CMW_MODE_CONTINUOUS or CMW_MODE_SNAPSHOT
  * @retval CMW status
*/
int32_t CMW_CAMERA_Start(uint32_t pipe, uint8_t *pbuff, uint32_t Mode);
int32_t CMW_CAMERA_DoubleBufferStart(uint32_t pipe, uint8_t *pbuff1, uint8_t *pbuff2, uint32_t Mode);
```

```C
/**
  * @brief  Suspend the CAMERA capture on selected pipe
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Suspend(uint32_t pipe);
```

```C
/**
  * @brief  Resume the CAMERA capture on selected pipe
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Resume(uint32_t pipe);
```

### Configure sensor characteristics

Those APIs is not available for every sensors

```C
/**
  * @brief  Run the Camera driver run. Usually run the ISP process.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Run();

/**
  * @brief  Set White Balance mode.
  * @param  Automatic  If not null, set automatic white balance mode
  * @param  RefColorTemp  If automatic is null, set white balance mode
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetWBRefMode(uint8_t Automatic, uint32_t RefColorTemp);
/**
  * @brief  Get White Balance reference modes list.
  * @param  RefColorTemp  White Balance reference modes
  * @retval CMW status
  */
int32_t CMW_CAMERA_ListWBRefModes(uint32_t RefColorTemp[]);

int CMW_CAMERA_SetAntiFlickerMode(int flicker_mode);
int CMW_CAMERA_GetAntiFlickerMode(int *flicker_mode);

int CMW_CAMERA_SetBrightness(int Brightness);
int CMW_CAMERA_GetBrightness(int *Brightness);

int CMW_CAMERA_SetContrast(int Contrast);
int CMW_CAMERA_GetContrast(int *Contrast);

int CMW_CAMERA_SetGain(int32_t Gain);
int CMW_CAMERA_GetGain(int32_t *Gain);

int CMW_CAMERA_SetExposure(int32_t exposure);
int CMW_CAMERA_GetExposure(int32_t *exposure);

int32_t CMW_CAMERA_SetMirrorFlip(int32_t MirrorFlip);
int32_t CMW_CAMERA_GetMirrorFlip(int32_t *MirrorFlip);

int32_t CMW_CAMERA_SetExposureMode(int32_t exposureMode);
int32_t CMW_CAMERA_GetExposureMode(int32_t *exposureMode);

int32_t CMW_CAMERA_SetTestPattern(int32_t mode);
int32_t CMW_CAMERA_GetTestPattern(int32_t *mode);

int32_t CMW_CAMERA_GetSensorInfo(ISP_SensorInfoTypeDef *info);

/**
  * @brief  Enable the Restart State. When enabled, at system restart, the ISP middleware configuration
  *         is restored from the last update before the restart.
  * @param  ISP_RestartState pointer to ISP Restart State. To use this mode in a Low Power use case, where
  *         the ISP state is applied at system wake up, this pointer must be in some retention memory.
  * @retval CMW status
  */
int32_t CMW_CAMERA_EnableRestartState(ISP_RestartStateTypeDef *ISP_RestartState);
/**
  * @brief  Disable the Restart State
  * @retval CMW status
  */
int32_t CMW_CAMERA_DisableRestartState();
```

### Configure clocks

```C
HAL_StatusTypeDef MX_DCMIPP_Init(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Instance);
```

### Callbacks

```C
  int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe);
  int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t pipe);
  void CMW_CAMERA_PIPE_ErrorCallback(uint32_t pipe);
```

### API driver sensors (Template)

```C
#define ISP_SENSOR_INFO_MAX_LENGTH      (32U)

typedef struct
{
  char name[ISP_SENSOR_INFO_MAX_LENGTH];
  uint8_t bayer_pattern;
  uint8_t color_depth;
  uint32_t width;
  uint32_t height;
  uint32_t gain_min;
  uint32_t gain_max;
  uint32_t again_max;
  uint32_t exposure_min;
  uint32_t exposure_max;
} ISP_SensorInfoTypeDef;

typedef struct
{
  uint32_t width;
  uint32_t height;
  int fps;
  uint32_t mirrorFlip;
  void *sensor_config; /* to pass specific config from application side*/
} CMW_Sensor_Init_t;

typedef struct
{
  int32_t (*Init)(void *, CMW_Sensor_Init_t *);
  int32_t (*DeInit)(void *);
  int32_t (*Start)(void *);
  int32_t (*Run)(void *);
  void    (*VsyncEventCallback)(void *, uint32_t);
  void    (*FrameEventCallback)(void *, uint32_t);
  int32_t (*Stop)(void *);
  int32_t (*ReadID)(void *, uint32_t *);
  int32_t (*SetBrightness)(void *, int32_t);
  int32_t (*SetSaturation)(void *, int32_t);
  int32_t (*SetContrast)(void *, int32_t);
  int32_t (*SetMirrorFlip)(void *, uint32_t);
  int32_t (*SetResolution)(void *, uint32_t);
  int32_t (*GetResolution)(void *, uint32_t *);
  int32_t (*SetPixelFormat)(void *, uint32_t);
  int32_t (*GetPixelFormat)(void *, uint32_t *);
  int32_t (*SetFrequency)(void *, int32_t);
  int32_t (*SetFramerate)(void*, int32_t);
  int32_t (*SetGain)(void *, int32_t);
  int32_t (*SetExposure)(void *, int32_t);
  int32_t (*SetExposureMode)(void *, int32_t);
  int32_t (*SetWBRefMode)(void *, uint8_t, uint32_t);
  int32_t (*ListWBRefModes)(void *, uint32_t[]);
  int32_t (*SetFlickerMode)(void *, int32_t);
  int32_t (*GetSensorInfo)(void *, ISP_SensorInfoTypeDef *);
  int32_t (*SetTestPattern)(void *, int32_t);
  int32_t (*GetDefaultPHYBitrate)(void *, int32_t *); /* in bps */
} CMW_Sensor_if_t;

```
