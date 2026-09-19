# STM32 Dual-Mode PWM LED Controller (ADC & UART DMA)

## Overview
This project implements a dual-mode, non-blocking LED brightness controller for the STM32L476RG. It leverages Direct Memory Access (DMA) pipelines to handle both analog sensor readings and serial communication in the background, leaving the CPU entirely free to manage the state machine and hardware register updates. 

The system operates in two distinct modes, toggled seamlessly via an external push-button using external interrupts (EXTI).

## Hardware Setup
*   **Microcontroller:** STM32L476RG (Nucleo-L476RG)
*   **Potentiometer:** `PA0` (ADC1_IN5)
*   **Green LED:** `PC6` (TIM3_CH1 - PWM Output)
*   **Push Button:** `PC9` (GPIO_EXTI9)
*   **Serial Communication:** `PA2` (TX) / `PA3` (RX) connected to USB via ST-Link.

## Core Features & Architecture

### 1. The State Machine
The firmware utilizes an EXTI-driven button with a 250ms software debouncer (`HAL_GetTick()`) to flip between two operational states:
*   **MODE_RECEIVING (Default):** The STM32 waits for 8-character serial commands (e.g., `LED:4095`) via UART DMA. Upon receiving a valid command, the CPU extracts the integer and pushes it to the PWM hardware register.
*   **MODE_SENDING:** The STM32 streams analog potentiometer readings directly into the PWM hardware register to manually dim the LED. It also transmits the current brightness value back to the serial terminal via UART DMA.

### 2. High-Efficiency Hardware Control
Instead of using standard HAL macros for PWM updates, this project uses **Direct Register Access** (`TIM3->CCR1 = current_pot`). This writes the brightness value directly into the silicon in a single CPU clock cycle, completely eliminating software overhead.

### 3. Mitigating an "Interrupt Storm"
Because the ADC is configured in continuous DMA mode, it completes reads millions of times a second. To prevent the DMA Transfer Complete interrupt from crashing the CPU, the hardware pipeline is surgically muted using:
*   `__HAL_DMA_DISABLE_IT(...)` to silence the DMA peripheral.
*   `HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn)` to block the alarm at the CPU manager level.
This allows the DMA to secretly update RAM in the background while leaving the CPU 100% free to listen for button presses.

### 4. ADC Jitter & Noise Filtering
To counter ambient electrical noise and breadboard interference, a dual-layer software filter was implemented in the transmission logic:
1.  **Threshold Filter:** The value must change by more than 100 raw ADC units to register as intentional movement.
2.  **Time Throttle:** A non-blocking `HAL_GetTick()` timer ensures UART transmissions occur a maximum of once every 100ms. 
This entirely eliminates terminal spam while keeping the physical LED fading instantly responsive.

## Usage & Testing

### Serial Terminal Configuration (RealTerm / PuTTY)
*   **Baud Rate:** 115200
*   **Data Bits:** 8
*   **Parity:** None
*   **Stop Bits:** 1
*   *Note for RealTerm users: Ensure `+CR` and `+LF` are UNCHECKED when sending ASCII commands.*

### How to Operate
1.  **Boot Up:** The board defaults to `--- RECEIVING MODE ---`.
2.  **Remote Control:** Send an 8-character command like `LED:0050` (dim) or `LED:4095` (max brightness) via the serial terminal. The LED will instantly update.
3.  **Mode Switch:** Press and release the button on `PC9`. The terminal will announce `--- SENDING MODE ---`.
4.  **Local Control:** Twist the potentiometer. The LED will smoothly fade up and down, and the terminal will print `Brightness: XXXX` as you turn the dial.
