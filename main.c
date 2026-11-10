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
  int s, p;

  /* Define the configurations you want to test (S&W, GBN, SR) */
  Protocol_Config protocols[] = {
    {"Stop-and-Wait",    1,  1},
    {"Go-Back-N",       12,  1},
    {"Selective-Repeat", 12,  2},
    {"Selective-Repeat", 12,  6},
    {"Selective-Repeat", 12, 12}
  };
  int num_protocols = sizeof(protocols) / sizeof(protocols[0]);

  /* Start with a fresh results file */
  remove("simulation_results.csv");

  printf("==================================================\n");
  printf("RUNNING ARQ COMPARISON ACROSS EXPLICIT SEEDS\n");
  printf("==================================================\n");

  /* 1. Outer Loop: Iterate through each protocol configuration */
  for (p = 0; p < num_protocols; p++) {
    
    /* 2. Inner Loop: Iterate strictly through your provided RANDOM_SEED_LIST */
    s = 0;
    while ((random_seed = RANDOM_SEEDS[s++]) != 0) {

      random_generator_initialize(random_seed);
      simulation_run = (Simulation_Run_Ptr) simulation_run_new();
      simulation_run_set_data(simulation_run, (void *) &data);

      /* Load parameters */
      data.Ws = protocols[p].ws;
      data.Wr = protocols[p].wr;
      data.pd = PACKET_ERROR_PROB;
      data.pa = ACK_ERROR_PROB;
      data.timeout_interval = TIMEOUT_DELAY;
      data.finite_source = FINITE_SOURCE;
      data.max_packets = RUNLENGTH;
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
        "simulation_results.csv",
        "Protocol,Ws,Wr,Random_Seed,Runlength,Packet_Error_Prob,ACK_Error_Prob,Link_Efficiency,Avg_TX_Attempts",
        "%s,%d,%d,%u,%ld,%.2f,%.2f,%.5f,%.5f",
        protocols[p].protocol_name, protocols[p].ws, protocols[p].wr,
        data.random_seed, data.max_packets, data.pd, data.pa, 
        efficiency, avg_tx_attempts
      );

      cleanup(simulation_run);
    }
  }

  printf("\n[SUCCESS] Protocol comparison data exported to 'simulation_results.csv'!\n");
  return 0;
}