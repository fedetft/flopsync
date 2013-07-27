/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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
// configuration
//#define SECOND_HOP //@@ Filled in by mkpackage.pl

// Disables deep sleep in the root node to make it responsive to commands that
// are sent from the command serial port
//#define INTERACTIVE_ROOTNODE //@@ Filled in by mkpackage.pl

// Enables event timestamping
//#define EVENT_TIMESTAMPING //@@ Filled in by mkpackage.pl

// Give a name to the experiment being done
#define experimentName "" //@@ Filled in by mkpackage.pl

//Sync period
const unsigned int nominalPeriod=static_cast<int>(60*16384+0.5f); //@@ Filled in by mkpackage.pl

//In a real implementation this should need to be computed randomly per node
#ifndef SECOND_HOP
const unsigned int retransmitPoint=static_cast<int>(0.333f*nominalPeriod+0.5f);
#else //SECOND_HOP
const unsigned int retransmitPoint=static_cast<int>(0.667f*nominalPeriod+0.5f);
#endif //SECOND_HOP

//time for STM32 PLL startup (500us)
const unsigned int pllBoot=static_cast<int>(0.0005f*16384+0.5f);

//Time for nRF to start its clock oscillator (1.6ms)
const unsigned int radioBoot=static_cast<int>(0.0016f*16384+0.5f);

//Time for the nRF to start receiving after a call to startReceiving() (130us)
const unsigned int receiverTurnOn=static_cast<int>(0.00013f*16384+0.5f);

//Additonal delay to absorb jitter (must be greater than pllBoot+radioBoot)
const unsigned int jitterAbsorption=static_cast<int>(0.0025f*16384+0.5f);

//Time to transfer a 4byte packet (+6byte overhead) on an 1MBps channel (80us)
const unsigned int packetTime=static_cast<int>(0.00008f*16384+0.5f);

//Sync window (fixed window), or maximum sync window (dynamic window)
const unsigned int w=static_cast<int>(0.003f*16384+0.5f);

//Minimum sync window (dynamic window only)
const unsigned int minw=static_cast<int>(0.00018f*16384+0.5f);

//Packet containing synchronization quality statistics, to send to base station
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

//When to send the synchronization quality statistics packet
const unsigned int txtime=static_cast<int>(30*16384+0.5f);

#endif //PROTOCOL_CONSTANTS_H
