# 📡 STM32 Advanced Ultrasonic Radar & Telemetry System

A professional embedded system built on the STM32 architecture (ARM Cortex-M4) utilizing the STM32 Hardware Abstraction Layer (HAL). This project implements a real-time ultrasonic distance tracker with a bidirectional Python telemetry interface via UART DMA, non-blocking hardware debouncing, and a custom 16x2 LCD driver.

## 🚀 Key Features

* **Non-Blocking Architecture:** The system uses `HAL_GetTick()` for all timing operations, ensuring the processor is never frozen by `HAL_Delay()` during the main execution loop.
* **Hardware Interrupts & DMA:** Uses Timer Input Capture (TIM3) for microsecond-precise echo timing and UART with DMA (Direct Memory Access) for seamless PC communication.
* **Smart Standby Mode:** Features a physical main switch with advanced software debouncing (Falling Edge detection) to safely put the system into standby, cutting off sensor emissions and telemetry.
* **Bidirectional Telemetry:** * **TX (STM32 -> PC):** Transmits binary frames with XOR checksums containing the precise distance in centimeters.
  * **RX (PC -> STM32):** The Python host evaluates the distance and sends back specific hardware commands `<LED,BUZZER>` to control warning states dynamically.
* **Custom LCD Driver:** A from-scratch 4-bit mode implementation for the HD44780 controller. It avoids heavy external libraries and completely eliminates floating-point math (`sprintf` with `%f`) to save Flash memory and processing cycles.
