
/*
 * 
 * Simulation_Run of the ALOHA Protocol
 * 
 * Copyright (C) 2014 Terence D. Todd
 * Hamilton, Ontario, CANADA
 * todd@mcmaster.ca
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 */

/**********************************************************************/
#ifndef _MAIN_H_
#define _MAIN_H_

/**********************************************************************/

#include "simlib.h"
#include "simparameters.h"

/**********************************************************************/

typedef double Time;
typedef Fifoqueue_Ptr Buffer_Ptr;

/**********************************************************************/

/* Passed as the event attachment for Packet, ACK, and Timeout events */
typedef struct _packet_ 
{
  long int seq_num;       /* Packet sequence number (or ACKed seq_num) */
  long int rn;            /* Cumulative ACK R_next carried by an ACK */
} Packet, * Packet_Ptr;

typedef struct _simulation_run_data_
{
  /* Protocol & Channel Parameters (allows sweeping in main.c later) */
  int Ws;
  int Wr;
  double pd;
  double pa;
  double timeout_interval;
  int finite_source;
  long int max_packets;

  /* Sender Sliding Window State */
  long int S_base;                  /* Oldest unacknowledged sequence number */
  long int next_seq;                /* Next new sequence number to transmit */
  int tx_busy;                      /* 1 if sender is currently transmitting */
  Buffer_Ptr retx_queue;            /* FIFO queue of sequence numbers to retransmit */

  /* Per-packet tracking arrays (dynamically allocated in main.c) */
  int * acked;                      /* 1 if packet seq_num has been ACKed */
  int * in_retx_queue;              /* 1 if packet seq_num is waiting in retx_queue */
  long int * timer_event_id;        /* simlib event_id of active timeout timer (-1 if none) */

  /* Receiver Sliding Window State */
  long int R_next;                  /* Next expected in-order sequence number (RN) */
  int * recv_buf;                   /* 1 if packet seq_num is buffered at receiver */

  /* Statistics */
  long int blip_counter;
  long int total_tx_attempts;       /* Total data packet transmissions attempted */
  long int number_of_packets_processed; /* Total packets delivered & ACKed (S_base) */
  unsigned random_seed;
} Simulation_Run_Data, * Simulation_Run_Data_Ptr;

/**********************************************************************/

int
main(void);

/**********************************************************************/

#endif /* main.h */