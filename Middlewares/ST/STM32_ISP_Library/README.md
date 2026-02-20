# STM32 ISP Library

![latest tag](https://img.shields.io/badge/tag-2.0.0-blue)

The ISP Library middleware (running on the target) hosts 2A algorithms
(Auto Exposure and Auto White Balance) and mechanisms to control the
ISP and load sensor ISP tuning file.

The STM32 ISP Library also implements a communication mechanism with the
STM32 ISP IQTune desktop application that provide services to tune the
ISP present in STM32 devices.
To activate this communication through USB link, the compilation flag
ISP_MW_TUNING_TOOL_SUPPORT must be enabled.

## Structure
- isp: core of the ISP Library with the ISP parameter configuration
- isp_param_conf: collection of sensor tuning parameters

## Enhancements, new features
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
- **UVC streaming** is now supported for a better STM32 ISP IQTune user experience

## Compatibility
Compatible with STM32 ISP IQTune 2.0.0 (No backward compatibility with previous STM32 ISP IQTune version).

## Known Issues and Limitations
None

## STM32 ISP IQTune desktop application
<https://www.st.com/en/development-tools/stm32-isp-iqtune.html>

## STM32 ISP Wiki documentation
<https://wiki.st.com/stm32mcu/wiki/Category:ISP>

## STM32 ISP tuning procedure
<https://wiki.st.com/stm32mcu/wiki/ISP:How_to_tune_ISP_using_the_STM32_ISP_IQTune>