import serial
import csv
from datetime import datetime

PORT = 'COM7'
BAUD = 115200

print(f"Opening port {PORT}...")
try:
    ser = serial.Serial(PORT, BAUD, timeout=2)
except Exception as e:
    print(f"Error while opening port!")
    exit()

# Create/Open file
with open('measurements.csv', mode='w', newline='') as file:
    writer = csv.writer(file)
    print("Waiting for frames from STM32...")

    try:
        while True:
            # Wait for 6 bytes
            if ser.in_waiting >= 6:
                byte = ser.read(1)

                if byte == b'\x02': # When STX
                    rest = ser.read(5)

                    if len(rest) == 5 and rest[4] == 0x03:
                        sensor_id = rest[0]
                        # rest[1] is our event we don't have tu use it here
                        msb = rest[2]
                        lsb = rest[3]

                        value = (msb << 8) | lsb
                        now = datetime.now().strftime('%d-%m-%d %H:%M:%S')

                        if sensor_id == 0x01:
                            print(f"[{now}] Light: {value}")
                        elif sensor_id == 0x02:
                            print(f"[{now}] Temp: {value}")

                        # Save to file: Time, Sensor ID, Value
                        writer.writerow([now, sensor_id, value])
                        file.flush() # Force to save on drive


    except KeyboardInterrupt:
        print("\nEnded gathering of data")
        ser.close()
