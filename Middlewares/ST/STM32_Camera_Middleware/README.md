# Camera Middleware

- [Software architecture](#software-architecture)
- [Purpose](#purpose)
- [Sensors drivers](#sensors-drivers)
- [ISP chaining configuration](#isp-chaining-configuration)
- [APIs](#apis)
  - [Initialization](#initialization)
    - [Initialization flow](#initialization-flow)
  - [Configure camera pipeline - DCMIPP](#configure-camera-pipeline---dcmipp)
  - [Configure camera pipe manually in your app](#configure-camera-pipe-manually-in-your-app)
  - [Start streaming sensors data](#start-streaming-sensors-data)
  - [Configure sensor characteristics](#configure-sensor-characteristics)
  - [Configure clocks](#configure-clocks)
  - [Callbacks](#callbacks)

Specific Doc:

- [How to add new sensor](Doc/How-to-add-new-sensor.md)

## Software architecture

![Architecture](_htmresc/CMW_Architecture.png)

Supported sensors:

- `IMX335`
- `VD66GY`
- `VD56G3`
- `OV5640`
- `VD55G1`
- `VD65G4`
- `VD1943`
- `VD5943`
- `IMX477`

**Note**: Some sensors use the STM32 ISP library, while others do not.

## Purpose

This document provides an overview and usage guide for the Camera Middleware (CMW).
The Camera Middleware is a hardware abstraction layer between camera sensor
drivers and user applications. It provides a unified interface for:

- Initializing and configuring camera sensors
- Managing camera pipelines and streaming
- Handling sensor-specific features (exposure, white balance, etc.)
- Abstracting hardware and board-specific details

**Quick Start:**

1. **Copy the configuration template:**

- Copy `cmw_camera_conf_template.h` to your application as `cmw_camera_conf.h`.
- Edit this file to enable/disable sensors and set board-specific includes.

2. **Include and use the middleware:**

- Include `cmw_camera.h` in your application.
- Use the APIs described below to initialize and control the camera.

**Note:** Not all APIs are supported by every sensor. See the sensor driver or
 use `CMW_CAMERA_GetSensorInfo()` to query capabilities at runtime.

## Sensors drivers

Sensor drivers are located in the `sensors/` directory. Each supported sensor
has its own driver files (e.g., `cmw_imx335.c`, `cmw_ov5640.c`).

**Currently supported camera modules:**

- MB1854B IMX335 (Default)
- ST VD66GY Camera module
- ST VD55G1 Camera module
- ST VD56G3 Camera module
- ST VD1943 Camera module
- ST VD5943 Camera module
- ST VD65G4 Camera module
- OV5640 Camera module
- Raspberry Pi Official HQ Camera - IMX477

To add support of a new sensor, you can follow the [How to add new sensor](Doc/How-to-add-new-sensor.md) documentation.

## ISP chaining configuration

By default, CMW automatically chains the ISP middleware during its workflow.
This can be disabled if you want to handle ISP from the application or if you do not want to use ISP at all.

`cmw_camera.h` includes `cmw_camera_conf.h` (provided by the application, usually copied from `cmw_camera_conf_template.h`).

To disable the automatic ISP chaining from CMW, add in `Inc/cmw_camera_conf.h`:

```c
#define CMW_USE_WITHOUT_ISP
```

When the ISP is chained by CMW, its auto-exposure / auto-white-balance algorithms run in
`CMW_CAMERA_Run()`. The application must therefore call `CMW_CAMERA_Run()` periodically
(typically once per frame or from the main loop):

```c
/**
  * @brief  Run the camera middleware periodic processing (e.g. ISP run algorithms).
  * @retval CMW status
  */
int32_t CMW_CAMERA_Run();

/**
  * @brief  Enable the Restart State. When enabled, at system restart, the ISP middleware configuration
  *         is restored from the last update before the restart.
  * @param  ISP_restart_state pointer to ISP Restart State. To use this mode in a Low Power use case, where
  *         the ISP state is applied at system wake up, this pointer must be in some retention memory.
  * @retval CMW status
  */
int32_t CMW_CAMERA_EnableRestartState(ISP_RestartStateTypeDef *ISP_restart_state);

/**
  * @brief  Disable the Restart State
  * @retval CMW status
  */
int32_t CMW_CAMERA_DisableRestartState();
```

**Note:** `CMW_CAMERA_Run()`, `CMW_CAMERA_EnableRestartState()` and
`CMW_CAMERA_DisableRestartState()` are only available/relevant when the ISP is chained
(i.e. `CMW_USE_WITHOUT_ISP` is not defined).

## APIs

### Initialization

**Sensor selection and configuration:**

```c
typedef struct {
  const char *sensor_name;   // Sensor name string (see registry, NULL for auto-probe)
  void *sensor_config;       // Pointer to sensor-specific config struct (see below)
} CMW_Advanced_Config_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  int fps;
  CMW_MirrorFlip_t mirror_flip;
#if defined (CMW_USE_WITHOUT_ISP)
  int isp_decimation_ratio_h;
  int isp_decimation_ratio_v;
#endif
} CMW_CameraInit_t;

/**
 * @brief  Fill the sensor configuration structure with default values.
 * @param  advanced_config  Pointer to the sensor configuration structure
 * @retval CMW status
 */
int32_t CMW_CAMERA_SetDefaultSensorValues(CMW_Advanced_Config_t *advanced_config);

/**
  * @brief  Initializes the camera.
  * @param  init_conf  Mandatory: General camera config
  * @param  advanced_config  Optional: Sensor specific configuration; NULL if you want to let CMW configure for you
  * @retval CMW status
  */
int32_t CMW_CAMERA_Init(CMW_CameraInit_t *init_conf, CMW_Advanced_Config_t *advanced_config);


/**
  * @brief  DeInitializes the camera.
  *         The user must call CMW_CAMERA_Stop if the camera is started before calling this function.
  * @retval CMW status
  */
int32_t CMW_CAMERA_DeInit();

/**
  * @brief  Get Sensor name.
  * @param  sensor_name  Camera sensor name
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetSensorName(const char **sensor_name);
```

#### Initialization flow

**Typical initialization flow - Camera probing**

```c
CMW_CameraInit_t cam_init =
{
  .width = 1920,
  .height = 1080,
  .fps = 30,
  .mirror_flip = CMW_MIRRORFLIP_NONE,
};

CMW_CAMERA_Init(&cam_init, NULL);
```

When you pass `NULL` for `advanced_config` parameter, the middleware will
auto-detect and configure the sensor if possible.

**Advanced Initialization Flow**

```c
CMW_IMX335_config_t imx335_cfg;
CMW_CameraInit_t cam_init =
{
  .width = 1920,
  .height = 1080,
  .fps = 30,
  .mirror_flip = CMW_MIRRORFLIP_NONE,
};

CMW_Advanced_Config_t adv_conf =
{
  .sensor_name = "IMX335",
  .sensor_config = &imx335_cfg
};
CMW_CAMERA_SetDefaultSensorValues(&adv_conf); // Optional: fill config with defaults
imx335_cfg.pixel_format = CMW_PIXEL_FORMAT_RAW10;

CMW_CAMERA_Init(&cam_init, &adv_conf);
```

### Configure camera pipeline - DCMIPP

The middleware provides helpers to configure the DCMIPP camera pipeline (resize, crop, format, etc.):

```c
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

/**
  * @brief  Configure a DCMIPP pipe output (crop, decimation, downsize, format).
  * @param  pipe  DCMIPP Pipe
  * @param  p_conf  Pointer to the pipe output configuration
  * @param  pitch  Pointer that receives the resulting pipe pitch (for buffer alignment)
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetPipeConfig(uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch);
```

### Configure camera pipe manually in your app

```C
/**
  * @brief  Get the DCMIPP handle used by the camera middleware.
  * @retval Pointer to the DCMIPP handle
  */
DCMIPP_HandleTypeDef* CMW_CAMERA_GetDCMIPPHandle();
```

### Start streaming sensors data

```c
/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff pointer to the camera output buffer
  * @param  mode  CMW_CAPTUREMODE_CONTINUOUS or CMW_CAPTUREMODE_SNAPSHOT
  * @retval CMW status
  */
int32_t CMW_CAMERA_Start(uint32_t pipe, uint8_t *pbuff, CMW_CaptureMode_t mode);

/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff1 pointer to the first camera output buffer
  * @param  pbuff2 pointer to the second camera output buffer
  * @param  mode  CMW_CAPTUREMODE_CONTINUOUS or CMW_CAPTUREMODE_SNAPSHOT
  * @retval CMW status
  */
int32_t CMW_CAMERA_DoubleBufferStart(uint32_t pipe, uint8_t *pbuff1, uint8_t *pbuff2, CMW_CaptureMode_t mode);

/**
  * @brief  Suspend the CAMERA capture on selected pipe
  *         Do not stop the camera, just suspend the selected pipe.
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Suspend(uint32_t pipe);

/**
  * @brief  Resume the CAMERA capture on selected pipe
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Resume(uint32_t pipe);

/**
  * @brief  Stops the camera stream and all the pipes.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Stop(void);
```

### Configure sensor characteristics

**Note:** Not all sensors support all features below. Use `CMW_CAMERA_GetSensorInfo()` to
query capabilities at runtime.

```c
/**
  * @brief  Set White Balance mode. (Only relevant when the ISP is chained.)
  * @param  automatic  If not null, set automatic white balance mode
  * @param  ref_color_temp  If automatic is null, set white balance mode
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetWBRefMode(uint8_t automatic, uint32_t ref_color_temp);

/**
  * @brief  Get White Balance reference modes list. (Only relevant when the ISP is chained.)
  * @param  ref_color_temp  White Balance reference modes. Must point to an array
  *                         of at least CMW_CAMERA_NB_WB_REF_MODES entries.
  * @param  array_size      Number of entries available in ref_color_temp
  * @retval CMW status
  */
int32_t CMW_CAMERA_ListWBRefModes(uint32_t ref_color_temp[], uint32_t array_size);

/**
  * @brief  Set the camera gain.
  * @param  gain     Gain in mdB
  * @retval CMW status
  */
int CMW_CAMERA_SetGain(int32_t gain);

/**
  * @brief  Get the camera gain.
  * @param  gain     Gain in mdB
  * @retval CMW status
  */
int CMW_CAMERA_GetGain(int32_t *gain);

/**
  * @brief  Set the camera exposure.
  * @param  exposure exposure in microseconds
  * @retval CMW status
  */
int CMW_CAMERA_SetExposure(int32_t exposure);

/**
  * @brief  Get the camera exposure.
  * @param  exposure exposure in microseconds
  * @retval CMW status
  */
int CMW_CAMERA_GetExposure(int32_t *exposure);

/**
  * @brief  Set the camera Mirror/Flip.
  * @param  mirror_flip CMW_MIRRORFLIP_NONE CMW_MIRRORFLIP_FLIP CMW_MIRRORFLIP_MIRROR CMW_MIRRORFLIP_FLIP_MIRROR
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetMirrorFlip(CMW_MirrorFlip_t mirror_flip);

/**
  * @brief  Get the camera Mirror/Flip.
  * @param  mirror_flip CMW_MIRRORFLIP_NONE CMW_MIRRORFLIP_FLIP CMW_MIRRORFLIP_MIRROR CMW_MIRRORFLIP_FLIP_MIRROR
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetMirrorFlip(CMW_MirrorFlip_t *mirror_flip);

/**
  * @brief  Set the camera exposure mode.
  * @param  exposure_mode Exposure mode CMW_EXPOSUREMODE_AUTO, CMW_EXPOSUREMODE_AUTOFREEZE, CMW_EXPOSUREMODE_MANUAL
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetExposureMode(CMW_ExposureMode_t exposure_mode);

/**
  * @brief  Get the camera exposure mode.
  * @param  exposure_mode Exposure mode CMW_EXPOSUREMODE_AUTO, CMW_EXPOSUREMODE_AUTOFREEZE, CMW_EXPOSUREMODE_MANUAL
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetExposureMode(CMW_ExposureMode_t *exposure_mode);

/**
  * @brief  Set (Enable/Disable and Configure) the camera test pattern
  * @param  mode Pattern mode (sensor specific value) to be configured. '-1' means disable.
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetTestPattern(int32_t mode);

/**
  * @brief  Get the camera test pattern
  * @param  mode Pattern mode (sensor specific value) to be returned. '-1' means disable.
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetTestPattern(int32_t *mode);

/**
  * @brief  Get the Camera Sensor info.
  * @param  info  pointer to sensor info
  * @note   This function should be called after the init. This to get Capabilities
  *         from the camera sensor
  * @retval Component status
  */
int32_t CMW_CAMERA_GetSensorInfo(CMW_Sensor_Info_t *info);
```

**Exposure mode values:**

```c
typedef enum {
  CMW_EXPOSUREMODE_AUTO       = 0x00U, // Auto exposure
  CMW_EXPOSUREMODE_AUTOFREEZE = 0x01U, // Freeze current auto value
  CMW_EXPOSUREMODE_MANUAL     = 0x02U, // Manual (software-controlled)
} CMW_ExposureMode_t;
```

### Configure clocks

```c
/**
  * @brief  DCMIPP Clock Config for DCMIPP.
  * @param  hdcmipp  DCMIPP Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL status
  */
HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp);
```

### Callbacks

Implement these callbacks in your application to handle camera events:

```c
/**
  * @brief  Frame Event callback on pipe
  * @param  pipe  Pipe receiving the callback
  * @retval CMW status
  */
int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe);

/**
  * @brief  Vsync Event callback on pipe
  * @param  pipe  Pipe receiving the callback
  * @retval CMW status
  */
int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t pipe);

/**
  * @brief  Error callback on pipe
  * @param  pipe  Pipe receiving the callback
  * @retval None
  */
void CMW_CAMERA_PIPE_ErrorCallback(uint32_t pipe);
```
