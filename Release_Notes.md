# Release Notes for X-CUBE-ISP
Copyright &copy; 2025 STMicroelectronics

## Purpose

The X-CUBE-ISP package provides the functionnality to control and tune the
DCMIPP ISP present in the STM32N6xx products.<br>
It contains:<br>
- An embedded ISP Library middleware (running on the target) hosting 2A
  algorithms (Auto Exposure and Auto White Blance) and mechanisms to control
  the ISP and load sensor tuning file.<br>
`x-cube-isp/Middlewares/ST/STM32_ISP_Library`
-  A tuning application (running on the target) that allows to interact
  with the STM32 ISP IQTune desktop application executed on the host computer
  and connected through usb.<br>
`x-cube-isp/Projects/DCMIPP_ISP/ISP_IQTune_App`

To take control of the DCMIPP ISP:<br>
- The host computer must execute the STM32 ISP IQTune desktop application and
connect to the target thanks to an USB-C cable on the USB1 (CN18) connector of
the board.
- The ISP IQTune App must be loaded and launched on the target.

## Key Features

- Control of all ISP parameters via desktop application
- Image analysis function for camera sensor tuning
- Generation of sensor ISP tuning file
- Embedded Auto Exposure algorithm (AE)
- Embedded Auto White Balance algorithm (AWB)
- STM32 ISP IQTune application

## Update history

### V2.0.0 / February 2026
- Update STM32N6 HAL, CMSI and BSP to the version 1.3.0
- **New 2A algorithms**:
  - The **Auto-Exposure (AE)** algorithm is now based on lux estimation, providing a faster and more stable approach to achieve the luminance target.
  - The **Auto White Balance (AWB)** algorithm now uses color ratios to reach color accuracy more quickly and with greater stability. Additionally, this new algorithm offers improved rendering between two profiles through interpolation.
- **New tuning parameters** are now required to run the new 2A algorithms. These parameters are avalaible for the following list of sensors:
  - IMX335
  - VD66GY
  - VD5943 (MONO)
  - VD1943
  - VD65G4
  - VD56G3 (MONO)
- **UVC feature**:
  - Support of USBx and Threadx
  - Camera preview is now displayed using UVC live streaming

### V1.2.0 / July 2025
- Update STM32N6 HAL, CMSI and BSP to the version 1.2.0
- Support unique Gamma
- Fix support of MacOS
- Fix memory issues for 5Mp sensor
- Fix build warnings

### V1.1.0 / July 2025
- Update STM32N6 HAL, CMSI and BSP to the version 1.1.0
- Add sensor delay feature that can be manually set or automaticaly computed thanks to the STM32-ISP-IQTune desktop application
- Improve AWB algorithm to avoid flickering between 2 illuminants
- Add AEC anti-flicker feature that support 50Hz and 60Hz region.
- Statistic area has been remove from init parameter of the ISP_Init() function
- Fix IAR build warnings in the evision libraries

### V1.0.0 / December 2024
 - Initial release

## Software components
| Name                         | Version
|-----                         | -------
| CMSIS                        | 5.9.0
| STM32N6xx CMSIS Device       | 1.3.0
| STM32N6xx HAL/LL Drivers     | 1.3.0
| STM32N6570-DK BSP Drivers    | 1.3.0
| BSP Component aps256xx       | 1.0.6
| BSP Component Common         | 7.3.0
| BSP Component mx66uw1g45g    | 1.1.0
| BSP Component rk050hr18      | 1.0.1
| STM32 Camera Middleware      | 1.5.1
| Azure RTOS USBX for STM32    | 6.4.0_rel / ST modified 251022
| Azure RTOS Threadx for STM32 | 6.4.0_rel / ST modified 250822
| STM32 ISP Library            | 2.0.0
| ISP IQTune application       | 2.0.0
