import serial
import struct
import time

SERIAL_PORT = 'COM7'
BAUD_RATE = 115200
FRAME_SIZE = 7

def calculate_checksum(data_bytes):
    # Calculate the checksum for the first 6 bytes of the frame
    checksum = 0
    for byte in data_bytes[:6]:
        checksum ^= byte
    return checksum

def determine_control_states(distance):
    # Returns a tuple of (led_state, buzzer_state) based on distance in cm
    if distance > 50.0:
        return 1, 0 # Green LED active, Buzzer silent
    elif 20.0 < distance <= 50.0:
        return 2, 1 # Yellow LED active, Slow beep command
    elif 5.0 < distance <= 20.0:
        return 3, 2 # Red LED active, Fast beep command
    else:
        return 4, 3 # Red LED active, Continuous emergency alarm

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to Nucleo on {SERIAL_PORT}. Awaiting telemetry...")

        # Flush initial buffers to avoid reading partial legacy frames
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        while True:
            # Look for our specific 0xAA start byte
            start_byte = ser.read(1)
            if not start_byte or start_byte[0] != 0xAA:
                continue # Skip junk bytes until frame sync is achieved

            # Read the remaining 6 bytes of the TelemetryFrame structure
            remaining_bytes = ser.read(FRAME_SIZE - 1)
            if len(remaining_bytes) < (FRAME_SIZE - 1):
                print("Warning: Incomplete frame received")
                continue

            full_frame = start_byte + remaining_bytes

            # Unpack the binary struct using standard C types alignment
            # < = little-endian, B = uint8, B = uint8, f = float, B = uint8
            msg_type, distance, received_checksum = struct.unpack('<BfB', remaining_bytes)

            # Validate data integrity using XOR algorithm
            computed_checksum = calculate_checksum(full_frame)
            if computed_checksum != received_checksum:
                print(f"Data corruption detected! Calculated: {computed_checksum}, Received: {received_checksum}")
                continue

            print(f"Telemetry Received -> Msg Type: {msg_type} | Distance: {distance:.2f} cm")

            # Determine the hardware tracking commands based on telemetry
            led_cmd, buzzer_cmd = determine_control_states(distance)

            # Package the string response payload: format <L,B>
            response_string = f"<{led_cmd},{buzzer_cmd}>"

            # Transmit command back down to stm32
            ser.write(response_string.encode('ascii'))

            # Tiny sleep to avoid thrashing CPU threads
            time.sleep(0.01)
    except serial.SerialException as e:
        print(f"Serial port error: {e}")
    except KeyboardInterrupt:
        print("\nStopping Host control application ")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial connection securely closed.")

if __name__ == '__main__':
    main()

