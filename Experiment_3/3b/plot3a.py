
import pandas as pd
import matplotlib.pyplot as plt

# Read simulation results
df = pd.read_csv("experiment3a_results.csv")

# Average link efficiency across the three random seeds
avg = (
    df.groupby(["Protocol", "Ws", "Wr", "Packet_Error_Prob"])
      ["Link_Efficiency"]
      .mean()
      .reset_index()
)

# Create plot
plt.figure(figsize=(9, 6))

# Plot each protocol configuration
for (protocol, ws, wr), group in avg.groupby(
    ["Protocol", "Ws", "Wr"]
):
    group = group.sort_values("Packet_Error_Prob")

    if protocol == "Selective-Repeat":
        label = f"Selective Repeat (Wr={wr})"
    else:
        label = protocol

    plt.plot(
        group["Packet_Error_Prob"],
        group["Link_Efficiency"],
        marker="o",
        linestyle="None",
        markersize=6,
        label=label
    )

# Graph formatting
plt.xlabel("Packet Error Probability (pd)")
plt.ylabel("Link Efficiency")
plt.title("Experiment 3(a): Link Efficiency vs Packet Error Probability")

plt.xlim(0, 0.50)
plt.ylim(0, 1.05)

plt.grid(True, linestyle="--", alpha=0.5)
plt.legend()
plt.tight_layout()

# Save graph
plt.savefig("experiment3a_efficiency.png", dpi=300)

# Display graph
plt.show()
