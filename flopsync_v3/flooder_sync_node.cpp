 
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

FlooderSyncNode::FlooderSyncNode(Timer& timer, Synchronizer& synchronizer,
        unsigned char hop) : timer(timer), transceiver(Cc2520::instance()), synchronizer(synchronizer),
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
    unsigned long long wakeupTime=computedFrameStart-
        (jitterAbsorption+receiverTurnOn+receiverWindow+preamblePacketTime);
    #ifdef FLOPSYNC_DEBUG   
    if(wakeupTime<timer.getValue())
    {
        puts("--FLOPSYNC_DEBUG-- Wakeup is in the past.");
        assert(false);
    }
    #endif
    timer.setAbsoluteWakeupSleep(wakeupTime);
    timer.sleep();
    timer.setAbsoluteWakeupWait(wakeupTime+jitterAbsorption);
    #ifdef FLOPSYNC_DEBUG   
    if(wakeupTime+jitterAbsorption<timer.getValue())
    {
        puts("--FLOPSYNC_DEBUG-- Wakeup is in the past.");
        assert(false);
    }
    #endif
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    timer.wait();
    miosix::ledOn();
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    #endif //SEND_TIMESTAMPS
    transceiver.setMode(Cc2520::RX);
    bool timeout;
    {
        CriticalSection cs;
        for(;;)
        {
            timer.setAbsoluteTimeout(wakeupTime+jitterAbsorption+receiverTurnOn+2*receiverWindow+preamblePacketTime);
            timeout=timer.waitForExtEventOrTimeout();
            measuredFrameStart=timer.getExtEventTimestamp();
            #ifndef FLOPSYNC_DEBUG
            transceiver.isSFDRaised();
            #else //FLOPSYNC_DEBUG
            transceiver.isSFDRaised()?printf("--FLOPSYNC_DEBUG-- SFD raised.\n"):printf("--FLOPSYNC_DEBUG-- No SFD raised.\n");
            #endif//FLOPSYNC_DEBUG
            miosix::ledOff();
            if(timeout) break;
            //Wait end of packet
            timer.setAbsoluteTimeout(measuredFrameStart+payloadPacketTime+fcsPacketTime+delaySendPacketTime);
            #ifndef SEND_TIMESTAMPS
            //Empty frame not inizialized with lenght.
            syncFrame = new Frame(false,false,1);
            #else
            syncFrame = new Frame(false,true);
            #endif //SEND_TIMESTAMPS
            timer.waitForExtEventOrTimeout();
            #ifndef FLOPSYNC_DEBUG
            transceiver.isRxFrameDone();
            #else //FLOPSYNC_DEBUG
            transceiver.isRxFrameDone()?printf("--FLOPSYNC_DEBUG-- Rx Frame done\n"):printf("--FLOPSYNC_DEBUG-- Rx Frame not done\n");
            #endif//FLOPSYNC_DEBUG
            
            if(transceiver.readFrame(*syncFrame)==0)
            {
                #ifndef SEND_TIMESTAMPS
                unsigned char payload[1];;
                unsigned char fcs[1];  
                syncFrame->getPayload(payload);
                syncFrame->getFCS(fcs);
                if((payload[0] & fcs[0]) ==0)
                {
                    #ifndef GLOSSY
                    if(payload[0]==hop) break;
                    #endif //GLOSSY
                }
                else
                {
                    delete syncFrame;
                }
                #else //SEND_TIMESTAMPS
                unsigned long long *timestamps=&receivedTimestamp;
                syncFrame->getPayload(timestamps);
                synchronizer.timestamps(getRadioTimestamp(),measuredFrameStart);
                break;
                #endif //SEND_TIMESTAMPS
            }
            miosix::ledOn();
            transceiver.flushRxFifoBuffer();
        }
        #ifdef MULTI_HOP
        if(!timeout) rebroadcast(measuredFrameStart+payloadPacketTime+delayRebroadcastTime);
        #endif //MULTI_HOP
    }
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    delete syncFrame;
    
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
    }
    else 
    {
        int e = measuredFrameStart-computedFrameStart;
        r=synchronizer.computeCorrection(e);
        missPackets=0;
    }
    clockCorrection=r.first;
    receiverWindow=r.second;
    int e = measuredFrameStart-computedFrameStart;
    printf("e=%d u=%d w=%d",e,clockCorrection,receiverWindow);
    timeout? printf(" ---> miss\n") : printf("\n");
    
    //Correct frame start considering hops
    measuredFrameStart-=hop*retransmitDelta;
    return false;
}

void FlooderSyncNode::resynchronize()
{ 
    synchronizer.reset();
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    #endif //SEND_TIMESTAMPS
    transceiver.setMode(Cc2520::RX);
    miosix::ledOn();
    for(;;)
    {
        timer.setAbsoluteTimeout(0);
        timer.waitForExtEventOrTimeout();
        measuredFrameStart=timer.getExtEventTimestamp();
        computedFrameStart=measuredFrameStart;    
        #ifndef FLOPSYNC_DEBUG
        transceiver.isSFDRaised();
        #else //FLOPSYNC_DEBUG
        transceiver.isSFDRaised()?puts("--FLOPSYNC_DEBUG-- SFD raised."):puts("--FLOPSYNC_DEBUG-- No SFD raised.");
        #endif//FLOPSYNC_DEBUG
        //Wait end of packet
        timer.setAbsoluteTimeout(measuredFrameStart+payloadPacketTime+fcsPacketTime+delaySendPacketTime);
        #ifndef SEND_TIMESTAMPS
        //Empty frame not inizialized with lenght.
        syncFrame = new Frame(false,false,1);
        #else
        syncFrame = new Frame(false,true);
        #endif //SEND_TIMESTAMPS
        timer.waitForExtEventOrTimeout();
        #ifndef FLOPSYNC_DEBUG
        transceiver.isRxFrameDone();
        #else //FLOPSYNC_DEBUG
        transceiver.isRxFrameDone()?puts("--FLOPSYNC_DEBUG-- Rx Frame done\n"):puts("--FLOPSYNC_DEBUG-- Rx Frame not done.");
        #endif//FLOPSYNC_DEBUG
      
        if(transceiver.readFrame(*syncFrame)==0)
        {
            #ifndef SEND_TIMESTAMPS
            unsigned char payload[1];
            unsigned char fcs[1];  
            syncFrame->getPayload(payload);
            syncFrame->getFCS(fcs);
            if((payload[0] & fcs[0]) ==0)
            {
                #ifndef GLOSSY
                if(payload[0]==hop) break;
                #endif //GLOSSY
                delete syncFrame;
            }
            else
            {
                delete syncFrame;
            }
            #else //SEND_TIMESTAMPS
            unsigned long long *timestamps=&receivedTimestamp;
            f->getPayload(timestamps);
            synchronizer.timestamps(getRadioTimestamp(),measuredFrameStart);
            if(synchronizer.overwritesHardwareClock())
                computedFrameStart=getRadioTimestamp();
            break;
            #endif //SEND_TIMESTAMPS
        }
    }
    miosix::ledOff();
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    clockCorrection=0;
    receiverWindow=w;
    missPackets=0;
    //Correct frame start considering hops
    #ifndef GLOSSY
    measuredFrameStart-=hop*retransmitDelta;
    #else// GLOSSY
    //FIXME
    #endif//GLOSSY
    
}

FlooderSyncNode::~FlooderSyncNode()
{
   // delete syncFrame;
}

