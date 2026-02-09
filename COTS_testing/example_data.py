
import numpy as np
import matplotlib.pyplot as plt

np.random.seed(2)  # reproducibility

# Frequency axis
freq_mhz = np.arange(800, 6001, 100)

# --- Tunable parameters ---
base_loss_db = 1.2          # starting loss near 800 MHz
end_loss_db = 6.0          # target-ish loss near 6000 MHz (sets overall scale)
growth_k = 1.5             # exponential "steepness" (bigger = more curvature)
noise_sigma_db = 0.1       # point-to-point jitter amplitude (dB)
corr_strength = 0.35        # 0..1-ish; higher adds more "wandering" measurement feel
# -------------------------

# Normalized frequency 0..1 for shaping
x = (freq_mhz - freq_mhz.min()) / (freq_mhz.max() - freq_mhz.min())

# Gentle exponential rise mapped to hit ~end_loss_db at the high end
shape = (np.exp(growth_k * x) - 1.0) / (np.exp(growth_k) - 1.0)  # 0..1
loss_clean = base_loss_db + (end_loss_db - base_loss_db) * shape

# Random jitter (different each run). No seed on purpose.
white = np.random.normal(0.0, noise_sigma_db, size=len(freq_mhz))

# Add a touch of correlated drift so it looks like real sweeps (non-periodic)
drift = np.cumsum(np.random.normal(0.0, noise_sigma_db * 0.15, size=len(freq_mhz)))
drift -= drift.mean()
drift *= corr_strength

loss_db = loss_clean + white + drift

# Plot
plt.figure(figsize=(8, 6))
plt.plot(freq_mhz, loss_db, marker='o')
plt.xlabel("Frequency (MHz)")
plt.ylabel("Loss (dB)")
plt.title("Insertion Loss Between in_1 And out_1")
plt.grid(True)

plt.show()
