
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
#include <math.h>
#include "trace.h"
#include "output.h"
#include "channel.h"
#include "packet_transmission.h"

/*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "trace.h"
#include "output.h"
#include "packet_duration.h"
#include "packet_transmission.h"

/*******************************************************************************/
/* Helper: Start transmitting a retransmission or new packet if transmitter idle */
/*******************************************************************************/

void
try_send_next_packet(Simulation_Run_Ptr simulation_run)
{
  Simulation_Run_Data_Ptr data;
  Packet_Ptr tx_pkt, tmr_pkt, q_pkt;
  long int seq_to_send = -1;
  int is_retx = 0;
  Time now;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  now = simulation_run_get_time(simulation_run);

  if (data->tx_busy) {
    return;
  }

  /* 1. Clean out any already-ACKed packets sitting at front of retx_queue */
  while (fifoqueue_size(data->retx_queue) > 0) {
    q_pkt = (Packet_Ptr) fifoqueue_see_front(data->retx_queue);
    if (data->acked[q_pkt->seq_num] || q_pkt->seq_num < data->S_base) {
      q_pkt = (Packet_Ptr) fifoqueue_get(data->retx_queue);
      data->in_retx_queue[q_pkt->seq_num] = 0;
      xfree(q_pkt);
    } else {
      break;
    }
  }

  /* 2. Priority 1: Retransmit timed-out packet from retx_queue */
  if (fifoqueue_size(data->retx_queue) > 0) {
    q_pkt = (Packet_Ptr) fifoqueue_get(data->retx_queue);
    seq_to_send = q_pkt->seq_num;
    data->in_retx_queue[seq_to_send] = 0;
    xfree(q_pkt);
    is_retx = 1;
  }
  /* 3. Priority 2: Send new packet if within send window [S_base, S_base + Ws - 1] */
  else if (data->next_seq < data->S_base + data->Ws) {
    if (!data->finite_source || data->next_seq < data->max_packets) {
      seq_to_send = data->next_seq;
      data->next_seq++;
      is_retx = 0;
    }
  }

  if (seq_to_send != -1) {
    data->tx_busy = 1;
    data->total_tx_attempts++;

    TRACE(printf("  [t=%.1f ms] SENDER: Start %s Pkt %ld (Win=[%ld, %ld])\n",
                 now, is_retx ? "RETX" : "TX", seq_to_send,
                 data->S_base, data->S_base + data->Ws - 1););

    /* Schedule End of Packet Transmission (now + 1.0 ms) */
    tx_pkt = (Packet_Ptr) xmalloc(sizeof(Packet));
    tx_pkt->seq_num = seq_to_send;
    tx_pkt->rn = 0;
    schedule_transmission_end_event(simulation_run,
                                    now + get_packet_duration(),
                                    (void *) tx_pkt);

    /* Cancel any previous timer for this seq_num before setting a new one */
    if (data->timer_event_id[seq_to_send] != -1) {
      void *old_att = simulation_run_deschedule_event(simulation_run,
                                                      data->timer_event_id[seq_to_send]);
      if (old_att != NULL) xfree(old_att);
      data->timer_event_id[seq_to_send] = -1;
    }

    /* Schedule Timeout Event (now + Tout) and save its event_id */
    tmr_pkt = (Packet_Ptr) xmalloc(sizeof(Packet));
    tmr_pkt->seq_num = seq_to_send;
    tmr_pkt->rn = 0;
    data->timer_event_id[seq_to_send] = schedule_timeout_event(
        simulation_run, now + data->timeout_interval, (void *) tmr_pkt);
  }
}

/*******************************************************************************/
/* Event 1: End of Packet Transmission at Source                               */
/*******************************************************************************/

long int
schedule_transmission_end_event(Simulation_Run_Ptr simulation_run,
                                Time event_time, void * packet)
{
  Event event;
  event.description = "End of Packet Tx";
  event.function = transmission_end_event;
  event.attachment = packet;
  return simulation_run_schedule_event(simulation_run, event, event_time);
}

