---
pagetitle: Release Notes for STM32N6xx CMSIS
lang: en
header-includes: <link rel="icon" type="image/x-icon" href="_htmresc/favicon.png" />
---

::: {.row}
::: {.col-sm-12 .col-lg-4}

<center>
# Release Notes for <mark>STM32N6xx CMSIS</mark>
Copyright &copy; 2024 STMicroelectronics\

[![ST logo](_htmresc/st_logo_2020.png)](https://www.st.com){.logo}
</center>

# Purpose

This driver provides the CMSIS device for the STM32N6xx product. This covers 

- STM32N657xx, STM32N655xx, STM32N647xx and STM32N645xx devices

This driver is composed of the description of the registers under "Include" directory.

Various template files are provided to easily build an application. They can be adapted to fit applications requirements.

- Templates/system_stm32n6xx.c contains the initialization code referred as SystemInit.
- Startup files are provided as example for EWARM&copy;, MDK-ARM&copy;, STM32CubeIDE&copy;.
- Linker files are provided as example for EWARM&copy;, MDK-ARM&copy;, STM32CubeIDE&copy;.

:::

::: {.col-sm-12 .col-lg-8}
# Update history



::: {.collapse}
<input type="checkbox" id="collapse-section3" checked aria-hidden="true">
<label for="collapse-section3" checked aria-hidden="true">__V1.2.0 / 04-June-2025__</label>
<div>

## Main Changes

###  Second maintenance release of **CMSIS devices drivers** for **STM32N6xx** series

## Contents

  - Fix Misra-C:2012 Rule 12.2 warnings in defining xxx_Pos constants as unsigned long
  - Fix typo affecting IWDG_IRQHandler definition in CubeIDE start-up files

## Development Toolchains and Compilers

- IAR Embedded Workbench for ARM (EWARM) toolchain V9.40.1
- RealView Microcontroller Development Kit (MDK-ARM) toolchain V5.39
- STM32CubeIDE 1.18.0

## Known Limitations

- None

## Dependencies

- None

## Notes

- None


</div>
:::




::: {.collapse}
<input type="checkbox" id="collapse-section2" aria-hidden="true">
<label for="collapse-section2" checked aria-hidden="true">__V1.1.0 / 05-February-2025__</label>
<div>

## Main Changes

###  First maintenance release of **CMSIS devices drivers** for **STM32N6xx** series

## Contents

  - Fix compensation cell registers SYSCFG VDDIOiCCCR/VDDIOiCCSR indices order
  - Remove useless FDCAN_Config_TypeDef structure
  - FPGA-related code removal  

## Development Toolchains and Compilers

- IAR Embedded Workbench for ARM (EWARM) toolchain V9.40.1
- RealView Microcontroller Development Kit (MDK-ARM) toolchain V5.39
- STM32CubeIDE 1.18.0

## Known Limitations

- None

## Dependencies

- None

## Notes

- None


</div>
:::


::: {.collapse}
<input type="checkbox" id="collapse-section1" aria-hidden="true">
<label for="collapse-section1" checked aria-hidden="true">__V1.0.0 / 30-October-2024__</label>
<div>

## First Release

###  First official release of **STM32CubeN6** firmware package supporting **STM32N6xx** devices

## Contents

  - First official release of **CMSIS devices drivers** for **STM32N6xx** series

## Development Toolchains and Compilers

- IAR Embedded Workbench for ARM (EWARM) toolchain V9.40.1
- RealView Microcontroller Development Kit (MDK-ARM) toolchain V5.39
- STM32CubeIDE 1.17.0

## Known Limitations

- None

## Dependencies

- None

## Notes

- None


</div>
:::


:::
:::

<footer class="sticky">
::: {.columns}
::: {.column width="95%"}
For complete documentation on STM32 Microcontrollers</mark>, visit: [[www.st.com/stm32](http://www.st.com/stm32)]{style="font-color: blue;"}

*This release note uses up to date web standards and, for this reason, should not be opened with Internet Explorer but preferably with popular browsers such as Google Chrome, Mozilla Firefox, Opera or Microsoft Edge.*
:::
::: {.column width="5%"}
<abbr title="Based on template cx566953 version 2.0">Info</abbr>
:::
:::
</footer>
