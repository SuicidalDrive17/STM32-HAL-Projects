# STM32 Non-Blocking Arcade Reflex Game

A fast-paced, bare-metal hardware reaction game built on the STM32L476RG Nucleo board. This project demonstrates robust embedded systems design by completely avoiding blocking functions (like `HAL_Delay`), utilizing hardware interrupts for real-time input, and integrating analog sensor data as an interactive reward.

## Hardware Requirements
* **Microcontroller:** STM32L476RG (Nucleo-64)
* **Inputs:** 1x Tactile Push Button, 1x Photoresistor (LDR)
* **Outputs:** 3x LEDs (Red, Yellow, Green)
* **Passive Components:** 1x 10kΩ Resistor (for ADC voltage divider), 1x 100nF Ceramic Capacitor (optional, for hardware debouncing)

## Gameplay Mechanics
1. **Start:** Press the external button to initialize a game round.
2. **Challenge:** A random LED will light up.
    * **Red:** Click exactly 1 time.
    * **Yellow:** Click exactly 2 times.
    * **Green:** Click exactly 3 times.
3. **Time Window:** You have exactly 3 seconds to input the correct number of clicks.
4. **Scoring:** Correct inputs grant +1 point. Incorrect clicks or timeouts result in -1 point. 
5. **Victory Mode:** Reaching 5 points unlocks the sensor reward, printing live, noise-filtered analog light readings to the UART terminal. Dropping to -5 points results in a Game Over.

## Key Engineering Concepts

### 1. Non-Blocking State Machine
The entire game runs on a `switch/case` state machine (`STATE_IDLE`, `STATE_SPAWN`, `STATE_PLAYING`, `STATE_EVALUATE`, `STATE_VICTORY`). Timeouts and transitions are managed by comparing asynchronous `HAL_GetTick()` timestamps and a hardware timer (`TIM2`), ensuring the CPU is never frozen by blocking delays.

### 2. EXTI with Software Debouncing
Physical button presses are captured instantly via External Interrupts (EXTI). To prevent "ghost clicks" caused by the mechanical vibration of the button's metal spring upon release, a 250ms timestamp-based software debounce filter is applied inside the interrupt callback.

### 3. UART Telemetry & State-Change Detection
Live game data (score, targets, live clicks) is transmitted to a serial terminal over USART2. The code uses state-change detection (`actual_clicks != previous_clicks`) to instantly trigger telemetry messages the microsecond a hardware interrupt registers a valid click, without spamming the terminal loop.

### 4. ADC Polling with Analog Noise Filtering
An analog photoresistor is wired as a voltage divider to an internal 12-bit ADC. To prevent the terminal from being flooded by natural electrical voltage jitter, the software implements an absolute-value threshold filter `abs(current - previous) > 50`, ensuring data is only transmitted when the environmental light significantly changes.

## Pinout & Wiring

| Component | STM32 Pin | Details |
| :--- | :--- | :--- |
| **Red LED** | `PC0` (Example) | GPIO Output |
| **Yellow LED** | `PC1` (Example) | GPIO Output |
| **Green LED** | `PC2` (Example) | GPIO Output |
| **Arcade Button** | `PC9` | GPIO Input (EXTI with Internal Pull-Up) |
| **Light Sensor (LDR)**| `PA0` | ADC1 Channel 5 (Wired to 3.3V and 10kΩ to GND) |
| **UART TX/RX** | `PA2` / `PA3` | Internal USB ST-LINK Virtual COM Port |
