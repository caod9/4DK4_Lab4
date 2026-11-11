
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
#ifndef _PACKET_TRANSMISSION_H_
#define _PACKET_TRANSMISSION_H_

#include "main.h"

/*******************************************************************************/

/* Helper to start transmitting next eligible packet if link is idle */
void
try_send_next_packet(Simulation_Run_Ptr);

/* 1. End of 1 ms Data Packet Transmission at Source */
long int
schedule_transmission_end_event(Simulation_Run_Ptr, Time, void *);
void
transmission_end_event(Simulation_Run_Ptr, void *);

/* 2. Data Packet Arrival at Destination (after 5 ms prop delay) */
long int
schedule_packet_arrival_at_dest_event(Simulation_Run_Ptr, Time, void *);
void
packet_arrival_at_dest_event(Simulation_Run_Ptr, void *);

/* 3. ACK Arrival at Source (after 5 ms prop delay) */
long int
schedule_ack_arrival_at_src_event(Simulation_Run_Ptr, Time, void *);
void
ack_arrival_at_src_event(Simulation_Run_Ptr, void *);

/* 4. Retransmission Timer Expiration at Source (after Tout ms) */
long int
schedule_timeout_event(Simulation_Run_Ptr, Time, void *);
void
timeout_event(Simulation_Run_Ptr, void *);

/*******************************************************************************/

#endif /* packet_transmission.h */




