# STM32 Hardware Reaction Timer

## Overview
This project implements a precision reaction time measuring system for the STM32L476RG. Moving away from standard blocking firmware, this architecture relies entirely on a non-blocking state machine, hardware-level interrupts, and a 32-bit microsecond hardware stopwatch. It measures human reflex speed down to the millisecond without trapping the CPU in delay loops.

## Hardware Setup
*   **Microcontroller:** STM32L476RG (Nucleo-L476RG)
*   **Trigger Button:** `PC9` (Configured as `GPIO_EXTI9`, Falling Edge, Pull-up)
*   **Stimulus LED:** `PC6` (Configured as `GPIO_Output`)
*   **Serial Monitor:** `PA2` (TX) / `PA3` (RX) via USART2 (115200 Baud)

## Core Engineering Features

### 1. Non-Blocking State Machine
The core loop operates freely without ever calling `HAL_Delay()`. The system transitions between four strict states:
*   **STATE_IDLE:** The CPU rests, waiting for a hardware interrupt from the button.
*   **STATE_WAITING:** The system waits for a random or fixed time interval. It uses a non-blocking `HAL_GetTick()` evaluation, allowing the CPU to instantly process a false start if the user presses the button early.
*   **STATE_MEASURING:** The green LED snaps on, and the hardware timer begins counting in the background. The CPU simply waits for the next interrupt.
*   **STATE_RESULT:** The stopwatch is halted, the microsecond value is extracted, and the final score is transmitted via UART.

### 2. High-Resolution Hardware Stopwatch (TIM2)
Instead of relying on standard 1-millisecond SysTick interrupts, this project utilizes `TIM2`, a 32-bit general-purpose timer. 
*   **The 16-Bit Prescaler Limitation:** Because the STM32 hardware limits the prescaler register to 16 bits (max 65,535), dividing the 80 MHz clock down to 1 millisecond ticks was impossible. 
*   **The 1 MHz Solution:** The prescaler is set to `80 - 1` (79). This divides the 80 MHz clock into a 1 MHz signal, meaning the 32-bit `TIM2->CNT` register ticks exactly once every **1 microsecond**. The software then divides this raw value by 1000 to output highly accurate millisecond reaction times.

### 3. EXTI & Hardware Interrupts
The reaction button is wired directly to the Nested Vectored Interrupt Controller (NVIC). By using a **Falling Edge** trigger, the stopwatch stops the exact microsecond the physical metal contacts close inside the button, completely bypassing software polling delays.

### 4. Contact Bounce Mitigation
Physical tactile switches generate electrical noise ("contact bounce") for 10-50 milliseconds after being pressed. To prevent this noise from rapidly triggering multiple states and resetting the game instantly, a software debounce filter was applied inside the EXTI callback:
```c
if (current_interrupt_time - last_interrupt_time > 200) { ... }
