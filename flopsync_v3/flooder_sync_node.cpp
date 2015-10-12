 
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
#include "../drivers/leds.h"
#include <cassert>
#include "critical_section.h"
#include <cstdio>


using namespace std;

//
// class FlooderSyncNode
//

#ifndef SYNC_BY_WIRE

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
    unsigned long long wakeupTime=computedFrameStart-(jitterAbsorption+rxTurnaroundTime+receiverWindow);
    #if FLOPSYNC_DEBUG > 0   
    assert(timer.getValue()<wakeupTime);
    #endif//FLOPSYNC_DEBUG
    timer.absoluteSleep(wakeupTime);
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    #endif //SEND_TIMESTAMPS
    transceiver.setMode(Cc2520::IDLE);
    #if FLOPSYNC_DEBUG > 0   
    assert(timer.getValue()<computedFrameStart-rxTurnaroundTime-receiverWindow);
    #endif//FLOPSYNC_DEBUG
    timer.absoluteWait(computedFrameStart-rxTurnaroundTime-receiverWindow);
    transceiver.setMode(Cc2520::RX);
    bool timeout;
    led1::high();
    for(;;)
    {
        
        //When I enter for the first time in the cycle is safe that SFD and FRM_DONE 
        //are reset because I have rxTurnaraoundTime before the transceiver go in 
        //RX mode. In the other case if SFD arrive in the middle between isSFDRaised() 
        //and absoluteWaitTimeoutOrEvent() than we are going in timeout.
        timeout = timer.absoluteWaitTimeoutOrEvent(computedFrameStart+receiverWindow+preambleFrameTime);
        measuredFrameStart=timer.getExtEventTimestamp()-preambleFrameTime;
        led1::low();
        if(timeout) break;
        
        
        transceiver.isSFDRaised();
        //Wait end of packet
        #ifndef SEND_TIMESTAMPS
        //Empty frame not initialized with length.
        syncFrame = new Frame(false,false,1);
        #else
        syncFrame = new Frame(false,true);
        #endif //SEND_TIMESTAMPS
        timer.absoluteWaitTimeoutOrEvent(measuredFrameStart+frameTime+delaySendPacketTime);
        transceiver.isRxFrameDone();

        if(transceiver.readFrame(*syncFrame)==0)
        {
            #ifndef SEND_TIMESTAMPS
            unsigned char payload[syncFrame->getSizePayload()];
            unsigned char fcs[syncFrame->getSizeFCS()]; 
            syncFrame->getPayload(payload);
            syncFrame->getFCS(fcs);
            if((payload[0] & fcs[0]) ==0)
            {
                #ifndef GLOSSY
                if(payload[0]==hop) break;
                #else //GLOSSY
                hop=payload[0];
                break;
                #endif//GLOSSY

            }                
            #else //SEND_TIMESTAMPS
            unsigned char payload[syncFrame->getSizePayload()];
            unsigned char *ts=reinterpret_cast<unsigned char*>(&receivedTimestamp);
            syncFrame->getPayload(payload);
            if(syncFrame->getSizePayload()>=8)
            {
                memcpy(ts,payload,8);
                synchronizer.timestamps(getRadioTimestamp(),measuredFrameStart);  
                break;
            }
            #endif //SEND_TIMESTAMPS
        }
        delete syncFrame;
        transceiver.flushRxFifoBuffer();
        transceiver.flushRxFifoBuffer();
        led1::high();
    }

    if(!timeout)
    {
        //rebroadcast
        #ifdef MULTI_HOP
        transceiver.setMode(Cc2520::TX);
        #if FLOPSYNC_DEBUG>0
        probe_pin15::high();
        probe_pin15::low();
        #endif//FLOPSYNC_DEBUG
        
        transceiver.setAutoFCS(false);
        unsigned long long rebreoadcastStart=measuredFrameStart+frameTime+piggybackingTime+delayRebroadcastTime;
        unsigned char payload[syncFrame->getSizePayload()];
        unsigned char fcs[syncFrame->getSizeFCS()];
        payload[0]=(hop+1);
        fcs[0]=~payload[0];
        syncFrame->setPayload(payload);
        syncFrame->setFCS(fcs);
        #if FLOPSYNC_DEBUG  >0
        assert(transceiver.writeFrame(*syncFrame)==0);
        #else//FLOPSYNC_DEBUG
        transceiver.writeFrame(*syncFrame);
        #endif//FLOPSYNC_DEBUG
        #if FLOPSYNC_DEBUG > 0   
        assert(timer.getValue()<rebreoadcastStart-txTurnaroundTime-trasmissionTime);
        #endif//FLOPSYNC_DEBUG
        #if FLOPSYNC_DEBUG>0
        probe_pin15::high();
        probe_pin15::low();
        #endif//FLOPSYNC_DEBUG
        timer.absoluteWaitTrigger(rebreoadcastStart-txTurnaroundTime-trasmissionTime);
        led1::high();
        timer.absoluteWaitTimeoutOrEvent(rebreoadcastStart-trasmissionTime+preambleFrameTime+delaySendPacketTime);
        transceiver.isSFDRaised();
        timer.absoluteWaitTimeoutOrEvent(rebreoadcastStart-trasmissionTime+frameTime+delaySendPacketTime);
        transceiver.isTxFrameDone();
        led1::low();
        #endif //MULTI_HOP
        delete syncFrame;
    }
    
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    
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
    
    #if FLOPSYNC_DEBUG
    timeout? printf("e=%d u=%d w=%d ---> miss\n",e,clockCorrection,receiverWindow):
                    printf("e=%d u=%d w=%d\n",e,clockCorrection,receiverWindow);
    #endif//FLOPSYNC_DEBUG
    
    //Correct frame start considering hops
    measuredFrameStart-=hop*(frameTime+piggybackingTime+delayRebroadcastTime);
    return false;
}

