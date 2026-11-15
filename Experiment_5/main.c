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

    /* Experiment 5: tout sweep, with all other params fixed */
    const double tout_values[] = {
        5.00, 10.00, 11.00, 12.00, 15.00, 20.00, 25.00, 30.00, 35.00, 40.00, 45.00, 50.00
    };

    const int num_tout = (int)(sizeof(tout_values) / sizeof(tout_values[0]));

    /* Same protocol window sizes as Experiment 3(a) */
    Protocol_Config protocols[] = {
        {"Stop-and-Wait",     1,  1},
        {"Go-Back-N",        12,  1},
        {"Selective-Repeat", 12,  2},
        {"Selective-Repeat", 12,  6},
        {"Selective-Repeat", 12, 12}
    };
    const int num_protocols = (int)(sizeof(protocols) / sizeof(protocols[0]));

    remove("experiment5_results.csv");

    printf("==================================================\n");
    printf("EXPERIMENT 5: VARIATION IN RETRANSMISSION TIMEOUT\n");
    printf("==================================================\n");

    for (e = 0; e < num_tout; e++) {
        printf("\nRunning tout = %.2f\n", tout_values[e]);

        for (p = 0; p < num_protocols; p++) {
            s = 0;
            while ((random_seed = RANDOM_SEEDS[s++]) != 0) {
                random_generator_initialize(random_seed);

                simulation_run = simulation_run_new();
                simulation_run_set_data(simulation_run, (void *)&data);

                data.Ws = protocols[p].ws;
                data.Wr = protocols[p].wr;
                data.pd = 0.1;
                data.pa = 0.1;
                data.timeout_interval = tout_values[e];
                data.finite_source = 0; /* Continuously backlogged source */
                data.max_packets = 10000;
                // data.max_packets = 20;
                data.random_seed = random_seed;

                data.S_base = 0;
                data.next_seq = 0;
                data.tx_busy = 0;
                data.R_next = 0;
                data.blip_counter = 0;
                data.total_tx_attempts = 0;
                data.number_of_packets_processed = 0;
                data.retx_queue = fifoqueue_new();

                max_seq_alloc = data.max_packets + data.Ws + 1000;
                data.acked = (int *)xcalloc((unsigned)max_seq_alloc, sizeof(int));
                data.in_retx_queue = (int *)xcalloc((unsigned)max_seq_alloc, sizeof(int));
                data.timer_event_id = (long int *)xcalloc((unsigned)max_seq_alloc, sizeof(long int));
                data.recv_buf = (int *)xcalloc((unsigned)max_seq_alloc, sizeof(int));

                for (i = 0; i < max_seq_alloc; i++) {
                    data.timer_event_id[i] = -1;
                }

                try_send_next_packet(simulation_run);
                while (data.number_of_packets_processed < data.max_packets) {
                    simulation_run_execute_event(simulation_run);
                }

                sim_time = simulation_run_get_time(simulation_run);
                efficiency = (data.number_of_packets_processed * MEAN_PACKET_DURATION) / sim_time;
                avg_tx_attempts = (double)data.total_tx_attempts / data.number_of_packets_processed;

                log_row_to_csv(
                    "experiment5_results.csv",
                    "Protocol,Ws,Wr,Random_Seed,Runlength,"
                    "Packet_Error_Prob,ACK_Error_Prob,Timeout,"
                    "Link_Efficiency,Avg_TX_Attempts",
                    "%s,%d,%d,%u,%ld,%.2f,%.2f,%.2f,%.5f,%.5f",
                    protocols[p].protocol_name,
                    data.Ws,
                    data.Wr,
                    data.random_seed,
                    data.max_packets,
                    data.pd,
                    data.pa,
                    data.timeout_interval,
                    efficiency,
                    avg_tx_attempts
                );

                cleanup(simulation_run);
            }
        }
    }

    printf("\n[SUCCESS] Experiment 5 results exported to 'experiment5_results.csv'!\n");
    return 0;
}
