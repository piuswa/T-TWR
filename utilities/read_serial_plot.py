import serial
import csv
import matplotlib.pyplot as plt
from datetime import datetime

# Initialize the serial connection
ser = serial.Serial(
    port='/dev/ttyACM0',  # Update with your COM port
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
        plt.figure(figsize=(10, 6))
        plt.plot(timestamps, values, marker='o', linestyle='-', color='b')
        plt.xlabel("Timestamp")
        plt.ylabel("Data")
        plt.title("Data vs Time")
        plt.xticks(rotation=45, ha='right')
        plt.tight_layout()
        plt.show()
    else:
        print("No data to plot.")