void
transmission_end_event(Simulation_Run_Ptr simulation_run, void * ptr)
{
  Simulation_Run_Data_Ptr data;
  Packet_Ptr this_packet;
  Time now;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  now = simulation_run_get_time(simulation_run);
  this_packet = (Packet_Ptr) ptr;

  /* Transmitter is now free */
  data->tx_busy = 0;

  /* Check if data packet experiences channel error with probability pd */
  if (uniform_generator() >= data->pd) {
    schedule_packet_arrival_at_dest_event(simulation_run,
                                          now + PROP_DELAY,
                                          (void *) this_packet);
  } else {
    TRACE(printf("  [t=%.1f ms] CHANNEL: Data Pkt %ld lost/corrupted (pd)\n",
                 now, this_packet->seq_num););
    xfree(this_packet);
  }

  /* Immediately start next packet transmission if eligible */
  try_send_next_packet(simulation_run);
}

/*******************************************************************************/
/* Event 2: Data Packet Arrival at Destination                                 */
/*******************************************************************************/

long int
schedule_packet_arrival_at_dest_event(Simulation_Run_Ptr simulation_run,
                                      Time event_time, void * packet)
{
  Event event;
  event.description = "Pkt Arrival at Dest";
  event.function = packet_arrival_at_dest_event;
  event.attachment = packet;
  return simulation_run_schedule_event(simulation_run, event, event_time);
}

void
packet_arrival_at_dest_event(Simulation_Run_Ptr simulation_run, void * ptr)
{
  Simulation_Run_Data_Ptr data;
  Packet_Ptr rx_pkt, ack_pkt;
  long int seq;
  int accepted = 0;
  Time now;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  now = simulation_run_get_time(simulation_run);
  rx_pkt = (Packet_Ptr) ptr;
  seq = rx_pkt->seq_num;

  TRACE(printf("  [t=%.1f ms] RECEIVER: Pkt %ld arrived (R_next=%ld, Win=[%ld, %ld])\n",
               now, seq, data->R_next, data->R_next, data->R_next + data->Wr - 1););

  /* Check if packet falls within receive window [R_next, R_next + Wr - 1] */
  if (seq < data->R_next) {
    /* Already delivered duplicate packet; re-ACK it */
    accepted = 1;
  } else if (seq >= data->R_next && seq < data->R_next + data->Wr) {
    accepted = 1;
    data->recv_buf[seq] = 1;
    /* Slide receive window R_next forward for contiguous in-order packets */
    while (data->recv_buf[data->R_next]) {
      data->recv_buf[data->R_next] = 0;
      data->R_next++;
    }
  }

  /* Generate ACK for successfully received data packet (lost with prob pa) */
  if (uniform_generator() >= data->pa) {
    ack_pkt = (Packet_Ptr) xmalloc(sizeof(Packet));
    ack_pkt->seq_num = accepted ? seq : -1;
    ack_pkt->rn = data->R_next;
    schedule_ack_arrival_at_src_event(simulation_run,
                                      now + PROP_DELAY,
                                      (void *) ack_pkt);
  } else {
    TRACE(printf("  [t=%.1f ms] CHANNEL: ACK for Pkt %ld (RN=%ld) lost (pa)\n",
                 now, seq, data->R_next););
  }

  xfree(rx_pkt);
}

/*******************************************************************************/
/* Event 3: ACK Arrival at Source                                              */
/*******************************************************************************/

long int
schedule_ack_arrival_at_src_event(Simulation_Run_Ptr simulation_run,
                                  Time event_time, void * ack_ptr)
{
  Event event;
  event.description = "ACK Arrival at Src";
  event.function = ack_arrival_at_src_event;
  event.attachment = ack_ptr;
  return simulation_run_schedule_event(simulation_run, event, event_time);
}

