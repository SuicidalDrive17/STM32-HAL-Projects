# STM32 UART Stopwatch & Telemetry

This project implements a precise stopwatch functionality on a **STM32 Nucleo-L476RG** development board. The device measures the duration of a button press and transmits the result as a binary telemetry frame via UART.

## Features
- **Precise Timing:** Uses `TIM6` configured with an interrupt-driven approach (10ms resolution).
- **UART Communication:** Sends data frames when the user releases the button.
- **Binary Protocol:** Structured 6-byte frame for efficient data transmission.
- **Hardware Interaction:** Triggered by the onboard User Button (B1).

## Communication Protocol
The system uses a custom binary protocol to ensure reliable data transfer. Each telemetry frame consists of 6 bytes:

| Byte | Description | Value |
| :--- | :--- | :--- |
| 1 | STX (Start of Transmission) | `0x02` |
| 2 | Timer ID | `0x03` |
| 3 | Event Type | `0x00` (Release) |
| 4 | Data MSB | Time high byte |
| 5 | Data LSB | Time low byte |
| 6 | ETX (End of Transmission) | `0x03` |

*The time value represents the duration in 10ms increments.*

## Project Structure
- `Core/`: Source code (`main.c`, etc.) and headers.
- `Drivers/`: STM32 HAL libraries and CMSIS.
- `UART_Stopwatch.ioc`: STM32CubeMX configuration file.

## Requirements
- **Hardware:** STM32 Nucleo-L476RG.
- **Software:** STM32CubeIDE.
- **Testing:** RealTerm or any serial terminal emulator (configured to Hex display mode).

## How to use
1. Connect the Nucleo board to your PC.
2. Open your terminal emulator (e.g., RealTerm) and set it to the correct COM port (Baud rate: 115200).
3. Press and hold the **B1 button** on the Nucleo board.
4. Release the button; a 6-byte telemetry frame will appear in your terminal.
