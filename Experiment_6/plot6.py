import pandas as pd
import matplotlib.pyplot as plt

# Experiment 6: finite transfer (N = 100). Each row is the mean over many
# replications for one seed; here we average those across the three seeds.
df = pd.read_csv("experiment6_results.csv")

avg = (
    df.groupby(["Protocol", "Ws", "Wr", "Packet_Error_Prob", "ACK_Error_Prob"],
               as_index=False)[["Mean_Completion_Time"]]
      .mean()
)

N = int(df["N"].iloc[0])

# (pd, pa, filename)
cases = [
    (0.00, 0.00, "experiment6_pd0_pa0.png"),
    (0.05, 0.00, "experiment6_pd005_pa0.png"),
    (0.05, 0.05, "experiment6_pd005_pa005.png"),
]

# (Protocol, Wr, label, color, marker)
curves = [
    ("Go-Back-N",        1,  "Go-Back-N (Wr=1)",          "orange", "o"),
    ("Selective-Repeat", 2,  "Selective Repeat (Wr=2)",   "green",  "s"),
    ("Selective-Repeat", 6,  "Selective Repeat (Wr=6)",   "red",    "^"),
    ("Selective-Repeat", 12, "Selective Repeat (Wr=12)",  "purple", "D"),
]


def make_plot(pd_val, pa_val, filename):
    sub = avg[(avg["Packet_Error_Prob"].round(4) == pd_val)
              & (avg["ACK_Error_Prob"].round(4) == pa_val)]

    plt.figure(figsize=(9, 6))

    # Stop-and-Wait reference (Ws = 1): single marker + dashed reference line
    sw = sub[sub["Protocol"] == "Stop-and-Wait"]
    if not sw.empty:
        y = sw["Mean_Completion_Time"].iloc[0]
        plt.axhline(y, color="blue", linestyle="--", linewidth=1, alpha=0.6)
        plt.plot([1], [y], marker="*", markersize=12, linestyle="None",
                 color="blue", label="Stop-and-Wait (Ws=1, Wr=1)")

    for protocol, wr, label, color, marker in curves:
        d = sub[(sub["Protocol"] == protocol) & (sub["Wr"] == wr)].sort_values("Ws")
        plt.plot(d["Ws"], d["Mean_Completion_Time"],
                 marker=marker, linestyle="None", markersize=7,
                 color=color, label=label)   # markers only, no interpolation

    plt.xlabel("Send Window Size Ws (packets)")
    plt.ylabel("Mean Transfer Completion Time (ms)")
    plt.title(f"Experiment 6: Completion Time vs Ws (N={N}, "
              f"pd={pd_val}, pa={pa_val}, Tout=15 ms)")
    plt.xticks(sorted(avg["Ws"].unique()))
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(filename, dpi=300)


for pd_val, pa_val, fname in cases:
    make_plot(pd_val, pa_val, fname)

plt.show()