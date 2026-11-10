
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

#include "simlib.h"
#include "main.h"
#include "simparameters.h"
#include "cleanup.h"

void
cleanup(Simulation_Run_Ptr simulation_run)
{
  Simulation_Run_Data_Ptr data;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);

  while (fifoqueue_size(data->retx_queue) > 0) {
    xfree(fifoqueue_get(data->retx_queue));
  }
  xfree(data->retx_queue);

  xfree(data->acked);
  xfree(data->in_retx_queue);
  xfree(data->timer_event_id);
  xfree(data->recv_buf);

  simulation_run_free_memory(simulation_run);
}