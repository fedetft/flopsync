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

//Enable glossy flooding
//#define GLOSSY //@@ Filled in by mkpackage.pl

///This is for enable debug flopsync
#define FLOPSYNC_DEBUG  1
#include <cstdio>

// Give a name to the experiment being done
#define experimentName "" //@@ Filled in by mkpackage.pl

#if defined(MULTI_HOP) && defined(SEND_TIMESTAMPS)
#error "Sending timestamps in sync packets with multihop is unimplemented"
#endif

//Channel bandwidth 250 Kbps
const unsigned long long channelbps=250000;

//Channel bandwidth 6 Mbps
const unsigned long long spibps=6000000;

#ifndef USE_VHT
const unsigned long long hz=16384;
#else //USE_VHT
const unsigned long long hz=24000000;
#endif //USE_VHT

//Sync period
const unsigned long long nominalPeriod=static_cast<unsigned long long>(60*hz+0.5f); //@@ Filled in by mkpackage.pl

//In a real implementation this should need to be computed randomly per node
//(legacy, not used)
#ifndef SECOND_HOP
const unsigned long long retransmitPoint=static_cast<unsigned long long>(0.333f*nominalPeriod+0.5f);
#else //SECOND_HOP
const unsigned long long retransmitPoint=static_cast<unsigned long long>(0.667f*nominalPeriod+0.5f);
#endif //SECOND_HOP


//Sync window (fixed window), or maximum sync window (dynamic window)
const unsigned long long w=static_cast<unsigned long long>(0.003f*hz+0.5f);

//Minimum sync window (dynamic window only)
#ifndef USE_VHT
const unsigned long long minw=static_cast<unsigned long long>(0.003f*hz+0.5f);
#else //USE_VHT
const unsigned long long minw=static_cast<unsigned long long>(0.00006f*hz+0.5f);
#endif //USE_VHT

//Retransmit time for flopsync2 "Flooder" flooding scheme (252us)
//Note that this is the time needed to rebroadcast a packet as soon as it's
//received, measured with an oscilloscope
const unsigned long long retransmitDelta=static_cast<unsigned long long>(0.000252f*hz+0.5f);

//Time for STM32 PLL startup (500us)
//const unsigned long long pllBoot=static_cast<unsigned long long>(0.0005f*hz+0.5f);

//Time for cc2520 to start its clock oscillator (1.5ms)
//const unsigned long long radioBoot=static_cast<unsigned long long>(0.001f*hz+0.5f); 

//Transmission of preamble begins 192us after STXON
const unsigned long long txTurnaroundTime=static_cast<unsigned long long>(0.000192*hz+0.5f); 

//Receiver is ready 192us after RX are enabled
const unsigned long long rxTurnaroundTime=static_cast<unsigned long long>(0.000192*hz+0.5f);

//Time required to read the timestamp in the packet and overwrite the node's
//hardware clock (38us), measured with an oscilloscope. Used if SEND_TIMESTAMPS
const unsigned long long overwriteClockTime=static_cast<unsigned long long>(0.000038f*hz+0.5f);



//Time to send a 1..8 byte packet via SPI @ 6MHz to the cc2520 (50us)
//Computed as the missing piece in the difference between frameStart and
//wakeupTime in FlooderRootNode. This is tricky to get right, especially with VHT.
//const unsigned long long spiPktSend=static_cast<unsigned long long>(0.000050f*hz+0.5f);

#ifndef USE_VHT
//Additional delay to absorb jitter (must be greater than pllBoot+radioBoot)
const unsigned long long jitterAbsorption=static_cast<unsigned long long>(0.005f*hz+0.5f); //FIXME
#else //USE_VHT
//Additional delay to absorb jitter (must be greater than pllBoot+radioBoot)
//Also needs to account for vht resynchronization time
const unsigned long long jitterAbsorption=static_cast<unsigned long long>(0.0015f*hz+0.5f);  //FIXME
#endif //USE_VHT

//Additional delay to absorb jitter software (command transfer to spi + time to process instruction)
//const unsigned long long jitterSWAbsorption=static_cast<unsigned long long>(0.005f*hz+0.5f); //FIXME

//Time to transfer a 4 preamble + 1 sfd byte on an 250Kbps channel
const unsigned long long preamblePacketTime=static_cast<unsigned long long>(5*8/channelbps*hz+0.5f); 
 
#ifndef SEND_TIMESTAMPS
//Time to transfer a 1byte len 1byte payload 1byte fcs on an 6Mbps channel (1byte of LEN 1 payload)
const unsigned long long spiTime=static_cast<unsigned long long>(3*8/spibps*hz+0.5f);
//Time to transfer a 2byte of payload on an 250Kbps channel (1byte of LEN 1 payload)
const unsigned long long payloadPacketTime=static_cast<unsigned long long>(2*8/channelbps*hz+0.5f);
//Time to transfer a 1byte of fcs on an 250Kbps channel 
const unsigned long long fcsPacketTime=static_cast<unsigned long long>(1*8/channelbps*hz+0.5f);
//Time to transfer piggybacking
const unsigned long long piggybackingTime=static_cast<unsigned long long>(0*8/channelbps*hz+0.5f);
//Time to transfer full packet
const unsigned long long packetTime=static_cast<unsigned long long>(8*8/channelbps*hz+0.5f);
#else //SEND_TIMESTAMPS
//Time to transfer a 1byte len 8byte payload on an 6Mbps channel (1byte of LEN 1 payload)
const unsigned long long spiTime=static_cast<unsigned long long>(9*8/spibps*hz+0.5f);
//Time to transfer a 9byte of payload on an 250Kbps channel (1byte of LEN 8 payload)
const unsigned long long payloadPacketTime=static_cast<unsigned long long>(9*8/channelbps*hz+0.5f);
//Time to transfer a 2byte of fcs on an 250Kbps channel 
const unsigned long long fcsPacketTime=static_cast<unsigned long long>(2*8/channelbps*hz+0.5f);
//Time to transfer piggybacking
const unsigned long long piggybackingTime=static_cast<unsigned long long>(0*8/channelbps*hz+0.5f);
//Time to transfer full packet
const unsigned long long packetTime=static_cast<unsigned long long>(16*8/channelbps*hz+0.5f);
#endif//SEND_TIMESTAMPS

//Time to wait before forwarding the packet
const unsigned long long delayRebroadcastTime=static_cast<unsigned long long>(0.0005f*hz+0.5f); //FIXME

//Waiting time over the reception of the nominal time of packet
const unsigned long long delaySendPacketTime=static_cast<unsigned long long>(0.00003f*hz+0.5f);  //FIXME


//New sync quality packet
struct Packet
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

//Comb spacing, for intra-frame error measure
const unsigned long long combSpacing=static_cast<unsigned long long>(0.5f*hz+0.5f);

#endif //PROTOCOL_CONSTANTS_H
