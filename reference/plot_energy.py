"""Plot the Day 2 energy drift experiment. Run exp_energy_drift first."""
import numpy as np
import matplotlib.pyplot as plt

d = np.genfromtxt("energy_drift.csv", delimiter=",", names=True)

fig, ax = plt.subplots(figsize=(9, 5))
ax.plot(d["orbit"], np.abs(d["rk4_h"]),     label="RK4, h",      lw=1)
ax.plot(d["orbit"], np.abs(d["rk4_h2"]),    label="RK4, h/2",    lw=1)
ax.plot(d["orbit"], np.abs(d["verlet_h"]),  label="Verlet, h",   lw=1)
ax.plot(d["orbit"], np.abs(d["verlet_h2"]), label="Verlet, h/2", lw=1)
ax.set_yscale("log")      # errors span orders of magnitude: log axis is essential
ax.set_xlabel("orbits")
ax.set_ylabel(r"$|\Delta E / E_0|$")
ax.set_title("Symplectic integration bounds the energy error; RK4 drifts")
ax.legend()
ax.grid(alpha=0.3)
plt.tight_layout()
plt.savefig("energy_drift.png", dpi=150)
print("wrote energy_drift.png")