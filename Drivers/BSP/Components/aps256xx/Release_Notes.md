---
pagetitle: Release Notes for APS256XX 16-Bits OCTOSPI Component Driver
lang: en
header-includes: <link rel="icon" type="image/x-icon" href="_htmresc/favicon.png" />
---
::: {.row}
::: {.col-sm-12 .col-lg-4}

<center>
# Release Notes for <mark>APS256XX</mark> Component Driver
Copyright &copy; 2020 STMicroelectronics\

[![ST logo](_htmresc/st_logo_2020.png)](https://www.st.com){.logo}
</center>

# Purpose

This driver provides a set of functions offered by APS256XX 16-Bits OCTOSPI component

:::

::: {.col-sm-12 .col-lg-8}
# Update History

::: {.collapse}
<input type="checkbox" id="collapse-section6" checked aria-hidden="true">
<label for="collapse-section6" aria-hidden="true">__V1.0.6 / 08-December-2023__</label>
<div>

## Main Changes

- Set correct bit in MR6 register to enter power down
- Correct MISRA-C 2012 warning

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section5" aria-hidden="true">
<label for="collapse-section5" aria-hidden="true">__V1.0.5 / 12-December-2022__</label>
<div>

## Main Changes

- Add XSPI_CCR_SIOO macro

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section4" aria-hidden="true">
<label for="collapse-section4" aria-hidden="true">__V1.0.4 / 09-February-2022__</label>
<div>			

## Main Changes

-	Update license

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section4" aria-hidden="true">
<label for="collapse-section4" aria-hidden="true">__V1.0.3 / 06-October-2021__</label>
<div>			

## Main Changes

-	Initialize sCommand structure as 0 in “Enable Memory mapped” API to avoid Issues related to non-filled sCommand field

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section3" aria-hidden="true">
<label for="collapse-section3" aria-hidden="true">__V1.0.2 / 23-September-2021__</label>
<div>			

## Main Changes

-	Code formatting and spelling

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section2" aria-hidden="true">
<label for="collapse-section2" aria-hidden="true">__V1.0.1 / 30-April-2021__</label>
<div>			

## Main Changes

-	Code formatting and spelling
-	Update memory Command configuration values

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section1" aria-hidden="true">
<label for="collapse-section1" aria-hidden="true">__V1.0.0 / 27-March-2020__</label>
<div>			

## Main Changes

-	First Official Release of APS256XX 16-bits SPI Component drivers in line with STM32Cube BSP drivers development guidelines (UM2298) 
-	The component drivers are composed of
	-	component core drivers files: aps256xx.h/.c
	-	component configuration file: aps256xx_conf_template.h

</div>
:::

:::
:::

<footer class="sticky">
For complete documentation on <mark>STM32 Microcontrollers</mark> ,
visit: [[www.st.com](http://www.st.com/STM32)]{style="font-color: blue;"}
</footer>
