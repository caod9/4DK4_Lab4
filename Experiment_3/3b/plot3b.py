import pandas as pd
import matplotlib.pyplot as plt

# Read simulation results
df = pd.read_csv("experiment3b_results.csv")

# Average results across the three random seeds
avg = (
    df.groupby(["Protocol", "Ws", "Wr"], as_index=False)
    [["Link_Efficiency", "Avg_TX_Attempts"]]
    .mean()
)

# Define protocol configurations
protocols = [
    ("Go-Back-N", 1, "Go-Back-N (Wr=1)"),
    ("Selective-Repeat", 2, "Selective Repeat (Wr=2)"),
    ("Selective-Repeat", 6, "Selective Repeat (Wr=6)"),
    ("Selective-Repeat", 12, "Selective Repeat (Wr=12)")
]

# --------------------------------------------------
# Graph 1: Link Efficiency vs Sender Window Size
# --------------------------------------------------

plt.figure(figsize=(9, 6))

for protocol, wr, label in protocols:
    data = avg[
        (avg["Protocol"] == protocol) &
        (avg["Wr"] == wr)
    ].sort_values("Ws")

    plt.plot(
        data["Ws"],
        data["Link_Efficiency"],
        marker="o",
        linestyle="None",
        label=label
    )

plt.xlabel("Sender Window Size (Ws)")
plt.ylabel("Link Efficiency")
plt.title("Experiment 3(b): Link Efficiency vs Sender Window Size")
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig("experiment3b_efficiency.png", dpi=300)


# --------------------------------------------------
# Graph 2: Average Transmission Attempts vs Ws
# --------------------------------------------------

plt.figure(figsize=(9, 6))

for protocol, wr, label in protocols:
    data = avg[
        (avg["Protocol"] == protocol) &
        (avg["Wr"] == wr)
    ].sort_values("Ws")

    plt.plot(
        data["Ws"],
        data["Avg_TX_Attempts"],
        marker="o",
        linestyle="None",
        label=label
    )

plt.xlabel("Sender Window Size (Ws)")
plt.ylabel("Average Transmission Attempts per Delivered Packet")
plt.title("Experiment 3(b): Transmission Attempts vs Sender Window Size")
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig("experiment3b_tx_attempts.png", dpi=300)

# Display both graphs
plt.show()