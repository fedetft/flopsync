/***************************************************************************
 *   Copyright (C) 2012, 2013 by Terraneo Federico                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef PROTOCOL_CONSTANTS_H
#define	PROTOCOL_CONSTANTS_H

#include "../drivers/timer.h"

// Define this to test the regulator performance with the relative clock model,
// while comment it out to test the absolute clock model
//#define RELATIVE_CLOCK //@@ Filled in by mkpackage.pl

// This is to force a node to attach to the second tier in a multihop
// configuration (legacy, not used)
//#define SECOND_HOP //@@ Filled in by mkpackage.pl

// Defined if we're in a multi hop environment
//#define MULTI_HOP //@@ Filled in by mkpackage.pl

// Disables deep sleep in the root node to make it responsive to commands that
// are sent from the command serial port
//#define INTERACTIVE_ROOTNODE //@@ Filled in by mkpackage.pl

// Enables event timestamping
//#define EVENT_TIMESTAMPING //@@ Filled in by mkpackage.pl

// Enables virtual high resolution timer
//#define USE_VHT //@@ Filled in by mkpackage.pl

//Synchronized nodes send temperature to root node
//#define SENSE_TEMPERATURE //@@ Filled in by mkpackage.pl

//Send timestamps in sync packets, used to make a comparison with existing
//WSN sync schemes.
//#define SEND_TIMESTAMPS //@@ Filled in by mkpackage.pl

// Give a name to the experiment being done
#define experimentName "" //@@ Filled in by mkpackage.pl

#if defined(MULTI_HOP) && defined(SEND_TIMESTAMPS)
#error "Sending timestamps in sync packets with multihop is unimplemented"
#endif

#ifndef USE_VHT
const unsigned int hz=16384;
#else //USE_VHT
const unsigned int hz=24000000;
#endif //USE_VHT

//Sync period
const unsigned int nominalPeriod=static_cast<int>(60*hz+0.5f); //@@ Filled in by mkpackage.pl

//In a real implementation this should need to be computed randomly per node
//(legacy, not used)
#ifndef SECOND_HOP
const unsigned int retransmitPoint=static_cast<int>(0.333f*nominalPeriod+0.5f);
#else //SECOND_HOP
const unsigned int retransmitPoint=static_cast<int>(0.667f*nominalPeriod+0.5f);
#endif //SECOND_HOP



//Time for STM32 PLL startup (500us)
const unsigned int pllBoot=static_cast<int>(0.0005f*hz+0.5f);

//Sync window (fixed window), or maximum sync window (dynamic window)
const unsigned int w=static_cast<int>(0.003f*hz+0.5f);

//Minimum sync window (dynamic window only)
#ifndef USE_VHT
const unsigned int minw=static_cast<int>(0.003f*hz+0.5f);
#else //USE_VHT
const unsigned int minw=static_cast<int>(0.00006f*hz+0.5f);
#endif //USE_VHT

//Retransmit time for flopsync2 "Flooder" flooding scheme (252us)
//Note that this is the time needed to rebroadcast a packet as soon as it's
//received, measured with an oscilloscope
const unsigned int retransmitDelta=static_cast<int>(0.000252f*hz+0.5f);

//Time for cc2520 to start its clock oscillator (200+192us)
const unsigned int radioBoot=static_cast<int>(0.000392f*hz+0.5f);

//Time for the cc2520 to start receiving after a call to startReceiving() (0s)
const unsigned int receiverTurnOn=0;

//Time required to read the timestamp in the packet and overwrite the node's
//hardwar clock (38us), measured with an oscilloscope. Used if SEND_TIMESTAMPS
const unsigned int overwriteClockTime=static_cast<int>(0.000038f*hz+0.5f);

//Time to send a 1..8 byte packet via SPI @ 6MHz to the cc2520 (50us)
//Computed as the missing piece in the difference between frameStart and
//wakeupTime in FlooderRootNode. This is tricky to get right, especially with VHT.
const unsigned int spiPktSend=static_cast<int>(0.000050f*hz+0.5f);

#ifndef USE_VHT
//Additonal delay to absorb jitter (must be greater than pllBoot+radioBoot)
const unsigned int jitterAbsorption=static_cast<int>(0.0015f*hz+0.5f);
#else //USE_VHT
//Additonal delay to absorb jitter (must be greater than pllBoot+radioBoot)
//Also needs to account for vht resynchronization time
const unsigned int jitterAbsorption=static_cast<int>(0.002f*hz+0.5f);
#endif //USE_VHT

//Time to transfer a 4byte of preamble on an 250Kbps channel (384us)
const unsigned int preamblePacketTime=static_cast<int>(0.000384f*hz+0.5f);

//Time to transfer a 4byte packet (+8byte overhead) on an 250Kbps channel (384us)
const unsigned int packetTime=static_cast<int>(0.000384f*hz+0.5f);

//Time to transfer a 1byte packet (+8byte overhead) on an 250Kbps channel (288us)
const unsigned int smallPacketTime=static_cast<int>(0.000288f*hz+0.5f);

//Time to transfer a 8byte packet (+8byte overhead) on an 250Kbps channel (512us)
const unsigned int longPacketTime=static_cast<int>(0.000512f*hz+0.5f);



//Packet containing synchronization quality statistics, to send to base station
// (legacy, not used)
struct Packet
{ 
    short e;
    short u; 
    short w;
    unsigned short misspackets;
    unsigned short unsynctime;
    unsigned short sequence;
    unsigned short check;
};

//New sync quality packet
struct Packet2
{
    short e;
    short u;
    short w;
    unsigned char miss;
    //if check & 0xf0==0x00 miss=1 if packet missed
    //if check & 0xf0==0x10 (miss | (check & 0xf)<<8)=raw temperature
    unsigned char check;
    //unsigned short t;
};

//When to send the synchronization quality statistics packet (legacy, not used)
const unsigned int txtime=static_cast<int>(30*hz+0.5f);

//Comb spacing, for intra-frame error measure
const unsigned int combSpacing=static_cast<int>(0.5f*hz+0.5f);

#endif //PROTOCOL_CONSTANTS_H
