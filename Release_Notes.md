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

## Software components
| Name                         | Version
|-----                         | -------
| CMSIS                        | v5.9.0
| STM32N6xx CMSIS Device       | v1.2.0
| STM32N6xx HAL/LL Drivers     | v1.2.0
| STM32N6570-DK BSP Drivers    | v1.2.0
| BSP Component aps256xx       | v1.0.6
| BSP Component Common         | v7.3.0
| BSP Component mx66uw1g45g    | v1.1.0
| BSP Component rk050hr18      | v1.0.1
| STM32 Camera Middleware      | v1.4.2
| STM32 USB Device Library     | v2.11.3
| STM32 ISP Library            | v1.2.0
| ISP IQTune application       | v1.3.0

## Update history

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
