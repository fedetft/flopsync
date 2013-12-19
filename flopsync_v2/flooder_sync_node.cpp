 
/***************************************************************************
 *   Copyright (C)  2013 by Terraneo Federico                              *
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
#include "flooder_sync_node.h"
#include <cassert>
#include "critical_section.h"
#include <cstdio>
using namespace std;

//
// class FlooderSyncNode
//

FlooderSyncNode::FlooderSyncNode(Timer& rtc, Synchronizer& synchronizer,
        unsigned char hop) : rtc(rtc), tr(Transceiver::instance()),
        timer(AuxiliaryTimer::instance()), synchronizer(synchronizer),
        measuredFrameStart(0), computedFrameStart(0), clockCorrection(0),
        receiverWindow(w), missPackets(maxMissPackets+1), hop(hop) {}
        
bool FlooderSyncNode::synchronize()
{
    if(missPackets>maxMissPackets) return true;
    
    #ifndef SEND_TIMESTAMPS
    computedFrameStart+=nominalPeriod+clockCorrection;
    #else //SEND_TIMESTAMPS
    if(synchronizer.overwritesHardwareClock())
        computedFrameStart=getRadioTimestamp()+nominalPeriod+clockCorrection;
    else computedFrameStart+=nominalPeriod+clockCorrection;
    #endif //SEND_TIMESTAMPS
    unsigned int wakeupTime=computedFrameStart-
        (jitterAbsorption+receiverTurnOn+receiverWindow+smallPacketTime);
    assert(static_cast<int>(rtc.getValue()-wakeupTime)<0);
    rtc.setAbsoluteWakeupSleep(wakeupTime);
    rtc.sleep();
    rtc.setAbsoluteWakeupWait(wakeupTime+jitterAbsorption);
    tr.setMode(Transceiver::RX);
    #ifndef SEND_TIMESTAMPS
    tr.setPacketLength(1);
    #else //SEND_TIMESTAMPS
    tr.setPacketLength(4);
    #endif //SEND_TIMESTAMPS
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    rtc.wait();
    miosix::ledOn();
    tr.startReceiving();
    timer.initTimeoutTimer(toAuxiliaryTimer(
        receiverTurnOn+2*receiverWindow+smallPacketTime));
    bool timeout;
    {
        CriticalSection cs;
        for(;;)
        {
            timeout=timer.waitForPacketOrTimeout(); //FIXME
            //while(tr.irq()==false) ;
            measuredFrameStart=rtc.getPacketTimestamp();
            // Falling edge signals RX received the packet.
            // It happens between 4 and 16us from the transmitter's falling edge
            // (measured with an oscilloscope)
            miosix::ledOff();
            if(timeout) break;
            #ifndef SEND_TIMESTAMPS
            char packet[1];
            #else //SEND_TIMESTAMPS
            unsigned int *packet=&receivedTimestamp;
            #endif //SEND_TIMESTAMPS
            tr.readPacket(packet);
            #ifndef SEND_TIMESTAMPS
            if(packet[0]==hop) break;
            #else //SEND_TIMESTAMPS
            synchronizer.timestamps(getRadioTimestamp(),measuredFrameStart);
            break;
            #endif //SEND_TIMESTAMPS
            miosix::ledOn();
        }
        timer.stopTimeoutTimer();
        #ifdef MULTI_HOP
        if(!timeout) rebroadcast();
        #endif //MULTI_HOP
    }
    tr.setMode(Transceiver::SLEEP);
    
    pair<int,int> r;
    if(timeout)
    {
        if(++missPackets>maxMissPackets)
        {
            puts("Lost sync");
            return true;
        }
        r=synchronizer.lostPacket();
        measuredFrameStart=computedFrameStart;
    } else {
        r=synchronizer.computeCorrection(measuredFrameStart-computedFrameStart);
        missPackets=0;
    }
    clockCorrection=r.first;
    receiverWindow=r.second;
    
    printf("e=%d u=%d w=%d%s\n",measuredFrameStart-computedFrameStart,
        clockCorrection,receiverWindow,timeout ? " (miss)" : "");
    
    //Correct frame start considering hops
    measuredFrameStart-=hop*retransmitDelta;
    return false;
}

void FlooderSyncNode::resynchronize()
{ 
    synchronizer.reset();
    #ifndef SEND_TIMESTAMPS
    tr.setPacketLength(1);
    #else //SEND_TIMESTAMPS
    tr.setPacketLength(4);
    #endif //SEND_TIMESTAMPS
    tr.setMode(Transceiver::RX);
    timer.initTimeoutTimer(0);
    tr.startReceiving();
    miosix::ledOn();
    for(;;)
    {
        timer.waitForPacketOrTimeout();
        //while(tr.irq()==false) ;
        measuredFrameStart=rtc.getPacketTimestamp();
        computedFrameStart=measuredFrameStart;
        #ifndef SEND_TIMESTAMPS
        char packet[1];
        #else //SEND_TIMESTAMPS
        unsigned int *packet=&receivedTimestamp;
        #endif //SEND_TIMESTAMPS
        tr.readPacket(packet);
        #ifndef SEND_TIMESTAMPS
        if(packet[0]==hop) break;
        #else //SEND_TIMESTAMPS
        synchronizer.timestamps(getRadioTimestamp(),measuredFrameStart);
        if(synchronizer.overwritesHardwareClock())
            computedFrameStart=getRadioTimestamp();
        break;
        #endif //SEND_TIMESTAMPS
    }
    miosix::ledOff();
    tr.setMode(Transceiver::SLEEP);
    clockCorrection=0;
    receiverWindow=w;
    missPackets=0;
    //Correct frame start considering hops
    measuredFrameStart-=hop*retransmitDelta;
}

void FlooderSyncNode::rebroadcast()
{
//     rtc.setAbsoluteWakeupWait(measuredFrameStart+retransmitDelta
//         -(receiverTurnOn+smallPacketTime+spiPktSend));
    tr.setMode(Transceiver::TX);
    timer.initTimeoutTimer(0);
    const char packet[]={hop+1};
//     rtc.wait();
    miosix::ledOn();
    tr.writePacket(packet);
    timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU
    miosix::ledOff(); //Falling edge signals synchronization packet sent
    tr.endWritePacket();
}
