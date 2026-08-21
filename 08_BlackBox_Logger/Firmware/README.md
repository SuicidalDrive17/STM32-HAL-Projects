# Project 08: Non-Blocking BlackBox Data Logger (I2C & UART DMA)

## 📖 Overview
This project implements an industrial-grade, non-blocking data acquisition system on an STM32 microcontroller. The firmware reads real-time timestamps from a DS1307 RTC and 3-axis acceleration data from an MPU9250 IMU over a shared I2C bus. The data is then formatted and streamed in real-time to a PC via UART.

Originally designed to log to a MicroSD card via SPI and FatFs, the project was dynamically pivoted to a PC-based UART logging system while maintaining the underlying non-blocking DMA architecture.

## 🛠️ Hardware Setup & Wiring
This project uses a shared I2C bus (I2C1) with two sensors of different voltage logic. 

**I2C Address Conflict Resolution:**
By default, both the DS1307 and MPU9250 attempt to use the `0x68` I2C address. To fix this collision on the shared bus, the MPU9250's **AD0** pin is physically pulled to 3.3V, changing its address to `0x69`.

| STM32 Pin | DS1307 RTC (5V) | MPU9250 IMU (3.3V) | Description |
| :--- | :--- | :--- | :--- |
| **5V** | VCC | - | Power for RTC logic |
| **3.3V** | - | VCC & AD0 | Power for IMU logic & Address Mod |
| **GND** | GND | GND | Common Ground Rail |
| **PB6** | SCL | SCL | I2C1 Serial Clock |
| **PB7** | SDA | SDA | I2C1 Serial Data |

*Note: The DS1307 `SQW` pin is left completely floating.*

## 🧠 Software Architecture
The firmware abandons beginner-level blocking delays (like `HAL_Delay`) in favor of an **Event-Driven State Machine** combined with **Direct Memory Access (DMA)** pipelines.

### The State Machine (`SystemState_t`)
1. **`STATE_IDLE`**: CPU sleeps, waiting for a UART interrupt.
2. **`STATE_READ_TIME`**: CPU triggers I2C DMA to fetch 3 bytes from the RTC, then instantly steps aside.
3. **`STATE_TIME_PENDING`**: System rests while the DMA works in the background.
4. **`STATE_READ_SENSOR`**: CPU triggers I2C DMA to fetch 6 bytes from the IMU.
5. **`STATE_SENSOR_PENDING`**: System rests while DMA pulls the X, Y, and Z axis data.
6. **`STATE_SEND_UART`**: CPU formats the raw bytes into a human-readable `snprintf()` string and triggers the UART TX DMA pipeline.
7. **`STATE_TX_PENDING`**: System waits for the PC to receive the complete string before looping back to read the time again.
8. **`STATE_ERROR`**: Safe catch-all for hardware bus failures or missing sensor ACKs.

### Interrupts vs. DMA
*   **UART RX (Interrupt):** Used for listening to PC commands (`'S'` or `'X'`). Because it is only waiting for a spontaneous 1-byte signal, lightweight interrupts (`_IT`) are highly efficient.
*   **I2C RX & UART TX (DMA):** Used for heavy, predictable, continuous data flow. The CPU is not paused during memory transfers, preventing bus lock-ups and maximizing speed.

## 🚀 How to Use (Testing via RealTerm)
1. **Compile and Flash** the code to the STM32 Nucleo board using STM32CubeIDE.
2. **Open a Serial Terminal** (e.g., RealTerm, PuTTY, TeraTerm) with the following settings:
   * **Baud Rate:** `115200`
   * **Data Bits:** 8, **Parity:** None, **Stop Bits:** 1 (8N1)
   * **EOL:** Ensure `+CR` and `+LF` are *unchecked* when sending commands.
3. **Start the Logger:** Type `S` and click **Send ASCII**. The terminal will flood with a real-time data stream:
   `Time: 14:05:27 | X:-984, Y:-16, Z:16284`
4. **Stop the Logger:** Type `X` and click **Send ASCII**. The state machine will instantly halt and return to idle.
