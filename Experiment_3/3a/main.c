
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

/* Structure to define each protocol configuration */
typedef struct {
    const char *protocol_name;
    int ws;
    int wr;
} Protocol_Config;

int main(void)
{
    unsigned random_seed;
    unsigned RANDOM_SEEDS[] = {RANDOM_SEED_LIST, 0};

    Simulation_Run_Ptr simulation_run;
    Simulation_Run_Data data;

    long int i, max_seq_alloc;
    double sim_time, efficiency, avg_tx_attempts;

    int s, p, e;

    /* Experiment 3(a): Packet error probability sweep */
    const double pd_values[] = {
        0.00, 0.05, 0.10, 0.15, 0.20, 0.25,
        0.30, 0.35, 0.40, 0.45, 0.50
    };

    const int num_pd =
        sizeof(pd_values) / sizeof(pd_values[0]);

    /* Protocol configurations */
    Protocol_Config protocols[] = {
        {"Stop-and-Wait",     1,  1},
        {"Go-Back-N",        12,  1},
        {"Selective-Repeat", 12,  2},
        {"Selective-Repeat", 12,  6},
        {"Selective-Repeat", 12, 12}
    };

    int num_protocols =
        sizeof(protocols) / sizeof(protocols[0]);

    /* Start with a fresh CSV */
    remove("experiment3a_results.csv");

    printf("==================================================\n");
    printf("EXPERIMENT 3(a): EFFICIENCY VS PACKET ERROR PROBABILITY\n");
    printf("==================================================\n");

    /* Loop through packet error probabilities */
    for (e = 0; e < num_pd; e++) {

        printf("Running pd = %.2f\n", pd_values[e]);

        /* Loop through protocol configurations */
        for (p = 0; p < num_protocols; p++) {

            /* Loop through random seeds */
            s = 0;

            while ((random_seed = RANDOM_SEEDS[s++]) != 0) {

                random_generator_initialize(random_seed);

                simulation_run = simulation_run_new();
                simulation_run_set_data(
                    simulation_run, (void *)&data
                );

                /* Configure simulation parameters */
                data.Ws = protocols[p].ws;
                data.Wr = protocols[p].wr;

                data.pd = pd_values[e];
                data.pa = 0.0;

                data.timeout_interval = TIMEOUT_DELAY;

                data.finite_source = 0;
                data.max_packets = 10000;

                data.random_seed = random_seed;

                /* Initialize sender and receiver state */
                data.S_base = 0;
                data.next_seq = 0;
                data.tx_busy = 0;
                data.R_next = 0;

                data.blip_counter = 0;
                data.total_tx_attempts = 0;
                data.number_of_packets_processed = 0;

                data.retx_queue = fifoqueue_new();

                /* Allocate tracking arrays */
                max_seq_alloc =
                    data.max_packets + data.Ws + 1000;

                data.acked = (int *)xcalloc(
                    (unsigned)max_seq_alloc, sizeof(int)
                );

                data.in_retx_queue = (int *)xcalloc(
                    (unsigned)max_seq_alloc, sizeof(int)
                );

                data.timer_event_id = (long int *)xcalloc(
                    (unsigned)max_seq_alloc, sizeof(long int)
                );

                data.recv_buf = (int *)xcalloc(
                    (unsigned)max_seq_alloc, sizeof(int)
                );

                for (i = 0; i < max_seq_alloc; i++) {
                    data.timer_event_id[i] = -1;
                }

                /* Start simulation */
                try_send_next_packet(simulation_run);

                while (
                    data.number_of_packets_processed
                    < data.max_packets
                ) {
                    simulation_run_execute_event(
                        simulation_run
                    );
                }

                /* Calculate performance metrics */
                sim_time =
                    simulation_run_get_time(simulation_run);

                efficiency =
                    (data.number_of_packets_processed *
                     MEAN_PACKET_DURATION) / sim_time;

                avg_tx_attempts =
                    (double)data.total_tx_attempts /
                    data.number_of_packets_processed;

                /* Export results to CSV */
                log_row_to_csv(
                    "experiment3a_results.csv",

                    "Protocol,Ws,Wr,Random_Seed,Runlength,"
                    "Packet_Error_Prob,ACK_Error_Prob,"
                    "Link_Efficiency,Avg_TX_Attempts",

                    "%s,%d,%d,%u,%ld,%.2f,%.2f,%.5f,%.5f",

                    protocols[p].protocol_name,
                    protocols[p].ws,
                    protocols[p].wr,

                    data.random_seed,
                    data.max_packets,

                    data.pd,
                    data.pa,

                    efficiency,
                    avg_tx_attempts
                );

                /* Free memory */
                cleanup(simulation_run);
            }
        }
    }

    printf(
        "\n[SUCCESS] Experiment 3(a) results "
        "exported to 'experiment3a_results.csv'!\n"
    );

    return 0;
}
