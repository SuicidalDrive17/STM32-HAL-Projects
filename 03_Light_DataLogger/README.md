# 03_Light_DataLogger

A complete IoT telemetry system that acquires analog data from two sensors using an STM32 microcontroller and transmits it via a custom binary protocol to a PC for logging and real-time visualization in Python.

---

## Hardware Architecture

The project utilizes two independent voltage divider circuits on a breadboard built with **10kΩ resistors** and powered strictly from the **3.3V** rail of the Nucleo board:

1. **Light Intensity Sensor (Photoresistor):** Connected to pin `PC0` (ADC1_IN1).
2. **Temperature Sensor (NTC Thermistor):** Connected to pin `PA0` (ADC1_IN5).

---

## Firmware Configuration (STM32)

### 1. Clock & Timing
* **System Clock:** Configured via PLL to run at **80 MHz**.
* **Timer 6 (TIM6):** Acts as the hardware time-base generator triggering interrupts.
  * **Prescaler:** `15999` (Internal clock division: $80,000,000 / 16,000 = 5,000\text{ Hz}$)
  * **Counter Period:** `999` (Generates an update event every 1000 ticks)
  * **Resulting Sampling Rate:** Exactly **5 Hz** (data acquired and transmitted **5 times per second**).

### 2. ADC1 (Analog to Digital Converter)
To poll two channels efficiently without crosstalk or hardware data overwrite errors, the following settings were implemented:
* **Scan Conversion Mode:** Enabled (Sequencing Rank 1 -> Rank 2).
* **Continuous Conversion Mode:** Disabled.
* **Discontinuous Conversion Mode:** Enabled (Crucial! Forces the ADC to pause after each channel execution, allowing the CPU to safely read the data).
* **Sampling Time:** Extended to **247.5 Cycles** for both ranks to eliminate **ADC Ghosting** (gives the internal sampling capacitor enough time to charge properly through the 10kΩ impedance of the breadboard circuit).

### 3. USART2
* **Baud Rate:** 115200
* **Word Length:** 8 Bits
* **Stop Bits:** 1
* **Parity:** None

---

## Custom Telemetry Frame Format (6 Bytes)

Data is pushed over the serial interface using a secure, low-overhead binary framing protocol:

| Byte Index | Field Name | Value / Description |
| :--- | :--- | :--- |
| **Byte 0** | **STX** (Start of Text) | `0x02` |
| **Byte 1** | **Sensor ID** | `0x01` = Photoresistor \| `0x02` = Thermistor |
| **Byte 2** | **Event Type** | `0x00` (Constant measurement streaming) |
| **Byte 3** | **Data MSB** | Most Significant Byte of the 12-bit ADC value |
| **Byte 4** | **Data LSB** | Least Significant Byte of the 12-bit ADC value |
| **Byte 5** | **ETX** (End of Text) | `0x03` |

---

## Software Architecture (Python)

The PC-side environment is isolated into the `Software/` directory and consists of two primary scripts:

### 1. `datalogger.py`
Monitors the virtual COM port, extracts whole 6-byte frames from the hardware ring buffer, verifies framing integrity (`STX` and `ETX` validations), and converts incoming byte pairs back into a single 12-bit integer.
* **Storage Mode:** Configured to `mode='w'` (Write/Overwrite mode), which clears previous data upon boot, starting each recording session fresh.
* **Output:** Saves records out dynamically into `measurements.csv` formatted as: `Timestamp, Sensor_ID, ADC_Value`.

### 2. `chart.py`
A data processing script leveraging `pandas` and `matplotlib`.
* Parses text timestamps directly into high-fidelity `datetime` objects.
* Filters interleaved streaming structures into dedicated telemetry tables via boolean indexing matching sensor IDs.
* Subplots are configured with `sharex=True`, anchoring both time-axes together for synchronous tracking, zooming, and cross-examination.

---

## Project Structure

```text
03_Light_DataLogger/
├── Firmware/
│   ├── Core/               # Application source files (main.c, stm32l4xx_it.c, etc.)
│   ├── Drivers/            # CMSIS and HAL Hardware Abstraction Layers
│   ├── .cproject           # Toolchain configuration (Keep for STM32CubeIDE)
│   ├── .project            # Workspace configuration (Keep for STM32CubeIDE)
│   └── 03_Light_DataLogger.ioc
└── Software/
    ├── datalogger.py       # Serial interface parser and CSV recorder
    └── chart.py            # Analytics engine and visualization plotter
