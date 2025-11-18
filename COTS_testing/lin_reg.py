import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Read the CSV file
df = pd.read_csv("data_agg.csv")   # replace with your filename

# Extract x and y columns
x = df['x']
y = df['y']

# ---- Linear regression ----
slope, intercept = np.polyfit(x, y, 1)

# Regression line
x_line = np.linspace(min(x), max(x), 200)
y_line = slope * x_line + intercept

# ---- Plot ----
plt.figure()
plt.scatter(x, y, zorder=2, s=30)
plt.plot(x_line, y_line, color='red', linewidth=2, zorder=3, label='Linear Regression')

plt.xlabel("P_in (dBm)")
plt.ylabel("DC_out (V)")
plt.title("4GHz")
plt.grid(True, zorder=0)
plt.legend()

# ---- Add slope/intercept text on the plot ----
text = f"Slope: {slope:.4f}\nIntercept: {intercept:.4f}"
plt.text(
    0.05, 0.95,
    text,
    transform=plt.gca().transAxes,
    verticalalignment='top',
    bbox=dict(boxstyle="round,pad=0.3", facecolor="white", alpha=0.7)
)

plt.show()
