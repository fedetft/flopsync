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

//Enable synchronization by wire
//#define SYNC_BY_WIRE //@@ Filled in by mkpackage

//Enable comb
//#define COMB //@@ Filled in by mkpackage

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

// Give a hop for this node
const unsigned char node_hop=0; //@@ Filled in by mkpackage.pl

// Define controller
const unsigned char controller=1; //@@ Filled in by mkpackage.pl


//Sync period
const unsigned long long nominalPeriod=static_cast<unsigned long long>(60*hz+0.5f); //@@ Filled in by mkpackage.pl

//Sync window (fixed window), or maximum sync window (dynamic window)
const unsigned long long w=static_cast<unsigned long long>(0.005f*hz+0.5f);

//Minimum sync window (dynamic window only)
#ifndef USE_VHT
const unsigned long long minw=static_cast<unsigned long long>(0.001f*hz+0.5f);
#else //USE_VHT
const unsigned long long minw=static_cast<unsigned long long>(0.00003f*hz+0.5f);
#endif //USE_VHT

//Retransmit time for flopsync2 "Flooder" flooding scheme (252us)
//Note that this is the time needed to rebroadcast a packet as soon as it's
//received, measured with an oscilloscope
//const unsigned long long retransmitDelta=static_cast<unsigned long long>(0.000252f*hz+0.5f);

//Time for STM32 PLL startup (500us)
//const unsigned long long pllBoot=static_cast<unsigned long long>(0.0005f*hz+0.5f);

//Time for cc2520 to start its clock oscillator (1.5ms)
//const unsigned long long radioBoot=static_cast<unsigned long long>(0.001f*hz+0.5f); 

//Transmission of preamble begins 192.3704us after STXON (estimated with frequencymeter) stdev 4.8948652684167 ns

const unsigned long long txTurnaroundTime=static_cast<unsigned long long>(0.0001923704*hz+0.5f); 

//Receiver is ready 192us after RX are enabled but to eliminate jitterSoftware we set it to 250us
const unsigned long long rxTurnaroundTime=static_cast<unsigned long long>(0.000250*hz+0.5f);

//Time required to read the timestamp in the packet and overwrite the node's
//hardware clock (38us), measured with an oscilloscope. Used if SEND_TIMESTAMPS
const unsigned long long overwriteClockTime=static_cast<unsigned long long>(0.000617f*hz+0.5f);


#ifndef USE_VHT
//Additional delay to absorb jitter (must be greater than pllBoot+radioBoot)
const unsigned long long jitterAbsorption=static_cast<unsigned long long>(0.005f*hz+0.5f); 
#else //USE_VHT
//Additional delay to absorb jitter (must be greater than pllBoot+radioBoot)
//Also needs to account for vht resynchronization time
const unsigned long long jitterAbsorption=static_cast<unsigned long long>(0.0015f*hz+0.5f); 
#endif //USE_VHT

//Time measured with oscilloscope between sfd sender and sfd receiver is on 
//average 3.37376us with a standard deviation of 0.0420027 us
const unsigned long long trasmissionTime=static_cast<unsigned long long>(0.00000337376f*hz+0.5f); 

//Time to transfer a 4 preamble + 1 sfd byte on an 250Kbps channel
const unsigned long long preambleFrameTime=static_cast<unsigned long long>((5*8*hz)/channelbps+0.5f); 
 
#ifndef SEND_TIMESTAMPS
//Time to transfer a 2byte of payload on an 250Kbps channel (1byte of LEN 1 payload)
const unsigned long long payloadFrameTime=static_cast<unsigned long long>((2*8*hz)/channelbps+0.5f);
//Time to transfer a 1byte of fcs on an 250Kbps channel 
const unsigned long long fcsFrameTime=static_cast<unsigned long long>((1*8*hz)/channelbps+0.5f);
//Time to transfer piggybacking
const unsigned long long piggybackingTime=static_cast<unsigned long long>((0*8*hz)/channelbps+0.5f);
//Time to transfer full packet
const unsigned long long frameTime=static_cast<unsigned long long>((8*8*hz)/channelbps+0.5f);
#else //SEND_TIMESTAMPS
//Time to transfer a 9byte of payload on an 250Kbps channel (1byte of LEN 8 payload)
const unsigned long long payloadFrameTime=static_cast<unsigned long long>((9*8*hz)/channelbps+0.5f);
//Time to transfer a 2byte of fcs on an 250Kbps channel 
const unsigned long long fcsFrameTime=static_cast<unsigned long long>((2*8*hz)/channelbps+0.5f);
//Time to transfer piggybacking
const unsigned long long piggybackingTime=static_cast<unsigned long long>((0*8*hz)/channelbps+0.5f);
//Time to transfer full packet
const unsigned long long frameTime=static_cast<unsigned long long>((16*8*hz)/channelbps+0.5f);
#endif//SEND_TIMESTAMPS

//Time to wait before forwarding the packet
const unsigned long long delayRebroadcastTime=static_cast<unsigned long long>(0.0005f*hz+0.5f); 

//Waiting time over the reception of the nominal time of packet.
//Aka slack time, we wait for a little longer than the nominal time to avoid spurous timeouts
const unsigned long long delaySendPacketTime=static_cast<unsigned long long>(0.0001f*hz+0.5f);



//Payload bytes' number in RTT request packet
const unsigned long long rttPayloadBytes=2;

//Time required to send RTT request packet's payload
const unsigned long long rttTailPacketTime=(rttPayloadBytes*8*hz)/channelbps;

//time required to send a complete packet (pre + sfd + payload)
const unsigned long long rttPacketTime=preambleFrameTime+rttTailPacketTime;

//Time gap used for reply packet timeout
const unsigned long long rttSlackTime=static_cast<unsigned long long>(0.0002f*hz+0.5f);

//Time gap between SFD request packet and reply packet sending (tail packet + slack time)
const unsigned long long rttRetransmitTime=rttTailPacketTime+txTurnaroundTime+rttSlackTime;

//Payload bytes' number in RTT response packet
const unsigned long long rttResponsePayloadBytes=16;

//Time required to send RTT response packet's payload
const unsigned long long rttResponseTailPacketTime=static_cast<unsigned long long>((rttResponsePayloadBytes*8*hz)/channelbps+0.5f);





//New sync quality packet
struct Packet
{
    int e;
    int u;
    int w;
    unsigned char miss;
    //if check & 0xf0==0x00 miss=1 if packet missed
    //if check & 0xf0==0x10 (miss | (check & 0xf)<<8)=raw temperature
    unsigned char check;
};

const unsigned long long packetTime=static_cast<unsigned long long>(((sizeof(Packet)+8)*8*hz)/channelbps+0.5f); //TODO: why "+8" instead of "+7"?

//Comb spacing, for intra-frame error measure - 500ms gap width
// const unsigned long long combSpacing=static_cast<unsigned long long>(0.5f*hz+0.5f);

//Comb spacing, for intra-frame error measure - 100ms gap width
const unsigned long long combSpacing=static_cast<unsigned long long>(0.1f*hz+0.5f);

//Time slot width used for RTT measure - 1 ms gap width
const unsigned long long rttSpacing=static_cast<unsigned long long>(0.01f*hz+0.5f); //TODO: set the correct value, 1ms is dummy!

#endif //PROTOCOL_CONSTANTS_H
