/* COE4DK4 Lab 4 - Experiment 7: Effect of finite transfer size.
 * Based on the ARQ simulation used for Experiments 3 and 4.
 * Repeat a suitable subset of Part 6 without running Part 6 separately.
 */
#include <stdio.h>
#include <stdlib.h>
#include "output.h"
#include "trace.h"
#include "simparameters.h"
#include "cleanup.h"
#include "packet_transmission.h"
#include "csv_logger.h"
#include "main.h"

typedef struct { const char *name; int ws, wr; } Protocol_Config;

int main(void)
{
    const long int transfer_sizes[] = {20, 100, 1000};
    const int sender_windows[] = {2, 4, 6, 8, 12, 16, 24};
    const int receiver_windows[] = {2, 6, 12};
    /* Suitable subset of Part 6: error-free and both data+ACK errors. */
    const double error_cases[][2] = {{0.0, 0.0}, {0.05, 0.05}};
    const unsigned seeds[] = {RANDOM_SEED_LIST, 0};
    const char *csv = "experiment7_results.csv";
    int n, ec, w, r, s, mode;
    remove(csv);
    for (n = 0; n < (int)(sizeof(transfer_sizes)/sizeof(transfer_sizes[0])); ++n) {
        for (ec = 0; ec < (int)(sizeof(error_cases)/sizeof(error_cases[0])); ++ec) {
            /* mode: 0=S&W, 1=GBN, 2=SR */
            for (mode = 0; mode < 3; ++mode) {
                int nw = (mode == 0) ? 1 : (int)(sizeof(sender_windows)/sizeof(sender_windows[0]));
                int nr = (mode == 2) ? (int)(sizeof(receiver_windows)/sizeof(receiver_windows[0])) : 1;
                for (w = 0; w < nw; ++w) for (r = 0; r < nr; ++r) {
                    Protocol_Config cfg;
                    cfg.name = mode == 0 ? "Stop-and-Wait" : mode == 1 ? "Go-Back-N" : "Selective-Repeat";
                    cfg.ws = mode == 0 ? 1 : sender_windows[w];
                    cfg.wr = mode == 2 ? receiver_windows[r] : 1;
                    for (s = 0; seeds[s] != 0; ++s) {
                        Simulation_Run_Data data = {0};
                        Simulation_Run_Ptr run;
                        long int i, alloc_size;
                        double completion;
                        random_generator_initialize(seeds[s]);
                        run = simulation_run_new();
                        simulation_run_set_data(run, (void *)&data);
                        data.Ws = cfg.ws;
                        data.Wr = cfg.wr;
                        data.pd = error_cases[ec][0];
                        data.pa = error_cases[ec][1];
                        data.timeout_interval = TIMEOUT_DELAY;
                        data.finite_source = 1;
                        data.max_packets = transfer_sizes[n];
                        data.random_seed = seeds[s];
                        data.retx_queue = fifoqueue_new();
                        /* Finite source never creates seq >= N. */
                        alloc_size = data.max_packets + data.Ws + 2;
                        data.acked = (int *)xcalloc((unsigned)alloc_size, sizeof(int));
                        data.in_retx_queue = (int *)xcalloc((unsigned)alloc_size, sizeof(int));
                        data.timer_event_id = (long int *)xcalloc((unsigned)alloc_size, sizeof(long int));
                        data.recv_buf = (int *)xcalloc((unsigned)alloc_size, sizeof(int));
                        for (i = 0; i < alloc_size; ++i) data.timer_event_id[i] = -1;
                        try_send_next_packet(run);
                        while (data.number_of_packets_processed < data.max_packets) {
                            simulation_run_execute_event(run);
                        }
                        completion = simulation_run_get_time(run);
                        log_row_to_csv(csv,
                            "Protocol,Ws,Wr,Random_Seed,Transfer_Size,Packet_Error_Prob,ACK_Error_Prob,Completion_Time_ms,Total_TX_Attempts",
                            "%s,%d,%d,%u,%ld,%.2f,%.2f,%.5f,%ld",
                            cfg.name, cfg.ws, cfg.wr, data.random_seed,
                            data.max_packets, data.pd, data.pa, completion, data.total_tx_attempts);
                        printf("N=%ld pd=%.2f pa=%.2f %-16s Ws=%2d Wr=%2d seed=%u completion=%.2f ms\n",
                               data.max_packets, data.pd, data.pa, cfg.name, cfg.ws, cfg.wr, data.random_seed, completion);
                        cleanup(run);
                    }
                }
            }
        }
    }
    printf("\nExperiment 7 complete: %s\n", csv);
    return 0;
}
