# Smart Vault Security System

This project implements a fully non-blocking, event-driven hardware security system. The firmware runs on an STM32L476RG microcontroller and utilizes direct memory access (DMA) to process sensor data and serial logging without stalling the central CPU.

## Hardware Components

| Component | Interface | Function |
| :--- | :--- | :--- |
| **STM32L476RG Nucleo** | N/A | Main control unit driving the state machine and peripherals. |
| **MPU9250 IMU** | I2C1 | Monitors the Z-axis gravity vector to detect physical movement or tampering. |
| **4x4 Matrix Keypad** | GPIO (Rows/Cols) | Captures user input for arming and disarming the vault. |
| **Active-High Buzzer** | GPIO | Acts as the system siren when the alarm state is triggered. |

## Pinout & Wiring Configuration

The physical hardware is mapped to the STM32 using the following configuration. All custom GPIOs are assigned to `GPIOC` to keep the wiring grouped cleanly on the Nucleo headers.

### MPU9250 IMU (I2C1)
*   **VCC:** 3.3V *(Do not use 5V to prevent logic level mismatches)*
*   **GND:** GND
*   **SDA / SCL:** I2C1 Hardware Pins
*   **AD0:** 3.3V *(Sets I2C address to `0x69`)*

### 4x4 Matrix Keypad
*   **Columns 1-4:** `GPIOC` (`COL_1_Pin` - `COL_4_Pin`) — Configured as Output Push-Pull.
*   **Rows 1-4:** `GPIOC` (`ROW_1_Pin` - `ROW_4_Pin`) — Configured as Input with Internal Pull-Down resistors.

### Active-High Buzzer
*   **VCC:** 3.3V
*   **GND:** GND
*   **I/O (Signal):** `GPIOC` (`BUZZER_Pin`) — Configured as Output Push-Pull. Driven `SET` (3.3V) to trigger the alarm and `RESET` (0V) to silence.

## Software Architecture

The firmware is built on a rigid foreground/background architecture, separating data collection from logic processing to prevent hardware deadlocks.

*   **Background Polling (Interrupts & DMA):**
    *   **Keypad Scanning:** A hardware timer (`TIM2`) fires a background interrupt every 5ms to continuously scan the columns and rows of the 4x4 keypad, providing debounced input data.
    *   **IMU Data Acquisition:** The I2C peripheral utilizes DMA to pull 6 bytes of accelerometer data 20 times a second. Hardware state checks prevent bus collisions or infinite loops during physical wire disruptions.
    *   **UART Logging:** Debug messages and system states are transmitted via UART utilizing DMA, ensuring the 115200 baud transmission rate never blocks the CPU execution pipeline.
*   **Foreground Logic (Super-loop):** The main `while(1)` loop acts purely as a state machine, responding instantly to the global flags raised by the background hardware processes.

## State Machine Logic

The security protocols are governed by a strict four-stage state machine:

1.  **`STATE_DISARMED`**: The system rests in a low-security state. The user inputs a new 4-digit PIN. Pressing `*` flushes the entry buffer, while pressing `#` locks the PIN into memory, wakes up the I2C sensor, and instantly transitions the system.
2.  **`STATE_ARMED`**: The MPU9250 constantly reads Z-axis gravity data via I2C DMA. If the accelerometer deviates outside the established resting threshold (12,000 to 20,000), a debounce filter counts the deviations. 5 consecutive anomalies will trip the system.
3.  **`STATE_ALARM`**: A voltage is sent to the active-high buzzer, sounding a continuous siren. The system automatically flushes any leftover keypad data and locks into the disarming protocol.
4.  **`STATE_DISARMING`**: The siren continues to sound while the background timer collects new keypad entries. The system utilizes `strcmp` to match the newly entered code against the saved PIN. A correct match disables the buzzer and returns the system to `STATE_DISARMED`; an incorrect match flushes the buffer and forces the user to try again.
