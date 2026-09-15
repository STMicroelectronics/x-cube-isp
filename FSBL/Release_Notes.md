# Release Notes for ai_fsbl.hex

## Update history

### v1.4.0 / November 2025

- Use STM32CubeN6 Firmware Package 1.3.0
- Before compiling append BOOT_GetApplicationSize() implementation that return correct size for a boot header v2.3 to
  extmem.c (since such implementation has been removed from delivered extmem.c).

### v1.3.0 / October 2025

- Use STM32CubeN6 Firmware Package 1.2.0

### v1.2.0 / November 2024

- Use STM32CubeN6 Firmware Package 1.0.0

### v1.1.0 / September 2024

- Switch to hex file

### v1.0.0 / September 2024

- Initial Version
