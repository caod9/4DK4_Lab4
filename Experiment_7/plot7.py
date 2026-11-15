
"""Experiment 7: Mean completion time vs sender window.

Requires: pandas and matplotlib
Run after experiment7.exe generates experiment7_results.csv.
"""

import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

base = Path(__file__).resolve().parent

# Read simulation results
df = pd.read_csv(base / "experiment7_results.csv")

# Calculate mean and standard deviation
summary = (
    df.groupby(
        [
            "Transfer_Size",
            "Packet_Error_Prob",
            "ACK_Error_Prob",
            "Protocol",
            "Ws",
            "Wr"
        ]
    )["Completion_Time_ms"]
    .agg(["mean", "std"])
    .reset_index()
)

# Save averaged results
summary.to_csv(base / "experiment7_summary.csv", index=False)

# Generate plots for each error scenario
for (pd_err, pa_err), cases in df.groupby(
    ["Packet_Error_Prob", "ACK_Error_Prob"]
):

    fig, axes = plt.subplots(
        1, 3,
        figsize=(15, 4.5),
        sharey=False
    )

    # Plot each transfer size
    for ax, (n, subset) in zip(
        axes,
        cases.groupby("Transfer_Size")
    ):

        # Stop-and-Wait reference
        sw = subset[
            subset.Protocol == "Stop-and-Wait"
        ]["Completion_Time_ms"].mean()

        ax.axhline(
            sw,
            color="gray",
            linestyle="--",
            label="Stop-and-Wait"
        )

        # Go-Back-N and Selective Repeat
        for (protocol, wr), group in subset[
            subset.Protocol != "Stop-and-Wait"
        ].groupby(["Protocol", "Wr"]):

            vals = (
                group.groupby("Ws")["Completion_Time_ms"]
                .mean()
                .sort_index()
            )

            ax.plot(
                vals.index,
                vals.values,
                marker="o",
                label=f"{protocol} Wr={wr}"
            )

        ax.set(
            title=f"N = {n}",
            xlabel="Sender window Ws",
            ylabel="Mean completion time (ms)"
        )

        ax.grid(alpha=0.3)

    # Shared legend
    handles, labels = axes[-1].get_legend_handles_labels()

    fig.legend(
        handles,
        labels,
        loc="lower center",
        ncol=3
    )

    fig.suptitle(
        f"Mean Transfer Completion Time vs. Sender Window Size\n"
        f"(pd={pd_err:.2f}, pa={pa_err:.2f})",
        fontsize=14
    )

    fig.tight_layout(rect=[0, 0.13, 1, 0.92])

    # Save PNG
    out = base / (
        f"experiment7_pd{pd_err:.2f}_pa{pa_err:.2f}.png"
    )

    fig.savefig(out, dpi=180)

    print("Saved", out)

print("Saved experiment7_summary.csv")

# Display both plots in interactive figure windows
plt.show()
