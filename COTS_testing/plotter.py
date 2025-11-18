import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file
df = pd.read_csv("data_agg.csv")   # replace with your filename

# Extract x and y columns
x = df['x']
y = df['y']

# Plot
plt.figure()
plt.scatter(x, y, zorder=2, s=30)
plt.xlabel("P_in (dBm)") # Specify max of FFT
plt.ylabel("DC_out (V)")
plt.title("4Ghz")
plt.grid(True, zorder=0)
plt.show()
