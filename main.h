#ifndef _MAIN_H_
#define _MAIN_H_

#include "simlib.h"
#include "simparameters.h"

typedef struct _packet_ {
  long int seq;
  long int ack_seq;
  long int R_next;
  double service_time;
} Packet, *Packet_Ptr;

typedef struct _simulation_run_data_ {
  /* Configuration Parameters */
  int Ws;
  int Wr;
  double pd;
  double pa;
  double timeout_interval;
  int finite_source;
  long int max_packets;
  unsigned random_seed;

  /* Sender & Receiver State */
  long int S_base;
  long int next_seq;
  int tx_busy;
  long int R_next;
  long int blip_counter;
  long int total_tx_attempts;
  long int number_of_packets_processed;
  Fifoqueue_Ptr retx_queue;

  /* Dynamic Tracking Arrays */
  int *acked;
  int *in_retx_queue;
  long int *timer_event_id;
  int *recv_buf;

  /* Diagnostic / Verification Counters */
  long int pkt_drops;
  long int ack_drops;
  long int timeouts;
  long int retx_count;
  long int ooo_rejected;
  long int ooo_buffered;
} Simulation_Run_Data, *Simulation_Run_Data_Ptr;

#endif /* _MAIN_H_ */