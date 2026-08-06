import serial
import struct
from vpython import *

# Configuration
COM_PORT = 'COM7'
BAUD_RATE = 115200
START_BYTE = b'\xAA'
STOP_BYTE = 0xBB

# 3D Scene setup
scene.title = "MPU-6500 Telemetry | Real-Time Artificial Horizon"
scene.width = 1024
scene.height = 768
scene.background = color.black

# Creating a simple rectangular cuboid
sensor_box = box(length=6, width=4, height=0.5, color=color.orange)

# Serial port initialization
try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
    print(f"Successfully connected to {COM_PORT}. Waiting for binary telemetry...")
except serial.SerialException as e:
    print(f"Hardware Error: Cannot open {COM_PORT}. Is it used by another program?")
    exit()

# Main telemetry loop
while True:
    # Essential for VPython: caps the engine at 50 FPS and prevents UI freezing
    rate(50)

    try:
        # Check if we have at least one full frame waiting in the hardware (8 bytes)
        if ser.in_waiting >= 8:
            if ser.read(1) == START_BYTE:
                # Read the remaining 7 bytes of the payload
                payload = ser.read(7)

                if len(payload) == 7 and payload[6] == STOP_BYTE:
                    # Binary decoding
                    # '>hhh' means: Big-Endian (>), 3 short signed integers (h h h)
                    raw_x, raw_y, raw_z = struct.unpack('>hhh', payload[0:6])

                    # Unit conversion (raw to G-Force)
                    # MPU-6500 default sensitivity is +- 2g (1g = 16384 LSB)
                    g_x = raw_x / 16384.0
                    g_y = raw_y / 16384.0
                    g_z = raw_z / 16384.0

                    # 3D orientation update
                    # Map the sensor's X, Y, Z axes to VPython's 3D space axes
                    up_vector = vector(-g_x, g_z, -g_y)

                    # Protect against free-fall (0g) vector math errors
                    if mag(up_vector) > 0.1:
                        # Instantly updates the 3D cuboid's orientation
                        sensor_box.up = up_vector
    except KeyboardInterrupt:
        print("Telemetry stream stopped by user\n")
        ser.close()
        break
    except Exception as e:
        # Ignore random serial drops or frame shifting during movement
        pass





