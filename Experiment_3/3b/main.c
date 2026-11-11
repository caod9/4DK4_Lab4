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

/* Structure to define each protocol configuration to test */
typedef struct {
  const char *protocol_name;
  int ws;
  int wr;
} Protocol_Config;

int
main(void)
{
  unsigned random_seed;
  unsigned RANDOM_SEEDS[] = {RANDOM_SEED_LIST, 0};

  Simulation_Run_Ptr simulation_run;
  Simulation_Run_Data data;
  long int i, max_seq_alloc;
  double sim_time, efficiency, avg_tx_attempts;
  int s, p, w;
  /* Experiment 3(b): sweep the sender window size. */
  const int ws_values[] = {1, 2, 4, 6, 8, 10, 12, 16, 20, 24, 32};
  const int num_ws = (int)(sizeof(ws_values) / sizeof(ws_values[0]));

  /* Go-Back-N: Wr = 1; Selective Repeat: three receiver windows. */
  Protocol_Config protocols[] = {
    {"Go-Back-N",        1,  1},
    {"Selective-Repeat", 1,  2},
    {"Selective-Repeat", 1,  6},
    {"Selective-Repeat", 1, 12}
  };
  const int num_protocols = (int)(sizeof(protocols) / sizeof(protocols[0]));

  /* Start with a fresh results file */
  remove("experiment3b_results.csv");

  printf("==================================================\n");
  printf("EXPERIMENT 3(b): METRICS VS SENDER WINDOW SIZE\n");
  printf("==================================================\n");

  /* Sweep sender window size, then protocol, then seed. */
  for (w = 0; w < num_ws; w++) {
    printf("Running Ws = %d\n", ws_values[w]);
    for (p = 0; p < num_protocols; p++) {
    
    /* 2. Inner Loop: Iterate strictly through your provided RANDOM_SEED_LIST */
    s = 0;
    while ((random_seed = RANDOM_SEEDS[s++]) != 0) {

      random_generator_initialize(random_seed);
      simulation_run = (Simulation_Run_Ptr) simulation_run_new();
      simulation_run_set_data(simulation_run, (void *) &data);

      /* Load parameters */
      data.Ws = ws_values[w];
      data.Wr = protocols[p].wr;
      data.pd = 0.10; /* Fixed nonzero packet error probability */
      data.pa = 0.0;  /* Experiment 3(b): ACKs are error-free */
      data.timeout_interval = TIMEOUT_DELAY;
      data.finite_source = 0; /* Infinite backlog */
      data.max_packets = 10000; /* Long run for performance plots */
      data.random_seed = random_seed;

      /* Initialize State */
      data.S_base = 0;
      data.next_seq = 0;
      data.tx_busy = 0;
      data.R_next = 0;
      data.blip_counter = 0;
      data.total_tx_attempts = 0;
      data.number_of_packets_processed = 0;
      data.retx_queue = fifoqueue_new();

      /* Allocate tracking arrays */
      max_seq_alloc = data.max_packets + data.Ws + 1000;
      data.acked = (int *) xcalloc((unsigned) max_seq_alloc, sizeof(int));
      data.in_retx_queue = (int *) xcalloc((unsigned) max_seq_alloc, sizeof(int));
      data.timer_event_id = (long int *) xcalloc((unsigned) max_seq_alloc, sizeof(long int));
      data.recv_buf = (int *) xcalloc((unsigned) max_seq_alloc, sizeof(int));

      for (i = 0; i < max_seq_alloc; i++) {
        data.timer_event_id[i] = -1;
      }

      /* Run simulation */
      try_send_next_packet(simulation_run);
      while (data.number_of_packets_processed < data.max_packets) {
        simulation_run_execute_event(simulation_run);
      }

      /* Compute metrics */
      sim_time = simulation_run_get_time(simulation_run);
      efficiency = (data.number_of_packets_processed * MEAN_PACKET_DURATION) / sim_time;
      avg_tx_attempts = (double) data.total_tx_attempts / data.number_of_packets_processed;

      /* Log row to CSV with runlength included */
      log_row_to_csv(
        "experiment3b_results.csv",
        "Protocol,Ws,Wr,Random_Seed,Runlength,Packet_Error_Prob,ACK_Error_Prob,Link_Efficiency,Avg_TX_Attempts",
        "%s,%d,%d,%u,%ld,%.2f,%.2f,%.5f,%.5f",
        protocols[p].protocol_name, data.Ws, data.Wr,
        data.random_seed, data.max_packets, data.pd, data.pa, 
        efficiency, avg_tx_attempts
      );

      cleanup(simulation_run);
    }
    }
  }

  printf("\n[SUCCESS] Experiment 3(b) results exported to 'experiment3b_results.csv'!\n");
  return 0;
}