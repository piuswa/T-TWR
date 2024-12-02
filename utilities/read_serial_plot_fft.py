import serial
import csv
import matplotlib.pyplot as plt
from datetime import datetime
from scipy.signal import stft
import numpy as np

# Initialize the serial connection
ser = serial.Serial(
    port='COM12',  # Update with your COM port
    baudrate=9600,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0.1
)

print("Connected to: " + ser.portstr)

# CSV file setup
csv_filename = "serial_data_with_time.csv"
with open(csv_filename, mode='w', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(["Timestamp", "Data"])  # Write the header

timestamps = []  # Timestamps for the x-axis
values = []  # Serial data values for the y-axis

try:
    while True:
        # Read a line from the serial interface
        line = ser.readline().decode('utf-8').strip()
        if line:  # If data is received
            try:
                value = float(line)  # Convert to float (or int if preferred)
                timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]  # Current time with milliseconds

                # Print received data and timestamp
                print(f"Received at {timestamp}: {value}")
                
                # Append to data lists
                timestamps.append(timestamp)
                values.append(value)

                # Write to CSV
                with open(csv_filename, mode='a', newline='') as file:
                    writer = csv.writer(file)
                    writer.writerow([timestamp, value])

            except ValueError:
                print(f"Invalid data received: {line}")

except KeyboardInterrupt:
    print("\nExiting program.")

finally:
    ser.close()
    print("Serial connection closed.")

    # Generate the final plot
    if timestamps and values:
        sample_rate = 1000  # 1 kHz from delay(1) in Arduino code
        symbol_length = 0.25 # 250 ms per symbol
        nperseg = int(sample_rate * symbol_length)  # Number of samples per segment
        frequencies, times, Zxx = stft(values, fs=sample_rate, nperseg=nperseg)
        # decode the received data from the serial port into 0 and 1
        freq0 = 100
        freq1 = 200
        bits = []
        idx_freq0 = np.argmin(np.abs(frequencies - freq0))
        idx_freq1 = np.argmin(np.abs(frequencies - freq1))

        for i in range(Zxx.shape[1]):
            # Check the magnitude of the target frequencies in each segment
            if np.abs(Zxx[idx_freq0, i]) > np.abs(Zxx[idx_freq1, i]):
                bits.append(0)
            else:
                bits.append(1)


        # Print the shapes of the arrays
        print("Zxx: ", Zxx.shape)
        print("frequencies: ", frequencies.shape)
        print("times: ", times.shape)
        print("Decoded bits: ", bits)
        # Plot the spectrogram
        plt.figure(figsize=(10, 6))
        plt.pcolormesh(times, frequencies, np.abs(Zxx), shading='auto')
        plt.title('Spectrogram of the signal')
        plt.ylabel('Frequency [Hz]')
        plt.xlabel('Time [sec]')
        plt.colorbar(label='Intensity')
        plt.show()
    else:
        print("No data to plot.")
