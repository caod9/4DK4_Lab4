
/*
 * Simulation_Run of the ALOHA Protocol
 * 
 * Copyright (C) 2014 Terence D. Todd Hamilton, Ontario, CANADA
 * todd@mcmaster.ca
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

/*******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "output.h"
#include "trace.h"
#include "simparameters.h"
#include "cleanup.h"
#include "packet_transmission.h"
#include "main.h"

int
main(void)
{
  unsigned random_seed;
  unsigned RANDOM_SEEDS[] = {RANDOM_SEED_LIST, 0};

  Simulation_Run_Ptr simulation_run;
  Simulation_Run_Data data;
  long int i, max_seq_alloc;
  int j = 0;

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

    /* Allocate tracking arrays with headroom for window beyond RUNLENGTH */
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

    /* Run discrete-event simulation until max_packets are delivered & ACKed */
    while (data.number_of_packets_processed < data.max_packets) {
      simulation_run_execute_event(simulation_run);
    }

    output_results(simulation_run);
    cleanup(simulation_run);
  }

  return 0;
}