void
ack_arrival_at_src_event(Simulation_Run_Ptr simulation_run, void * ptr)
{
  Simulation_Run_Data_Ptr data;
  Packet_Ptr ack_pkt;
  long int p, ack_seq, rn;
  Time now;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  now = simulation_run_get_time(simulation_run);
  ack_pkt = (Packet_Ptr) ptr;
  ack_seq = ack_pkt->seq_num;
  rn = ack_pkt->rn;

  TRACE(printf("  [t=%.1f ms] SENDER: ACK arrived (ack_seq=%ld, RN=%ld)\n",
               now, ack_seq, rn););

  /* 1. Cumulative ACK (RN) acknowledges all packets < RN */
  if (rn > data->S_base) {
    for (p = data->S_base; p < rn; p++) {
      data->acked[p] = 1;
      if (data->timer_event_id[p] != -1) {
        void *old_att = simulation_run_deschedule_event(simulation_run,
                                                        data->timer_event_id[p]);
        if (old_att != NULL) xfree(old_att);
        data->timer_event_id[p] = -1;
      }
    }
    data->S_base = rn;
    while (data->acked[data->S_base]) {
      data->S_base++;
    }
  }

  /* 2. For SR (Wr > 1), selective ACK acknowledges individual buffered packet */
  if (data->Wr > 1 && ack_seq >= data->S_base) {
    data->acked[ack_seq] = 1;
    if (data->timer_event_id[ack_seq] != -1) {
      void *old_att = simulation_run_deschedule_event(simulation_run,
                                                      data->timer_event_id[ack_seq]);
      if (old_att != NULL) xfree(old_att);
      data->timer_event_id[ack_seq] = -1;
    }
    while (data->acked[data->S_base]) {
      data->S_base++;
    }
  }

  data->number_of_packets_processed = data->S_base;
  output_blip_to_screen(simulation_run);

  xfree(ack_pkt);

  if (data->number_of_packets_processed < data->max_packets) {
    try_send_next_packet(simulation_run);
  }
}

/*******************************************************************************/
/* Event 4: Retransmission Timeout at Source                                   */
/*******************************************************************************/

long int
schedule_timeout_event(Simulation_Run_Ptr simulation_run,
                       Time event_time, void * pkt_ptr)
{
  Event event;
  event.description = "Packet Timeout";
  event.function = timeout_event;
  event.attachment = pkt_ptr;
  return simulation_run_schedule_event(simulation_run, event, event_time);
}

void
timeout_event(Simulation_Run_Ptr simulation_run, void * ptr)
{
  Simulation_Run_Data_Ptr data;
  Packet_Ptr tmr_pkt, q_pkt;
  long int seq, p;
  Time now;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  now = simulation_run_get_time(simulation_run);
  tmr_pkt = (Packet_Ptr) ptr;
  seq = tmr_pkt->seq_num;

  /* Timer has now fired, so clear its ID */
  data->timer_event_id[seq] = -1;

  if (!data->acked[seq]) {
    TRACE(printf("  [t=%.1f ms] SENDER: TIMEOUT for Pkt %ld!\n", now, seq););

    if (data->Wr == 1) {
      /* GBN / S&W: Queue all unACKed packets from S_base to next_seq - 1 */
      for (p = data->S_base; p < data->next_seq; p++) {
        if (!data->acked[p] && !data->in_retx_queue[p]) {
          q_pkt = (Packet_Ptr) xmalloc(sizeof(Packet));
          q_pkt->seq_num = p;
          q_pkt->rn = 0;
          fifoqueue_put(data->retx_queue, (void *) q_pkt);
          data->in_retx_queue[p] = 1;

          /* Deschedule pending timers for subsequent packets in window */
          if (data->timer_event_id[p] != -1) {
            void *old_att = simulation_run_deschedule_event(simulation_run,
                                                            data->timer_event_id[p]);
            if (old_att != NULL) xfree(old_att);
            data->timer_event_id[p] = -1;
          }
        }
      }
    } else {
      /* Selective Repeat: Retransmit only the timed-out packet */
      if (!data->in_retx_queue[seq]) {
        q_pkt = (Packet_Ptr) xmalloc(sizeof(Packet));
        q_pkt->seq_num = seq;
        q_pkt->rn = 0;
        fifoqueue_put(data->retx_queue, (void *) q_pkt);
        data->in_retx_queue[seq] = 1;
      }
    }

    try_send_next_packet(simulation_run);
  }

  xfree(tmr_pkt);
}