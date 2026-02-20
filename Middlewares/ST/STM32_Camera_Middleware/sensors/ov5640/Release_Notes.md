---
pagetitle: Release Notes for OV5640 Component Driver
lang: en
header-includes: <link rel="icon" type="image/x-icon" href="_htmresc/favicon.png" />
---

::: {.row}
::: {.col-sm-12 .col-lg-4}

<center>
# Release Notes for
# <mark>OV5640 Component Driver </mark>
Copyright &copy; 2019-2024 STMicroelectronics\

[![ST logo](_htmresc/st_logo_2020.png)](https://www.st.com){.logo}
</center>

# Purpose

This driver provides a set of camera functions offered by OV5640 component

:::

::: {.col-sm-12 .col-lg-8}

# Update History

::: {.collapse}
<input type="checkbox" id="collapse-section13" checked aria-hidden="true">
<label for="collapse-section13" aria-hidden="true">__V4.0.2-cmw-patch-2 / December-2025__</label>
<div>

## Main Changes

- Patch for Camera Middleware integration
- Fix bad ov5640 frame rate and Fps in mipi mode is 17.5 fps. Update OV5640_SC_PLL_CONTRL2 so it's 30 fps

</div>
:::

::: {.col-sm-12 .col-lg-8}

# Update History

::: {.collapse}
<input type="checkbox" id="collapse-section12" aria-hidden="true">
<label for="collapse-section11" aria-hidden="true">__V4.0.2 / 12-January-2024__</label>
<div>

## Main Changes

- Update Timing for 800x480 resolution
- Add 48 MHz Pixel clock support
- Correct return value for parallel mode config


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section11" aria-hidden="true">
<label for="collapse-section11" aria-hidden="true">__V4.0.1 / 08-December-2023__</label>
<div>			

## Main Changes

- Correct pclk clock setting at 9 and 12 MHz in OV5640_SetPCLK() function.

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section10" aria-hidden="true">
<label for="collapse-section10" aria-hidden="true">__V4.0.0 / 28-September-2023__</label>
<div>			

## Main Changes

- Add support of serial interface: 
  - OV5640_Init() must be called for either PARALLEL_MODE or SERIAL_MODE.


## Backward compatibility

-	This software breaks the compatibility with previous versions

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section9" aria-hidden="true">
<label for="collapse-section9" aria-hidden="true">__V3.2.4 / 27-June-2023__</label>
<div>			

## Main Changes

-   Update license


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section8" aria-hidden="true">
<label for="collapse-section8" aria-hidden="true">__V3.2.3 / 08-December-2022__</label>
<div>			

## Main Changes

-   Code Spell fixed


</div>
:::


::: {.collapse}
<input type="checkbox" id="collapse-section7" aria-hidden="true">
<label for="collapse-section7" aria-hidden="true">__V3.2.2 / 01-February-2022__</label>
<div>			

## Main Changes

-   Update license


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section6" aria-hidden="true">
<label for="collapse-section6" aria-hidden="true">__V3.2.1 / 30-April-2021__</label>
<div>			

## Main Changes

-   Formatting update


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section5" aria-hidden="true">
<label for="collapse-section5" aria-hidden="true">__V3.2.0 / 26-january-2021__</label>
<div>			

## Main Changes

-   Set default Pixel clock to 12Mhz
-   Add OV5640_SetPixelClock() API
-   Set HTS and VTS timings to get 7.5fps when using default PCLK=12MHz
-	Check Misra-C 2012 coding rules compliance
-   Formatting update


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section4"  aria-hidden="true">
<label for="collapse-section4" aria-hidden="true">__V3.1.0 / 11-December-2020__</label>
<div>			

## Main Changes

-	Add support of RGB888, YUV422, Y8 and JPEG formats
-   Add support gradual vertical colorbar
-	Codespell miss-spelling errors correction.


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section3" aria-hidden="true">
<label for="collapse-section3" aria-hidden="true">__V3.0.0 / 17-April-2020__</label>
<div>			

## Main Changes


-	Second Official Release of OV5640 Camera Component drivers in line with legacy BSP drivers development guidelines 
-	Add Support of the following features:
	-	OV5640 SetPolarities 
	-	OV5640 GetPolarities
	-	OV5640 ColorBar handling
	-	OV5640 Embedded Synchronization mode

## Dependencies

This software release is compatible with:

-	BSP Common version v6.0.0 or above

## Backward compatibility

-	This software breaks the compatibility with previous version v2.0.0
-	This software is compatible with version v1.0.0


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section2" aria-hidden="true">
<label for="collapse-section2" aria-hidden="true">__V2.0.0 / 07-February-2020__</label>
<div>			

## Main Changes

-	Official Release of OV5640 Camera Component drivers in line with legacy BSP drivers development guidelines  
-	The component drivers are composed of
	-	component core drivers files: ov5640.h/.c

## Dependencies

This software release is compatible with:

-	BSP Common version v5.1.2 or lower versions

## Backward compatibility

-	This software breaks the compatibility with previous version v1.0.0


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section1" aria-hidden="true">
<label for="collapse-section1" aria-hidden="true">__V1.0.0 / 30-0ctober-2019__</label>
<div>			

## Main Changes

-	First Official Release of OV5640 Camera Component drivers in line with STM32Cube BSP drivers development guidelines (UM2298)  
-	The component drivers are composed of
	-	component core drivers files: ov5640.h/.c
	-	component register drivers files: ov5640_regs.h/.c

## Dependencies

This software release is compatible with:

-	BSP Common v6.0.0 or above


</div>
:::

:::
:::

<footer class="sticky">
For complete documentation on <mark>STM32 Microcontrollers</mark> ,
visit: [[www.st.com](http://www.st.com/STM32)]{style="font-color: blue;"}
</footer>
