#ifndef _PACKET_TRANSMISSION_H_
#define _PACKET_TRANSMISSION_H_

#include "simlib.h"

void try_send_next_packet(Simulation_Run_Ptr simulation_run);
void end_packet_transmission_event(Simulation_Run_Ptr simulation_run, void *ptr);
void end_packet_reception_event(Simulation_Run_Ptr simulation_run, void *ptr);
void ack_arrival_event(Simulation_Run_Ptr simulation_run, void *ptr);
void timeout_event(Simulation_Run_Ptr simulation_run, void *ptr);

#endif /* _PACKET_TRANSMISSION_H_ */