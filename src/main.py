import serial
import csv

ser = serial.Serial('COM3', 115200)
filename = "oximeter_data.csv"

with open(filename, "w", newline='') as f:
    writer = csv.writer(f)

    writer.writerow(["Timestamp", "Red", "IR"])

    try:
        while True:
            line = ser.readline().decode('utf-8').strip()
            if line:
                data = line.split(",")
                if len(data) == 3:
                    writer.writerow(data)
                    print(f"Data Recived: {data}")
    except KeyboardInterrupt:
        print("Stopped recording.")
        ser.close()
