
/*
 * Simulation of the ALOHA Protocol
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

#ifndef _SIMPARAMETERS_H_
#define _SIMPARAMETERS_H_

/*******************************************************************************/

#define MEAN_PACKET_DURATION 1.0   /* 1000 bits / 1 Mbps = 1.0 ms Tx time */
#define PROP_DELAY           5.0   /* 5.0 ms one-way propagation delay (RTT = 10 ms) */
#define TIMEOUT_DELAY        15.0   /* Default 15.0 ms timeout */

/* ARQ Window Sizes:
 * S&W: Ws = 1, Wr = 1
 * GBN: Ws = N, Wr = 1
 * SR:  Ws = N, Wr = M
 */

#define SEND_WINDOW_SIZE     1      /* Ws */
#define RECV_WINDOW_SIZE     1      /* Wr */

/* Channel Error Probabilities */
#define PACKET_ERROR_PROB    0.0    /* pd (set to 0.0 or 0.5 for Exp 1) */
#define ACK_ERROR_PROB       0.0    /* pa (set to 0.0 or 0.5 for Exp 1) */

/* Simulation Run Settings */
#define FINITE_SOURCE        0      /* 0 = continuously backlogged, 1 = finite N packets */
#define RUNLENGTH            10000      /* 5 packets for Exp 1 trace (use 10000 for plots) */
#define BLIPRATE             1000

/* Comma separated list of random seeds to run (put your Student ID first!) */
#define RANDOM_SEED_LIST 1, 2, 3

/*******************************************************************************/

#endif /* simparameters.h */



