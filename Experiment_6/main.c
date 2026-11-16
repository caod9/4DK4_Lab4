#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "output.h"
#include "trace.h"
#include "simparameters.h"
#include "cleanup.h"
#include "packet_transmission.h"
#include "csv_logger.h"
#include "main.h"

/* ---------------- Part 6 settings ---------------- */
#define NUM_PACKETS_N     100     /* finite transfer size N */
#define NUM_REPLICATIONS  500     /* independent transfers per seed per config */
#define RESULTS_FILE      "experiment6_results.csv"

typedef struct {
    const char *name;
    double pd;
    double pa;
} Error_Case;

/* Run ONE finite transfer; returns completion time in ms. */
static double run_one_transfer(Simulation_Run_Data *data, int ws, int wr,
                               double pd, double pa, long int n,
                               unsigned seed)
{
    Simulation_Run_Ptr simulation_run;
    long int i, max_seq_alloc;
    double completion_time;

    simulation_run = simulation_run_new();
    simulation_run_set_data(simulation_run, (void *)data);

    data->Ws = ws;
    data->Wr = wr;
    data->pd = pd;
    data->pa = pa;
    data->timeout_interval = TIMEOUT_DELAY;
    data->finite_source = 1;           /* finite source of N packets at t = 0 */
    data->max_packets = n;
    data->random_seed = seed;

    data->S_base = 0;
    data->next_seq = 0;
    data->tx_busy = 0;
    data->R_next = 0;
    data->blip_counter = 0;
    data->total_tx_attempts = 0;
    data->number_of_packets_processed = 0;
    data->retx_queue = fifoqueue_new();

    max_seq_alloc = n + ws + 1000;
    data->acked = (int *)xcalloc((unsigned)max_seq_alloc, sizeof(int));
    data->in_retx_queue = (int *)xcalloc((unsigned)max_seq_alloc, sizeof(int));
    data->timer_event_id = (long int *)xcalloc((unsigned)max_seq_alloc, sizeof(long int));
    data->recv_buf = (int *)xcalloc((unsigned)max_seq_alloc, sizeof(int));
    for (i = 0; i < max_seq_alloc; i++) data->timer_event_id[i] = -1;

    /* First transmission begins at t = 0 */
    try_send_next_packet(simulation_run);
    while (data->number_of_packets_processed < data->max_packets) {
        simulation_run_execute_event(simulation_run);
    }

    /* Clock when the ACK for the last packet reaches the source */
    completion_time = simulation_run_get_time(simulation_run);

    cleanup(simulation_run);
    return completion_time;
}

/* Run NUM_REPLICATIONS transfers for one seed, log mean/std to CSV. */
static void run_config(Simulation_Run_Data *data, const char *name,
                       int ws, int wr, const Error_Case *ec, unsigned seed)
{
    int r;
    double t, sum = 0.0, sumsq = 0.0, mean, var, std;

    /* Seed once; replications continue the same random stream. */
    random_generator_initialize(seed);

    for (r = 0; r < NUM_REPLICATIONS; r++) {
        t = run_one_transfer(data, ws, wr, ec->pd, ec->pa,
                             NUM_PACKETS_N, seed);
        sum += t;
        sumsq += t * t;
    }
    mean = sum / NUM_REPLICATIONS;
    var = (sumsq - NUM_REPLICATIONS * mean * mean) / (NUM_REPLICATIONS - 1);
    std = (var > 0.0) ? sqrt(var) : 0.0;

    log_row_to_csv(
        RESULTS_FILE,
        "Protocol,Ws,Wr,Random_Seed,N,Replications,"
        "Packet_Error_Prob,ACK_Error_Prob,Timeout,"
        "Mean_Completion_Time,Std_Completion_Time",
        "%s,%d,%d,%u,%d,%d,%.2f,%.2f,%.2f,%.5f,%.5f",
        name, ws, wr, seed, NUM_PACKETS_N, NUM_REPLICATIONS,
        ec->pd, ec->pa, (double)TIMEOUT_DELAY, mean, std);
}

int main(void)
{
    unsigned random_seed;
    unsigned RANDOM_SEEDS[] = {RANDOM_SEED_LIST, 0};
    Simulation_Run_Data data;
    int c, w, k, s;

    /* The three error cases required by Part 6 */
    const Error_Case cases[] = {
        {"pd=0, pa=0",        0.00, 0.00},
        {"pd=0.05, pa=0",     0.05, 0.00},
        {"pd=0.05, pa=0.05",  0.05, 0.05}
    };
    const int num_cases = (int)(sizeof(cases) / sizeof(cases[0]));

    /* Ws sweep: spans below and above the ~11 needed to fill the pipe
       (RTT = 11 ms incl. 1 ms Tx time) */
    const int ws_values[] = {1, 2, 3, 4, 5, 6, 8, 10, 11, 12, 14, 16, 20, 24};
    const int num_ws = (int)(sizeof(ws_values) / sizeof(ws_values[0]));

    /* Three SR receive-window sizes (Wr > 1); only run when Wr <= Ws */
    const int wr_values[] = {2, 6, 12};
    const int num_wr = (int)(sizeof(wr_values) / sizeof(wr_values[0]));

    remove(RESULTS_FILE);

    printf("==================================================\n");
    printf("EXPERIMENT 6: FINITE TRANSFER (N = %d, %d reps/seed)\n",
           NUM_PACKETS_N, NUM_REPLICATIONS);
    printf("==================================================\n");

    for (c = 0; c < num_cases; c++) {
        printf("\n\nCase: %s\n", cases[c].name);

        for (w = 0; w < num_ws; w++) {
            int ws = ws_values[w];
            printf("\n  Ws = %d\n", ws);

            for (s = 0; (random_seed = RANDOM_SEEDS[s]) != 0; s++) {
                if (ws == 1) {
                    /* Stop-and-Wait reference */
                    run_config(&data, "Stop-and-Wait", 1, 1,
                               &cases[c], random_seed);
                } else {
                    run_config(&data, "Go-Back-N", ws, 1,
                               &cases[c], random_seed);
                    for (k = 0; k < num_wr; k++) {
                        if (wr_values[k] <= ws) {
                            run_config(&data, "Selective-Repeat", ws,
                                       wr_values[k], &cases[c], random_seed);
                        }
                    }
                }
            }
        }
    }

    printf("\n\n[SUCCESS] Experiment 6 results exported to '%s'!\n", RESULTS_FILE);
    return 0;
}