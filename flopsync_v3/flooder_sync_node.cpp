 
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
        receiverWindow(w), missPackets(maxMissPackets+1), hop(hop) 
{
    #if FLOPSYNC_DEBUG >0
    wakeup::mode(miosix::Mode::OUTPUT);
    pll::mode(miosix::Mode::OUTPUT);
    syncVht::mode(miosix::Mode::OUTPUT);
    xoscRadioBoot::mode(miosix::Mode::OUTPUT);
    jitterHW::mode(miosix::Mode::OUTPUT);
    #endif//FLOPSYNC_DEBUG
}
        
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
    unsigned long long wakeupTime=computedFrameStart-(jitterHWAbsorption+radioBoot+rxTurnaroundTime+receiverWindow);
    #if FLOPSYNC_DEBUG > 0   
    if(wakeupTime<timer.getValue())
    {
        puts("--FLOPSYNC_DEBUG-- Wakeup is in the past.");
        assert(false);
    }   
    #endif//FLOPSYNC_DEBUG
    timer.setAbsoluteWakeupSleep(wakeupTime);
    #if FLOPSYNC_DEBUG>0   
    if(wakeupTime<timer.getValue())
    {
        puts("--FLOPSYNC_DEBUG-- Wakeup is in the past.");
        assert(false);
    }
    #endif
    timer.sleep();
    timer.setAbsoluteWakeupWait(wakeupTime+jitterHWAbsorption+radioBoot);
    miosix::ledOn();
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    #endif //SEND_TIMESTAMPS
    transceiver.setMode(Cc2520::RX);
    timer.wait();
    #if FLOPSYNC_DEBUG >0
    jitterHW::high();
    #endif//FLOPSYNC_DEBUG
    bool timeout;
    {
        CriticalSection cs;
        for(;;)
        {
            //timeout end of window
            timer.setAbsoluteTimeoutForEvent(computedFrameStart+preamblePacketTime+receiverWindow);
            timeout=timer.wait();
            measuredFrameStart=timer.getExtEventTimestamp()-preamblePacketTime;
            
            miosix::ledOff();
            if(timeout) break;
            
            #if FLOPSYNC_DEBUG<2
            transceiver.isSFDRaised();
            #else //FLOPSYNC_DEBUG
            transceiver.isSFDRaised()?puts("--FLOPSYNC_DEBUG-- SFD raised."):puts("--FLOPSYNC_DEBUG-- No SFD raised.");
            #endif//FLOPSYNC_DEBUG
            //Wait end of packet
            timer.setAbsoluteTimeoutForEvent(measuredFrameStart+packetTime+delaySendPacketTime);
            #ifndef SEND_TIMESTAMPS
            //Empty frame not inizialized with lenght.
            syncFrame = new Frame(false,false,1);
            #else
            syncFrame = new Frame(false,true);
            #endif //SEND_TIMESTAMPS
            timer.wait();
            #if FLOPSYNC_DEBUG <2
            transceiver.isRxFrameDone();
            #else //FLOPSYNC_DEBUG
            transceiver.isRxFrameDone()?puts("--FLOPSYNC_DEBUG-- Rx Frame done."):puts("--FLOPSYNC_DEBUG-- Rx Frame not done.");
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
        
        if(!timeout)
        {
           #ifdef MULTI_HOP 
           rebroadcast(measuredFrameStart+payloadPacketTime+piggybackingTime+fcsPacketTime+delayRebroadcastTime);
           #endif //MULTI_HOP
           delete syncFrame;
        }
    }
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    #if FLOPSYNC_DEBUG>0
    wakeup::low();
    pll::low();
    syncVht::low();
    xoscRadioBoot::low();
    jitterHW::low();
    #endif//FLOPSYNC_DEBUG
    
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
    
    timeout? printf("e=%d u=%d w=%d ---> miss\n",e,clockCorrection,receiverWindow):
                    printf("e=%d u=%d w=%d\n",e,clockCorrection,receiverWindow);
    
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
        //this is very important to prevent the pin of sfd and frm done is 
        //already high before you get in waiting for an event
        miosix::FastInterruptDisableLock dLock;
        transceiver.isRxFrameDone(); //disable exception frame done
        transceiver.isSFDRaised();   ///disable exception sfd done
        timer.setAbsoluteTimeoutForEvent(0);
        miosix::FastInterruptEnableLock eLock(dLock);
        
        timer.wait();
        measuredFrameStart=timer.getExtEventTimestamp()-preamblePacketTime;
        computedFrameStart=measuredFrameStart;    
        #if FLOPSYNC_DEBUG <2
        transceiver.isSFDRaised();
        #else //FLOPSYNC_DEBUG
        transceiver.isSFDRaised()?puts("--FLOPSYNC_DEBUG-- SFD raised."):puts("--FLOPSYNC_DEBUG-- No SFD raised.");
        #endif//FLOPSYNC_DEBUG
        //Wait end of packet
        timer.setAbsoluteTimeoutForEvent(measuredFrameStart+packetTime+delaySendPacketTime);
        #ifndef SEND_TIMESTAMPS
        //Empty frame not inizialized with lenght.
        syncFrame = new Frame(false,false,1);
        #else
        syncFrame = new Frame(false,true);
        #endif //SEND_TIMESTAMPS
        timer.wait();
        #if FLOPSYNC_DEBUG <2
        transceiver.isRxFrameDone();
        #else//FLOPSYNC_DEBUG
        transceiver.isRxFrameDone()?puts("--FLOPSYNC_DEBUG-- Rx Frame done."):puts("--FLOPSYNC_DEBUG-- Rx Frame not done.");
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
    delete syncFrame;
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

