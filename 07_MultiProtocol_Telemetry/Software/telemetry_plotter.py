import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# ==========================================
# CONNECTION CONFIGURATION
# ==========================================
PORT = 'COM7'
BAUDRATE = 115200

print(f"Connecting to device on port {PORT}...")
try:
    # ADDED TIMEOUT: Prevents Python from freezing while waiting for data
    ser = serial.Serial(PORT, BAUDRATE, timeout=0.05)
except Exception as e:
    print(f"Connection error: {e}")
    exit()

# Plot preparation
fig, ax = plt.subplots(figsize=(10, 6))
xs, y_roll, y_pitch = [], [], []


def animate(i):
    line = ""

    # THE FIX: Read ALL pending data in the buffer, but only keep the last line.
    # This instantly eliminates the 1-2 second lag backlog.
    while ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
        except Exception:
            pass

    if line and "," in line:
        try:
            # Split "Roll,Pitch" string into two float variables
            roll, pitch = map(float, line.split(','))

            # Append data to lists
            xs.append(i)
            y_roll.append(roll)
            y_pitch.append(pitch)

            # Display only the last 50 measurements (moving window)
            xs_data = xs[-50:]

            ax.clear()
            ax.plot(xs_data, y_roll[-50:], label=f"Roll ({roll:.1f}°)", color='blue', linewidth=2)
            ax.plot(xs_data, y_pitch[-50:], label=f"Pitch ({pitch:.1f}°)", color='red', linewidth=2)

            # Grid and axes formatting
            ax.set_ylim(-90, 90)
            ax.grid(True, linestyle='--', alpha=0.7)
            ax.legend(loc='upper right')
            ax.set_title("Telemetry Ground Station - STM32 Live Data", fontsize=14, fontweight='bold')
            ax.set_ylabel("Tilt Angle [Degrees]")

        except ValueError:
            # Ignore errors if UART frame is truncated
            pass


# Start animation loop (interval=10ms ensures high refresh rate)
ani = animation.FuncAnimation(fig, animate, interval=10, cache_frame_data=False)

plt.tight_layout()
plt.show()

# Close port after closing the window
ser.close()