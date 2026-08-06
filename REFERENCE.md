# 📚 STM32 HAL & CubeMX Master Configuration Guide

The golden rule of pin configuration in the STM32CubeMX GUI. Although the HAL library abstracts away direct register manipulation, the underlying physics of the hardware remain exactly the same.

| Pin Purpose / Interface | GPIO Output Level | GPIO Mode | GPIO Pull-up / Pull-down | Why use this exact configuration? |
| :--- | :--- | :--- | :--- | :--- |
| **Basic LED / Relay** | Low / High | **Push-Pull** | `No pull-up and no pull-down` | You actively blast 3.3V (Push) to turn it on, and actively sink it to 0V (Pull) to turn it off. Internal resistors are useless when the line is actively driven. |
| **Basic Button** | *N/A (Input)* | *Input mode* | **Pull-up** or **Pull-down** | An unpressed wire acts like an antenna and absorbs static noise. A Pull-up/down acts like a "bungee cord" to hold the line at a safe, predictable default state. |
| **Analog Input (ADC)** | *N/A (Input)* | **Analog mode** | **No pull-up and no pull-down (CRITICAL)** | You must measure the *exact*, raw physical voltage. Turning on an internal Pull-Up injects extra voltage into the wire and completely ruins the math! |
| **USART (TX & RX)** | *N/A* | Alternate Function | **Pull-up** | TX uses Push-Pull for sharp square waves. We add a Pull-Up on the RX line because USART "idles" HIGH. If a wire unplugs, the Pull-Up prevents the MCU from reading floating garbage from the air. |
| **SPI (MOSI, MISO, SCK)** | *N/A* | Alternate Function | `No pull-up and no pull-down` | SPI is built for maximum speed (MHz). Push-Pull aggressively forces the voltage High and Low to create lightning-fast, perfect square edges. |
| **SPI (Chip Select / CS)** | High (Default) | **Push-Pull** | `No pull-up and no pull-down` | The STM32 actively drives this HIGH (sleep) or LOW (wake). *Pro Tip: Some engineers add an external physical Pull-Up resistor just to ensure peripheral sensors stay asleep while the STM32 boots up.* |
| **I2C (SDA & SCL)** | *N/A* | **Open-Drain** | **Pull-up** | **CRITICAL:** I2C devices share the same wires. If two chips used Push-Pull and fought (one pulling 3.3V, one pulling 0V), it would cause a short circuit! Open-Drain ensures chips can only pull the line LOW. |

> **⚠️ The Rule of HAL_Delay() and Interrupts:**
> Keep in mind that the `HAL_Delay()` function relies on the system timer (SysTick). You must **never** use `HAL_Delay()` inside an interrupt service routine (Callbacks, e.g., `HAL_UART_TxCpltCallback`), as it will cause a deadlock and completely freeze your program!
