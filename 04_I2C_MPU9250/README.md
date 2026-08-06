# 04_I2C_MPU9250 Telemetry System

A real-time 3D orientation telemetry system that reads data from an MPU-9250/MPU-6500 Inertial Measurement Unit (IMU) via I2C and visualizes the movement on a PC using Python.

## Project Overview
The goal of this project was to establish reliable digital communication between an STM32 microcontroller and an external sensor using the I2C protocol, package the data into secure binary transmission frames, and process them dynamically on a host computer.

## Features
- **I2C Communication:** Low-level register configuration and data acquisition from the MPU-6500 accelerometer.
- **Binary Framing:** Custom UART transmission protocol utilizing start/stop bytes for data integrity and error prevention.
- **3D Visualization:** Real-time artificial horizon and orientation mapping rendered in a Python environment using VPython.

## Hardware Configuration
- **Microcontroller:** STM32L476RG (Nucleo-L476)
- **Sensor:** MPU-9250 / MPU-6500 (IMU)
- **Pinout Mapping:**
  - `I2C1_SCL` -> PB6 (connected to MPU SCL)
  - `I2C1_SDA` -> PB7 (connected to MPU SDA)
  - `USART2_TX` -> PA2 (Virtual COM Port via USB)
  - `USART2_RX` -> PA3 (Virtual COM Port via USB)

## Software Stack
- **Firmware:** C (STM32CubeIDE / HAL Architecture)
- **Host Application:** Python 3.x (with `pyserial` and `vpython` libraries)

## How to Run
1. Connect the hardware according to the pinout mapping.
2. Flash the STM32 firmware using STM32CubeIDE.
3. Install Python dependencies: `pip install pyserial vpython`.
4. Update the correct COM port in the Python script.
5. Run the Python script to open the 3D visualization.
