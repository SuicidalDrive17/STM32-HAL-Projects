# 07 Multi-Protocol Telemetry Platform

## Overview
This project is a real-time telemetry platform built on the STM32L476RG microcontroller. It demonstrates the seamless integration of multiple hardware protocols (I2C, SPI, UART with DMA) and edge computing (Sensor Fusion). The system calculates physical orientation data (Roll and Pitch) from raw IMU sensors and visualizes it dynamically on both a local TFT display and a PC-based Ground Station.

## System Architecture
The repository is divided into two main components:
1. **Firmware (C):** STM32 code that reads raw acceleration and gyroscope data from an MPU9250 IMU via I2C. It utilizes the hardware FPU (Floating Point Unit) and `<math.h>` to perform Sensor Fusion. The calculated data is drawn on an ST7735 TFT display via SPI and simultaneously streamed as a CSV payload over UART using DMA for zero-CPU-overhead transmission.
2. **Software (Python):** A desktop Ground Station application utilizing `pyserial` and `matplotlib` to capture the incoming CSV stream and render real-time, zero-latency animated charts of the orientation data.

## Hardware Connections

### 1. MPU9250 IMU Sensor (I2C1 Interface)
*   **3V3**  -> VCC
*   **GND**  -> GND
*   **PB8**  -> SCL
*   **PB9**  -> SDA

### 2. ST7735 1.8" TFT Display (SPI2 Interface)
*   **5V**   -> VCC (Power supply for the LDO regulator)
*   **GND**  -> GND
*   **PB13** -> SCK / SCL (Serial Clock)
*   **PB15** -> SDA / MOSI (Master Out Slave In)
*   **PB12** -> CS (Chip Select)
*   **PA8**  -> DC / A0 / RS (Data/Command)
*   **PA9**  -> RESET / RES (Hardware Reset)
*   **3V3**  -> LED / BLK (Backlight Power)

### 3. PC Telemetry (UART2 via USB)
*   The Nucleo board's integrated ST-LINK handles the UART-to-USB bridge automatically via **PA2 (TX)** and **PA3 (RX)**.

## Features
*   **Bare-metal/HAL Hybrid:** Efficient register-level understanding combined with HAL for rapid peripheral configuration.
*   **Non-blocking Data Transfer:** UART DMA ensures the main loop is entirely dedicated to mathematical calculations and display updates.
*   **Sensor Fusion:** Hardware-accelerated trigonometric calculations convert raw `g` forces into human-readable angles.
*   **Buffer Backlog Mitigation:** The Python Ground Station implements custom buffer-clearing logic to prevent rendering latency during high-speed data streams.

