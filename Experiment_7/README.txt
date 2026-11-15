COE4DK4 LAB 4 - EXPERIMENT 7 ONLY

This folder contains a self-contained copy of your previously uploaded ARQ C files.
main.c is updated for Experiment 7. trace.h is fixed to disable verbose tracing.
Other simulation logic is carried over unchanged from the earlier uploaded files.

Experiment settings:
Transfer sizes N=20, 100, 1000; finite source; three original seeds.
Window sizes: S&W (1,1); GBN Ws=2,4,6,8,12,16,24 and Wr=1;
SR same Ws with Wr=2,6,12.
Error cases: (pd=0,pa=0) and (pd=0.05,pa=0.05).

Windows PowerShell (in this folder):
  gcc -O2 main.c packet_transmission.c packet_duration.c output.c cleanup.c channel.c simlib.c csv_logger.c -o experiment7.exe -lm
  .\experiment7.exe > experiment7_log.txt
  py -m pip install pandas matplotlib
  py plot7.py

Outputs: experiment7_results.csv, experiment7_summary.csv, two PNG figures.
The two plots compare N=20/100/1000 in panels for the same error case.

IMPORTANT: This reuses the earlier protocol implementation, including its timer/ACK
semantics; those have not been independently proven correct against every lab rule.
