---
pagetitle: Release Notes for STM32N6570-DK
lang: en
header-includes: <link rel="icon" type="image/x-icon" href="_htmresc/favicon.png" />
---

::: {.row}
::: {.col-sm-12 .col-lg-4}

<center>
# Release Notes for
# <mark>STM32N6570-DK</mark>
Copyright &copy; 2024 STMicroelectronics\
    
[![ST logo](_htmresc/st_logo_2020.png)](https://www.st.com){.logo}
</center>

# Purpose

This driver provides a set of functions to manage:

- LEDs, push-button, Virtual Com Port (VCP), Audio, USB-PD, LCD, camera sensor and external memories access on STM32N6570-DK Kit from STMicroelectronics

:::

::: {.col-sm-12 .col-lg-8}
# Update History


::: {.collapse}
<input type="checkbox" id="collapse-section3" checked aria-hidden="true">
<label for="collapse-section3" checked aria-hidden="true">__V1.2.0 / 04-June-2025__</label>
<div>

## Main Changes

###  Second maintenance release of board drivers supporting **STM32N6570-DK board** (MB1939C)


## Contents

  - Fix STM32N6570_DK_BSP_VERSION_MAIN/SUB1/SUB2/RC constants definitions to correct Misra-C:2012 R12.2 warnings


## Known Limitations

  - None

## Dependencies

  - ISP library v1.0.2

## Notes


*stm32n6570_discovery_conf_template.h* file must be copied in user application as *stm32n6570_discovery_conf.h* with optional configuration update


\

</div>
:::


::: {.collapse}
<input type="checkbox" id="collapse-section2" aria-hidden="true">
<label for="collapse-section2" checked aria-hidden="true">__V1.1.0 / 05-February-2025__</label>
<div>

## Main Changes

###  First maintenance release of board drivers supporting **STM32N6570-DK board** (MB1939C)


## Contents

  - Set NOR FLASH XSPI IO speed from GPIO_SPEED_FREQ_HIGH to GPIO_SPEED_FREQ_VERY_HIGH
  - Optimize PSRAM access parameters in MX_XSPI_RAM_Init()
  - Increase SD_WRITE_TIMEOUT and SD_READ_TIMEOUT 
  - Correct time-out management in BSP_SD_WriteBlocks()
  - Correct BSP_SD_Init() to fix initialization issue in forcing then releasing SDMMC instances reset
  - Align HAL_DCMIPP_PIPE_VsyncEventCallback() with latest ISP library tag
  - Add BSP camera APIs BSP_CAMERA_FullPlanarStart(), BSP_CAMERA_SemiPlanarStart(), BSP_CAMERA_FullPlanarDoubleBufferStart() and BSP_CAMERA_SemiPlanarDoubleBufferStart()


## Known Limitations

  - None

## Dependencies

  - ISP library v1.0.2

## Notes


*stm32n6570_discovery_conf_template.h* file must be copied in user application as *stm32n6570_discovery_conf.h* with optional configuration update


\

</div>
:::


::: {.collapse}
<input type="checkbox" id="collapse-section1"  aria-hidden="true">
<label for="collapse-section1" checked aria-hidden="true">__V1.0.0 / 30-October-2024__</label>
<div>

## First Release

###  First official release of **STM32CubeN6** firmware package supporting **STM32N6xx** devices

## Contents

  - First official release of board drivers for **STM32N6570-DK board** (MB1939C)
    - in line with STM32Cube BSP drivers development guidelines (UM2298 - revision 2)


## Known Limitations

None

## Dependencies

None

## Notes

*stm32n6570_discovery_conf_template.h* file must be copied in user application as *stm32n6570_discovery_conf.h* with optional configuration update

</div>
:::


:::
:::

<footer class="sticky">
::: {.columns}
::: {.column width="95%"}
For complete documentation on STM32,
visit: [www.st.com](http://www.st.com)

*This release note uses up to date web standards and, for this reason, should not be opened with Internet Explorer but preferably with popular browsers such as Google Chrome, Mozilla Firefox, Opera or Microsoft Edge.*
:::
::: {.column width="5%"}
<abbr title="Based on template cx566953 version 2.0">Info</abbr>
:::
:::
</footer>
