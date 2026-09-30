# Project 14: Blind Stopwatch

A precision hardware-based reaction game built for the STM32L476RG microcontroller. The objective is to select a target time (5 or 10 seconds), start a hidden hardware timer, and press a stop button as close to the target time as possible without looking at a clock. 

This project demonstrates the use of microsecond-precision hardware timers (TIM), External Interrupts (EXTI) for instant physical response, software debouncing, and a decoupled State Machine architecture to prevent serial communication delays from affecting timing accuracy.

## Features

*   **Microsecond Precision:** Utilizes `TIM2` clocked at 80 MHz with a prescaler of 79 to achieve exactly 1-microsecond resolution.
*   **Decoupled State Machine:** The game logic runs in the main `while(1)` loop, while all time-critical functions (starting/stopping the timer) are handled exclusively inside hardware-level EXTI interrupts. This guarantees that `HAL_UART_Transmit` delays do not impact the player's score.
*   **Hardware Interrupts (EXTI):** Buttons are routed to dedicated EXTI lines for immediate response, bypassing standard polling methods.
*   **Software Debouncing:** Implements a `HAL_GetTick()` 250ms filter inside the EXTI callback to ignore mechanical spring bounces and electrical noise.
*   **Dynamic Scoring System:** Evaluates accuracy based on tiered time boundaries (Green, Yellow, Red) using absolute difference integer math.
*   **Hardware Timeout:** The timer utilizes an Auto-Reload Register (ARR) interrupt to automatically penalize the player if they abandon the timer for more than 15 seconds.

## Hardware Configuration & Wiring

This project uses the Nucleo-L476RG board. Push buttons must connect the GPIO pin to Ground when pressed (Active-Low), as the pins are configured with internal Pull-Up resistors.

| Component | STM32 Pin | GPIO Mode | Configuration |
| :--- | :--- | :--- | :--- |
| **5S Mode Button** | PC0 | EXTI0 | Pull-up, Falling Edge Trigger |
| **10S Mode Button** | PC1 | EXTI1 | Pull-up, Falling Edge Trigger |
| **Stopwatch Button** | PC9 | EXTI9_5 | Pull-up, Falling Edge Trigger |
| **Green LED** | PC6 | Output | Push-Pull, No Pull |
| **Yellow LED** | PC7 | Output | Push-Pull, No Pull |
| **Red LED** | PC8 | Output | Push-Pull, No Pull |

### Peripheral Setup
*   **System Clock:** 80 MHz (HSI)
*   **TIM2:** Internal Clock, Prescaler: 79, Period (ARR): 14999999 (15-second hardware limit), Global Interrupt Enabled.
*   **USART2:** Asynchronous, 115200 Baud, 8N1.

## Software Architecture

The software relies on a master `GameState_t` enumeration to route logic cleanly. 

1.  **Menu/Idle Phase:** The system waits for an EXTI trigger on PC0 or PC1 to select the game mode.
2.  **Instruction Phase:** The main loop prints UI instructions via UART and transitions to a waiting state, preventing the CPU from spamming the serial terminal.
3.  **Active Phase:** The EXTI callback exclusively handles the start and stop commands from PC9 (Stopwatch Button), ensuring the `TIM2->CNT` register is captured the exact microsecond the electrical circuit closes.
4.  **Evaluation Phase:** The main loop takes over, calculating the absolute difference between `final_time` and the target (5,000,000 µs or 10,000,000 µs), formats the result via integer math (to bypass floating-point memory overhead), updates the score, and triggers the appropriate LED.

## Game Rules & Scoring

Players accumulate or lose points based on their accuracy. Reach **5000 points** to win. Drop to **-2000 points** and it is Game Over.

**5-Second Mode**
*   🟢 **Green (Perfect):** ± 0.5 seconds (+1000 pts)
*   🟡 **Yellow (Close):** ± 1.0 seconds (+0 pts)
*   🔴 **Red (Miss):** > 1.0 seconds (-500 pts)

**10-Second Mode** *(Double tolerances)*
*   🟢 **Green (Perfect):** ± 1.0 seconds (+1000 pts)
*   🟡 **Yellow (Close):** ± 2.0 seconds (+0 pts)
*   🔴 **Red (Miss):** > 2.0 seconds (-500 pts)

**Penalties**
*   **Timeout:** Waiting longer than 15 seconds triggers a hardware interrupt, resulting in an automatic Red LED and -500 points.

## How to Run
1. Clone the repository and open the project in **STM32CubeIDE**.
2. Build the project and flash it to the Nucleo-L476RG board.
3. Open a Serial Terminal (e.g., RealTerm, PuTTY, TeraTerm) connected to the board's COM port at `115200` baud.
4. Press the 5S or 10S button to begin.
