import pandas as pd
import matplotlib.pyplot as plt

print("Loading data from measurements.csv...")

try:
    # Load data with English column names
    df = pd.read_csv('measurements.csv', names=['Time', 'ID', 'Value'])
    df['Time'] = pd.to_datetime(df['Time'])

    # Split data based on sensor ID
    df_light = df[df['ID'] == 1]
    df_temp = df[df['ID'] == 2]

    # Create a window with two subplots (one below the other)
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

    # --- Plot 1: Light ---
    ax1.plot(df_light['Time'], df_light['Value'], color='orange', marker='.', linestyle='-')
    ax1.set_title('Light Intensity (Photoresistor)')
    ax1.set_ylabel('ADC Value')
    ax1.grid(True)

    # --- Plot 2: Temperature ---
    ax2.plot(df_temp['Time'], df_temp['Value'], color='blue', marker='.', linestyle='-')
    ax2.set_title('Temperature (Thermistor)')
    ax2.set_ylabel('ADC Value')
    ax2.grid(True)

    # Common X-axis labels
    plt.xlabel('Measurement Time')
    plt.xticks(rotation=45)
    plt.tight_layout()

    plt.show()

except FileNotFoundError:
    print("File not found! Run datalogger.py first to collect measurements.")