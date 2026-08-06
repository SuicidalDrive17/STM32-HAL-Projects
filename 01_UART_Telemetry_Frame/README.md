# 🚀 Project 01: UART Telemetry Frame & Internal Temperature Sensor

First project implemented using the **HAL (Hardware Abstraction Layer)** library. The main goal of this project is to learn how to handle analog peripherals and asynchronous data transmission without blocking the main processor loop.

## 📌 How it works
The microcontroller uses the temperature sensor built directly into the silicon structure. The voltage measured by the 12-bit ADC is subjected to bitwise operations and packed into a standardized, 5-byte binary frame. The frame is then sent to a PC (or a master device, e.g., Raspberry Pi) via the USART interface using hardware interrupts.

## 🛠 Utilized Mechanisms (Nucleo-L476RG)
* **ADC1 (Analog-to-Digital Converter):** Configured to read the internal Temperature Sensor Channel.
* **USART2 (Asynchronous):** Configured to transmit data at a baud rate of 115200 bps.
* **NVIC (Nested Vectored Interrupt Controller):** Handling the global `USART2_IRQn` interrupt for non-blocking data transmission (`HAL_UART_Transmit_IT` function and `HAL_UART_TxCpltCallback` callback).

## 📦 Binary Frame Structure
Data is not sent as raw ASCII text but is compressed into an optimized binary frame, simulating professional industrial transmission protocols:

| Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 |
| :---: | :---: | :---: | :---: | :---: |
| `0x02` | `0x02` | MSB | LSB | `0x03` |
| **STX** (Start) | **ID** (Sensor) | ADC High Byte | ADC Low Byte | **ETX** (Stop) |

## 🚀 How to test?
To read the frames, it is highly recommended to use a serial terminal like **RealTerm** or a custom Python script (e.g., using the `pyserial` library). In RealTerm, set the display mode (Display As) to **Hex[space]** to ignore ASCII encoding and observe the raw memory bytes.
