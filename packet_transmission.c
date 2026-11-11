#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "simlib.h"
#include "simparameters.h"
#include "packet_duration.h"
#include "output.h"
#include "trace.h"
#include "main.h"
#include "packet_transmission.h"

#ifndef PROPAGATION_DELAY
#define PROPAGATION_DELAY 5.0
#endif

#ifndef ACK_DURATION
#define ACK_DURATION 0.0
#endif

/* Set to 1 if you want step-by-step console event traces printed */
#ifndef ENABLE_CONSOLE_TRACE
#define ENABLE_CONSOLE_TRACE 0
#endif

static void
trace_msg(Simulation_Run_Ptr sim_run, const char *fmt, ...)
{
#if ENABLE_CONSOLE_TRACE
  va_list args;
  printf("[t=%6.2f ms] ", simulation_run_get_time(sim_run));
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
#else
  (void) sim_run;
  (void) fmt;
#endif
}

/* =========================================================================
 * Helper to schedule any event in simlib
 * ========================================================================= */
static long int
schedule_sim_event(Simulation_Run_Ptr sim_run,
                   const char *desc,
                   void (*func)(Simulation_Run_Ptr, void *),
                   void *arg,
                   double event_time)
{
  Event event;
  event.description = desc;
  event.function = func;
  event.attachment = arg;
  return simulation_run_schedule_event(sim_run, event, event_time);
}

/* =========================================================================
 * Attempt to transmit the next queued retransmission or new packet in window
 * ========================================================================= */
void
try_send_next_packet(Simulation_Run_Ptr simulation_run)
{
  Simulation_Run_Data_Ptr data;
  long int seq;
  double now, tx_duration;
  Packet_Ptr pkt;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);

  if (data->tx_busy || data->number_of_packets_processed >= data->max_packets) {
    return;
  }

  /* Priority 1: Retransmission queue */
  if (fifoqueue_size(data->retx_queue) > 0) {
    seq = (long int) fifoqueue_get(data->retx_queue);
    data->in_retx_queue[seq] = 0;
    data->retx_count++; /* Count actual retransmission */
  }
  /* Priority 2: New packet within the Send Window [S_base, S_base + Ws - 1] */
  else if (data->next_seq < data->S_base + data->Ws) {
    if (data->finite_source && data->next_seq >= data->max_packets) {
      return;
    }
    seq = data->next_seq++;
  } else {
    return; /* Send window is full; wait for ACK */
  }

  now = simulation_run_get_time(simulation_run);
  tx_duration = get_packet_duration();

  data->tx_busy = 1;
  data->total_tx_attempts++;

  /* Allocate packet descriptor for transmission event */
  pkt = (Packet_Ptr) xmalloc(sizeof(Packet));
  pkt->seq = seq;
  pkt->ack_seq = -1;
  pkt->R_next = -1;
  pkt->service_time = tx_duration;

  /* Schedule end of packet transmission on link */
  schedule_sim_event(simulation_run, "End Packet TX",
                     end_packet_transmission_event, (void *) pkt,
                     now + tx_duration);

  /* Cancel existing timer if active, then schedule fresh timeout */
  if (data->timer_event_id[seq] != -1) {
    simulation_run_deschedule_event(simulation_run, data->timer_event_id[seq]);
    data->timer_event_id[seq] = -1;
  }

  data->timer_event_id[seq] = schedule_sim_event(
    simulation_run, "Packet Timeout",
    timeout_event, (void *) seq,
    now + data->timeout_interval
  );

  trace_msg(simulation_run, "[TX START] Pkt seq=%ld (S_base=%ld, next_seq=%ld)\n",
            seq, data->S_base, data->next_seq);
}

/* =========================================================================
 * Event: Packet finishes serializing onto the link
 * ========================================================================= */
void
end_packet_transmission_event(Simulation_Run_Ptr simulation_run, void *ptr)
{
  Simulation_Run_Data_Ptr data;
  Packet_Ptr pkt;
  double now;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  pkt = (Packet_Ptr) ptr;
  now = simulation_run_get_time(simulation_run);

  data->tx_busy = 0;

  /* Roll channel error probability pd */
  if (uniform_generator() >= data->pd) {
    schedule_sim_event(simulation_run, "Packet Arrival at RX",
                       end_packet_reception_event, (void *) pkt,
                       now + PROPAGATION_DELAY);
    trace_msg(simulation_run, "[TX DONE] Pkt seq=%ld propagating to receiver\n", pkt->seq);
  } else {
    data->pkt_drops++; /* Count dropped data packet */
    trace_msg(simulation_run, "[DROP DATA] Pkt seq=%ld corrupted on channel\n", pkt->seq);
    xfree(pkt);
  }

  /* Pipeline next packet immediately if window allows */
  try_send_next_packet(simulation_run);
}

/* =========================================================================
 * Event: Data packet arrives at the receiver
 * ========================================================================= */
