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

int
main(void)
{
  unsigned random_seed;
  unsigned RANDOM_SEEDS[] = {RANDOM_SEED_LIST, 0};

  Simulation_Run_Ptr simulation_run;
  Simulation_Run_Data data;
  long int i, max_seq_alloc;
  double sim_time, efficiency, avg_tx_attempts;
  int j = 0;

  /* Do a new simulation run for each random number generator seed. */
  while ((random_seed = RANDOM_SEEDS[j++]) != 0) {

    random_generator_initialize(random_seed);
    simulation_run = (Simulation_Run_Ptr) simulation_run_new();
    simulation_run_set_data(simulation_run, (void *) &data);

    /* Load parameters from simparameters.h */
    data.Ws = SEND_WINDOW_SIZE;
    data.Wr = RECV_WINDOW_SIZE;
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

    /* =========================================================================
     * GENERIC CSV EXPORT LOG CALL
     * Easily customizable or repeatable for any future parts / experiments!
     * ========================================================================= */
    sim_time = simulation_run_get_time(simulation_run);
    efficiency = (data.number_of_packets_processed * MEAN_PACKET_DURATION) / sim_time;
    avg_tx_attempts = (double) data.total_tx_attempts / data.number_of_packets_processed;

    log_row_to_csv(
      "simulation_results.csv",                                   /* Filename */
      "Random_Seed,Ws,Wr,Packet_Error_Prob,ACK_Error_Prob,Timeout,Delivered_Packets,Total_Time_ms,Link_Efficiency,Avg_TX_Attempts", /* Headers */
      "%u,%d,%d,%.2f,%.2f,%.1f,%ld,%.2f,%.5f,%.5f",               /* Format string */
      data.random_seed, data.Ws, data.Wr, data.pd, data.pa, 
      data.timeout_interval, data.number_of_packets_processed, 
      sim_time, efficiency, avg_tx_attempts
    );

    /* Clean up memory for this seed run. */
    cleanup(simulation_run);
  }

  printf("\n[SUCCESS] Results successfully appended to 'simulation_results.csv'!\n");
  return 0;
}