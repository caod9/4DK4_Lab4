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

  /* Protocol configurations for verification (S&W, GBN, SR) */
  Protocol_Config protocols[] = {
    {"Go-Back-N",        1,  1},
    {"Go-Back-N",        2,  1},
    {"Go-Back-N",        3,  1},
    {"Go-Back-N",        4,  1},
    {"Go-Back-N",        5,  1},
    {"Go-Back-N",        6,  1},
    {"Go-Back-N",        7,  1},
    {"Go-Back-N",        8,  1},
    {"Go-Back-N",        9,  1},
    {"Go-Back-N",        10,  1},
    {"Go-Back-N",        11,  1},
    {"Go-Back-N",        12,  1},
    {"Go-Back-N",        13,  1},
    {"Go-Back-N",        14,  1},
    {"Go-Back-N",        15,  1},
    {"Go-Back-N",        16,  1},
    {"Go-Back-N",        17,  1},
    {"Go-Back-N",        18,  1},
    {"Go-Back-N",        19,  1},
    {"Go-Back-N",        20,  1},
    
    {"Selective-Repeat", 1, 2},
    {"Selective-Repeat", 2, 2},
    {"Selective-Repeat", 3, 2},
    {"Selective-Repeat", 4, 2},
    {"Selective-Repeat", 5, 2},
    {"Selective-Repeat", 6, 2},
    {"Selective-Repeat", 7, 2},
    {"Selective-Repeat", 8, 2},
    {"Selective-Repeat", 9, 2},
    {"Selective-Repeat", 10, 2},
    {"Selective-Repeat", 11, 2},
    {"Selective-Repeat", 12, 2},
    {"Selective-Repeat", 13, 2},
    {"Selective-Repeat", 14, 2},
    {"Selective-Repeat", 15, 2},
    {"Selective-Repeat", 16, 2},
    {"Selective-Repeat", 17, 2},
    {"Selective-Repeat", 18, 2},
    {"Selective-Repeat", 19, 2},
    {"Selective-Repeat", 20, 2},

    {"Selective-Repeat", 1, 6},
    {"Selective-Repeat", 2, 6},
    {"Selective-Repeat", 3, 6},
    {"Selective-Repeat", 4, 6},
    {"Selective-Repeat", 5, 6},
    {"Selective-Repeat", 6, 6},
    {"Selective-Repeat", 7, 6},
    {"Selective-Repeat", 8, 6},
    {"Selective-Repeat", 9, 6},
    {"Selective-Repeat", 10, 6},
    {"Selective-Repeat", 11, 6},
    {"Selective-Repeat", 12, 6},
    {"Selective-Repeat", 13, 6},
    {"Selective-Repeat", 14, 6},
    {"Selective-Repeat", 15, 6},
    {"Selective-Repeat", 16, 6},
    {"Selective-Repeat", 17, 6},
    {"Selective-Repeat", 18, 6},
    {"Selective-Repeat", 19, 6},
    {"Selective-Repeat", 20, 6},

    {"Selective-Repeat", 1, 12},
    {"Selective-Repeat", 2, 12},
    {"Selective-Repeat", 3, 12},
    {"Selective-Repeat", 4, 12},
    {"Selective-Repeat", 5, 12},
    {"Selective-Repeat", 6, 12},
    {"Selective-Repeat", 7, 12},
    {"Selective-Repeat", 8, 12},
    {"Selective-Repeat", 9, 12},
    {"Selective-Repeat", 10, 12},
    {"Selective-Repeat", 11, 12},
    {"Selective-Repeat", 12, 12},
    {"Selective-Repeat", 13, 12},
    {"Selective-Repeat", 14, 12},
    {"Selective-Repeat", 15, 12},
    {"Selective-Repeat", 16, 12},
    {"Selective-Repeat", 17, 12},
    {"Selective-Repeat", 18, 12},
    {"Selective-Repeat", 19, 12},
    {"Selective-Repeat", 20, 12}
    

    
  };
  int num_protocols = sizeof(protocols) / sizeof(protocols[0]);

  /* Start with a clean CSV file */
  remove("simulation_results.csv");

  for (p = 0; p < num_protocols; p++) {
    s = 0;
    while ((random_seed = RANDOM_SEEDS[s++]) != 0) {

      random_generator_initialize(random_seed);
      simulation_run = (Simulation_Run_Ptr) simulation_run_new();
      simulation_run_set_data(simulation_run, (void *) &data);

      /* Load parameters from simparameters.h and protocol table */
      data.Ws = protocols[p].ws;
      data.Wr = protocols[p].wr;
      data.pd = PACKET_ERROR_PROB;
      data.pa = ACK_ERROR_PROB;
      data.timeout_interval = TIMEOUT_DELAY;
      data.finite_source = FINITE_SOURCE;
      data.max_packets = RUNLENGTH;
      data.random_seed = random_seed;

      /* Initialize Sender & Receiver state */
      data.S_base = 0;
      data.next_seq = 0;
      data.tx_busy = 0;
      data.R_next = 0;
      data.blip_counter = 0;
      data.total_tx_attempts = 0;
      data.number_of_packets_processed = 0;
      data.retx_queue = fifoqueue_new();

      /* Initialize Diagnostic / Verification Counters */
      data.pkt_drops = 0;
      data.ack_drops = 0;
      data.timeouts = 0;
      data.retx_count = 0;
      data.ooo_rejected = 0;
      data.ooo_buffered = 0;

      /* Allocate tracking arrays */
      max_seq_alloc = data.max_packets + data.Ws + 1000;
      data.acked = (int *) xcalloc((unsigned) max_seq_alloc, sizeof(int));
      data.in_retx_queue = (int *) xcalloc((unsigned) max_seq_alloc, sizeof(int));
      data.timer_event_id = (long int *) xcalloc((unsigned) max_seq_alloc, sizeof(long int));
      data.recv_buf = (int *) xcalloc((unsigned) max_seq_alloc, sizeof(int));

      for (i = 0; i < max_seq_alloc; i++) {
        data.timer_event_id[i] = -1;
      }

      /* Start transmitting at t = 0.0 */
      try_send_next_packet(simulation_run);

      /* Execute events until we finish the run length */
      while (data.number_of_packets_processed < data.max_packets) {
        simulation_run_execute_event(simulation_run);
      }

      /* Print out console results */
      output_results(simulation_run);

/* Compute performance metrics */
      sim_time = simulation_run_get_time(simulation_run);
      efficiency = (data.number_of_packets_processed * MEAN_PACKET_DURATION) / sim_time;
      avg_tx_attempts = (double) data.total_tx_attempts / data.number_of_packets_processed;

      /* Log row to CSV including Total_Time_ms */
      log_row_to_csv(
        "simulation_results.csv",
        "Protocol,Ws,Wr,Random_Seed,Runlength,pd,pa,Pkt_Drops,ACK_Drops,Timeouts,Retx_Count,OOO_Rejected,OOO_Buffered,Total_Time_ms,Link_Efficiency,Avg_TX_Attempts",
        "%s,%d,%d,%u,%ld,%.2f,%.2f,%ld,%ld,%ld,%ld,%ld,%ld,%.2f,%.5f,%.5f",
        protocols[p].protocol_name, data.Ws, data.Wr,
        data.random_seed, data.max_packets, data.pd, data.pa,
        data.pkt_drops, data.ack_drops, data.timeouts, data.retx_count,
        data.ooo_rejected, data.ooo_buffered,
        sim_time, efficiency, avg_tx_attempts
      );

      /* Clean up memory for this seed run */
      cleanup(simulation_run);
    }
  }

  printf("\n[SUCCESS] Results successfully exported to 'simulation_results.csv'!\n");
  return 0;
}