void
end_packet_reception_event(Simulation_Run_Ptr simulation_run, void *ptr)
{
  Simulation_Run_Data_Ptr data;
  Packet_Ptr pkt, ack_pkt;
  long int seq, ack_seq;
  double now;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  pkt = (Packet_Ptr) ptr;
  seq = pkt->seq;
  now = simulation_run_get_time(simulation_run);

  xfree(pkt);

  /* Check if packet falls within Receive Window [R_next, R_next + Wr - 1] */
  if (seq >= data->R_next && seq < data->R_next + data->Wr) {
    if (seq > data->R_next && data->recv_buf[seq] == 0) {
      data->ooo_buffered++; /* Out-of-order frame buffered in Selective Repeat */
      trace_msg(simulation_run, "[RX BUFFERED] Out-of-order Pkt seq=%ld buffered (R_next=%ld)\n",
                seq, data->R_next);
    }

    data->recv_buf[seq] = 1;
    ack_seq = seq;

    /* Advance R_next across all contiguous buffered packets */
    while (data->recv_buf[data->R_next] == 1) {
      data->R_next++;
    }
    trace_msg(simulation_run, "[RX ACCEPT] Pkt seq=%ld accepted, new R_next=%ld\n",
              seq, data->R_next);
  } else {
    if (seq >= data->R_next + data->Wr) {
      data->ooo_rejected++; /* Out-of-order frame discarded (e.g., GBN Wr=1) */
      trace_msg(simulation_run, "[RX REJECT] Out-of-order Pkt seq=%ld discarded (R_next=%ld, Wr=%d)\n",
                seq, data->R_next, data->Wr);
    } else {
      trace_msg(simulation_run, "[RX DUPLICATE] Duplicate Pkt seq=%ld ignored (R_next=%ld)\n",
                seq, data->R_next);
    }
    ack_seq = -1;
  }

  /* Send ACK (contains cumulative R_next, plus selective seq if accepted) */
  if (uniform_generator() >= data->pa) {
    ack_pkt = (Packet_Ptr) xmalloc(sizeof(Packet));
    ack_pkt->seq = -1;
    ack_pkt->ack_seq = ack_seq;
    ack_pkt->R_next = data->R_next;
    ack_pkt->service_time = ACK_DURATION;

    schedule_sim_event(simulation_run, "ACK Arrival at Sender",
                       ack_arrival_event, (void *) ack_pkt,
                       now + ACK_DURATION + PROPAGATION_DELAY);
  } else {
    data->ack_drops++; /* Count dropped ACK */
    trace_msg(simulation_run, "[DROP ACK] ACK (R_next=%ld, ack_seq=%ld) lost on channel\n",
              data->R_next, ack_seq);
  }
}

/* =========================================================================
 * Event: ACK arrives back at the sender
 * ========================================================================= */
void
ack_arrival_event(Simulation_Run_Ptr simulation_run, void *ptr)
{
  Simulation_Run_Data_Ptr data;
  Packet_Ptr ack_pkt;
  long int sel_ack, cum_ack;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  ack_pkt = (Packet_Ptr) ptr;
  sel_ack = ack_pkt->ack_seq;
  cum_ack = ack_pkt->R_next;

  xfree(ack_pkt);

  /* 1. Mark selective ACK if present */
  if (sel_ack != -1 && !data->acked[sel_ack]) {
    data->acked[sel_ack] = 1;
    if (data->timer_event_id[sel_ack] != -1) {
      simulation_run_deschedule_event(simulation_run, data->timer_event_id[sel_ack]);
      data->timer_event_id[sel_ack] = -1;
    }
  }

  /* 2. Slide S_base up to cumulative R_next */
  while (data->S_base < cum_ack) {
    if (!data->acked[data->S_base]) {
      data->acked[data->S_base] = 1;
    }
    if (data->timer_event_id[data->S_base] != -1) {
      simulation_run_deschedule_event(simulation_run, data->timer_event_id[data->S_base]);
      data->timer_event_id[data->S_base] = -1;
    }
    data->S_base++;
    data->number_of_packets_processed++;
    output_blip_to_screen(simulation_run);
  }

  trace_msg(simulation_run, "[ACK ARRIVE] cum_ack=%ld, sel_ack=%ld -> S_base=%ld\n",
            cum_ack, sel_ack, data->S_base);

  try_send_next_packet(simulation_run);
}

/* =========================================================================
 * Event: Retransmission timer expires for packet seq
 * ========================================================================= */
void
timeout_event(Simulation_Run_Ptr simulation_run, void *ptr)
{
  Simulation_Run_Data_Ptr data;
  long int seq, p;

  data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
  seq = (long int) ptr;

  data->timer_event_id[seq] = -1;

  if (data->acked[seq]) {
    return; /* Packet already acknowledged */
  }

  data->timeouts++; /* Count valid timeout */

  if (data->Wr == 1) {
    /* Stop-and-Wait & Go-Back-N: Roll back and re-queue all unACKed packets */
    trace_msg(simulation_run, "[TIMEOUT GBN/SW] Pkt seq=%ld timed out; rolling back window [%ld..%ld]\n",
              seq, data->S_base, data->next_seq - 1);

    for (p = data->S_base; p < data->next_seq; p++) {
      if (!data->acked[p] && !data->in_retx_queue[p]) {
        fifoqueue_put(data->retx_queue, (void *) p);
        data->in_retx_queue[p] = 1;
      }
      if (p != seq && data->timer_event_id[p] != -1) {
        simulation_run_deschedule_event(simulation_run, data->timer_event_id[p]);
        data->timer_event_id[p] = -1;
      }
    }
  } else {
    /* Selective Repeat (Wr > 1): Surgically re-queue ONLY the timed-out packet */
    trace_msg(simulation_run, "[TIMEOUT SR] Pkt seq=%ld timed out; queuing selective retransmission\n",
              seq);

    if (!data->acked[seq] && !data->in_retx_queue[seq]) {
      fifoqueue_put(data->retx_queue, (void *) seq);
      data->in_retx_queue[seq] = 1;
    }
  }

  try_send_next_packet(simulation_run);
}