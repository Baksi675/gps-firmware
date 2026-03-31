# GPS Tracking Device — Firmware

Bare-metal firmware for a GPS tracking device built around the **STM32F401RE** Cortex-M4 microcontroller. Written entirely in C with no HAL or RTOS — all peripheral drivers and application subsystems are implemented from scratch.

---

## Features

- Real-time GPS data acquisition via the **u-blox NEO-6** module (NMEA parsing over USART6 at 9600 baud)
- Live display of location, date/time, movement, accuracy, and satellite data on a **128×64 SSD1309 OLED** (I2C)
- Three-button navigation UI with debounce and hold detection
- Automatic GPS data logging to a **microSD card** (FatFs, SPI) every 10 seconds
- UART serial console (USART1 at 115200 baud) for runtime inspection and CLI command dispatch
- RTC timekeeping backed by the LSE oscillator (32.768 kHz), preserved across resets via backup domain
- Structured logging system with per-module log levels and a compile-time enable gate
- Fully documented with Doxygen

---

## Documentation

The code documentation is available here: https://baksi675.github.io/gps-firmware/

---
