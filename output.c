
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

#include <stdio.h>
#include "simparameters.h"
#include "main.h"
#include "output.h"

void
output_blip_to_screen(Simulation_Run_Ptr simulation_run)
{
  double percentagedone;
  Simulation_Run_Data_Ptr data;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  data->blip_counter++;

  if ((data->blip_counter >= BLIPRATE) ||
      (data->number_of_packets_processed >= data->max_packets)) {
    data->blip_counter = 0;
    percentagedone = 100.0 * (double) data->number_of_packets_processed / data->max_packets;
    printf("%3.0f%% Successfully Delivered Pkts = %ld (Total Tx Attempts = %ld)\r",
           percentagedone, data->number_of_packets_processed, data->total_tx_attempts);
    fflush(stdout);
  }
}

void
output_results(Simulation_Run_Ptr this_simulation_run)
{
  double sim_time, efficiency, avg_attempts;
  Simulation_Run_Data_Ptr sim_data;

  sim_data = (Simulation_Run_Data_Ptr) simulation_run_data(this_simulation_run);
  sim_time = simulation_run_get_time(this_simulation_run);

  efficiency = (sim_data->number_of_packets_processed * MEAN_PACKET_DURATION) / sim_time;
  avg_attempts = (double) sim_data->total_tx_attempts / sim_data->number_of_packets_processed;

  printf("\n************************************************************\n");
  printf("Random Seed               = %u\n", sim_data->random_seed);
  printf("Protocol Settings         = Ws=%d, Wr=%d, pd=%.2f, pa=%.2f, Tout=%.1f ms\n",
         sim_data->Ws, sim_data->Wr, sim_data->pd, sim_data->pa, sim_data->timeout_interval);
  printf("Delivered Packets         = %ld\n", sim_data->number_of_packets_processed);
  printf("Total Tx Attempts         = %ld\n", sim_data->total_tx_attempts);
  printf("Total Completion Time     = %.2f ms\n", sim_time);
  printf("Link Efficiency           = %.5f\n", efficiency);
  printf("Avg Tx Attempts / Packet  = %.5f\n", avg_attempts);
  printf("************************************************************\n\n");
}