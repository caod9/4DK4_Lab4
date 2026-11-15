
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Experiment 5: read the simulation results (three seeds per configuration)
df = pd.read_csv("experiment5_results.csv")

# Average the two performance metrics across random seeds.
avg = (
    df.groupby(["Protocol", "Ws", "Wr", "Timeout"], as_index=False)
      [["Link_Efficiency", "Avg_TX_Attempts"]]
      .mean()
)

# Protocol configurations: same marker, different colours
protocols = [
    ("Stop-and-Wait", 1, 1, "Stop-and-Wait (Ws=1, Wr=1)", "blue"),
    ("Go-Back-N", 12, 1, "Go-Back-N (Ws=12, Wr=1)", "orange"),
    ("Selective-Repeat", 12, 2, "Selective Repeat (Ws=12, Wr=2)", "green"),
    ("Selective-Repeat", 12, 6, "Selective Repeat (Ws=12, Wr=6)", "red"),
    ("Selective-Repeat", 12, 12, "Selective Repeat (Ws=12, Wr=12)", "purple"),
]


def make_plot(metric, ylabel, title, filename):
    plt.figure(figsize=(9, 6))

    for protocol, ws, wr, label, color in protocols:
        data = avg[
            (avg["Protocol"] == protocol)
            & (avg["Ws"] == ws)
            & (avg["Wr"] == wr)
        ].sort_values("Timeout")

        plt.plot(
            data["Timeout"],
            data[metric],
            marker="o",
            linestyle="None",  # No piecewise-linear interpolation
            markersize=7,
            color=color,
            label=label,
        )

    plt.xlabel("Timeout (tout)")
    plt.ylabel(ylabel)
    plt.title(title + " pa = 0.10, ps = 0.10")
    plt.xticks(sorted(df["Timeout"].unique()))
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(filename, dpi=300)


make_plot(
    "Link_Efficiency",
    "Link Efficiency",
    "Experiment 5: Link Efficiency vs Timeout",
    "experiment5_efficiency.png",
)

make_plot(
    "Avg_TX_Attempts",
    "Avg. Data-Packet Transmission Attempts per Delivered Packet",
    "Experiment 5: Transmission Attempts vs Timeout",
    "experiment5_tx_attempts.png",
)

plt.show()