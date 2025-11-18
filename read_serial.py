import serial
import time
import matplotlib.pyplot as plt
from collections import deque

PORT = "COM3"
BAUD = 9600

# Connect to Teensy
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)

# --- Plot setup ---
plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 8), sharex=True)

# Plot 1: raw data
line1, = ax1.plot([], [], marker='o', linestyle='-')
ax1.set_title("Log Amp Output")
ax1.set_ylabel("Power (dBm)")

# Plot 2: transformed data
line2, = ax2.plot([], [], marker='o', linestyle='-', color='orange')
ax2.set_title("Loss")
ax2.set_ylabel("Power Loss (dB)")

MAX_POINTS = 200
xs = deque(maxlen=MAX_POINTS)
ys_raw = deque(maxlen=MAX_POINTS)
ys_transformed = deque(maxlen=MAX_POINTS)

sample_index = 0

try:
    while True:
        raw = ser.readline().decode("utf-8", errors="ignore").strip()
        if not raw:
            continue

        print(raw)

        # Try parsing first token as float
        try:
            value = float(raw.split()[0].replace(',', ''))
        except:
            continue

        # Append raw and transformed data
        xs.append(sample_index)
        ys_raw.append(value)
        ys_transformed.append(-5 - value)
        sample_index += 1

        # Update plots
        line1.set_xdata(xs)
        line1.set_ydata(ys_raw)

        line2.set_xdata(xs)
        line2.set_ydata(ys_transformed)

        # Rescale axes
        ax1.relim()
        ax1.autoscale_view()

        ax2.relim()
        ax2.autoscale_view()

        fig.canvas.draw()
        fig.canvas.flush_events()
        time.sleep(0.01)

except KeyboardInterrupt:
    print("\nExiting...")
finally:
    ser.close()
    plt.ioff()
    plt.show()
