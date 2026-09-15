# STM32 ISP Library

![latest tag](https://img.shields.io/badge/tag-2.1.0-blue)

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

- **Support for camera > 5 MP**
- **New tuning parameters** are available for:
  - VD65G4
  - VD66GY
  - VD1943
  - IMX477 (New supported Camera)
- Update to HAL 1.4.0
- **Enhance 2A algos with a new convergence speed parameter**
- Double buffering to avoid tearing
- Add Preview app which only shows an example on how to use the CMW + ISP in a simple application
- Some Bug fixes
- Use An FSBL for boot from flash mode (see [Doc/Boot-Overview.md](Doc/Boot-Overview.md))

## Compatibility

Compatible with STM32 ISP IQTune 2.1.0 (No backward compatibility with previous STM32 ISP IQTune version).

## Known Issues and Limitations

None

## STM32 ISP IQTune desktop application

<https://www.st.com/en/development-tools/stm32-isp-iqtune.html>

## STM32 ISP Wiki documentation

<https://wiki.st.com/stm32mcu/wiki/Category:ISP>

## STM32 ISP tuning procedure

<https://wiki.st.com/stm32mcu/wiki/ISP:How_to_tune_ISP_using_the_STM32_ISP_IQTune>