void FlooderSyncNode::resynchronize()
{
    #if FLOPSYNC_DEBUG >0
    puts("Resynchronize...");
    #endif//FLOPSYNC_DEBUG
    synchronizer.reset();
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    #endif //SEND_TIMESTAMPS
    transceiver.setMode(Cc2520::RX);
    led1::high();
    for(;;)
    {
        //this is very important to prevent the pin of sfd and frm done is 
        //already high before you get in waiting for an event
        miosix::FastInterruptDisableLock dLock;
        transceiver.isRxFrameDone(); //disable exception frame done
        transceiver.isSFDRaised();   ///disable exception sfd done
        timer.absoluteWaitTimeoutOrEvent(0);
        miosix::FastInterruptEnableLock eLock(dLock);
        
        measuredFrameStart=timer.getExtEventTimestamp()-preambleFrameTime;
        computedFrameStart=measuredFrameStart;    
        transceiver.isSFDRaised();
        timer.absoluteWaitTimeoutOrEvent(measuredFrameStart+frameTime+delaySendPacketTime);
        transceiver.isRxFrameDone();
        //Wait end of packet
        #ifndef SEND_TIMESTAMPS
        //Empty frame not initialized with length.
        syncFrame = new Frame(false,false,1);
        #else
        syncFrame = new Frame(false,true);
        #endif //SEND_TIMESTAMPS
      
        if(transceiver.readFrame(*syncFrame)==0)
        {
            #ifndef SEND_TIMESTAMPS
            unsigned char payload[syncFrame->getSizePayload()];
            unsigned char fcs[syncFrame->getSizeFCS()];  
            syncFrame->getPayload(payload);
            syncFrame->getFCS(fcs);
            if((payload[0] & fcs[0]) ==0)
            {
                #ifndef GLOSSY
                if(payload[0]==hop) break;
                #else //GLOSSY
                break;
                #endif//GLOSSY
            }
            #else //SEND_TIMESTAMPS
            unsigned char payload[syncFrame->getSizePayload()];
            unsigned char *ts=reinterpret_cast<unsigned char*>(&receivedTimestamp);
            syncFrame->getPayload(payload);
            if(syncFrame->getSizePayload()>=8)
            {
                memcpy(ts,payload,8);
                synchronizer.timestamps(getRadioTimestamp(),measuredFrameStart);
                break;
            }
            #endif //SEND_TIMESTAMPS
        }
        delete syncFrame;
    }
    led1::low();
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    delete syncFrame;
    clockCorrection=0;
    receiverWindow=w;
    missPackets=0;
    //Correct frame start considering hops
    measuredFrameStart-=hop*(frameTime+piggybackingTime+delayRebroadcastTime);
}

FlooderSyncNode::~FlooderSyncNode()
{
   // delete syncFrame;
}

#else//SYNC_BY_WIRE

//**********************************************************
//*              Synchronization by Wire                   *
//**********************************************************
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
    unsigned long long wakeupTime=computedFrameStart-(jitterAbsorption+receiverWindow);
    #if FLOPSYNC_DEBUG > 0   
    assert(timer.getValue()<wakeupTime);
    #endif//FLOPSYNC_DEBUG
    timer.absoluteSleep(wakeupTime);
    bool timeout;
    led1::high();
    for(;;)
    {
        timeout = timer.absoluteWaitTimeoutOrEvent(computedFrameStart+receiverWindow);
        measuredFrameStart=timer.getExtEventTimestamp();
        led1::low();
        break;
        led1::high();
    }

    
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
    measuredFrameStart-=hop*(frameTime+piggybackingTime+delayRebroadcastTime);
    return false;
}

void FlooderSyncNode::resynchronize()
{
    #if FLOPSYNC_DEBUG >0
    puts("Resynchronize...");
    #endif//FLOPSYNC_DEBUG
    synchronizer.reset();
    led1::high();
    for(;;)
    {
        timer.absoluteWaitTimeoutOrEvent(0);
        
        measuredFrameStart=timer.getExtEventTimestamp();
        computedFrameStart=measuredFrameStart;    
        break;
    }
    led1::low();
    clockCorrection=0;
    receiverWindow=w;
    missPackets=0;
    //Correct frame start considering hops
    measuredFrameStart-=hop*(frameTime+piggybackingTime+delayRebroadcastTime);
}

FlooderSyncNode::~FlooderSyncNode(){}

#endif//SYNC_BY_WIRE