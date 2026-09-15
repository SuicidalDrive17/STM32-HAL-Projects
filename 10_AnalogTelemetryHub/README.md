# STM32 Dual-Mode ADC & UART Controller

This project demonstrates a dual-mode hardware architecture using a master state machine, non-blocking DMA UART pipelines, and EXTI hardware interrupts.

## Hardware Setup
*   **Microcontroller:** STM32L476RG (Nucleo-64)
*   **Potentiometer:** Connected to **PA0** (ADC1 Channel 5)
*   **Green LED:** Connected to **PC6**
*   **Yellow LED:** Connected to **PC7**
*   **Red LED:** Connected to **PC8**
*   **Mode Switch Button:** Connected to **PC9** (Wired diagonally to GND, utilizing internal Pull-Up)

## System Architecture

The firmware utilizes a master `switch()` statement triggered by a hardware EXTI interrupt on PC9. A 250ms software debouncer ensures clean mode transitions. 

### 1. Sending Mode (Default)
In this mode, the CPU actively reads the potentiometer via ADC DMA. 
*   **Deadband Hysteresis:** A custom software filter prevents state-flickering when the analog voltage sits on a threshold boundary.
*   **Smart Transmission:** The system only fires a UART DMA transmit request if the user turns the dial more than 50 units, or if the discrete state changes, preventing terminal spam.
*   **LED States:**
    *   `STATE_SAFE`: Green LED (Potentiometer < 1250)
    *   `STATE_MID`: Yellow LED (Potentiometer 1250 - 2450)
    *   `STATE_WARNING`: Red LED (Potentiometer 2450 - 3650)
    *   `STATE_ALARM`: Red LED blinks every 250ms (Potentiometer > 3650)

### 2. Receiving Mode
Pressing the hardware button on PC9 freezes the ADC logic and switches the board into a listening state. 
*   **DMA Listener:** The UART RX pipeline waits in the background for exactly 7 characters, utilizing the "Flag Pattern" (`command_ready`) to defer processing to the main loop, keeping the interrupt routine under 3 clock cycles.
*   **Protocol:** Send a 7-character string formatted as `LCD:GYR` where `1` is ON and `0` is OFF.
    *   `LCD:111` -> Turns all LEDs ON
    *   `LCD:010` -> Turns Yellow LED ON, Green/Red OFF
    *   `LCD:000` -> Turns all LEDs OFF
