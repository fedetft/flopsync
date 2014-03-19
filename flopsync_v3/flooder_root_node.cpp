 
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
#include "flooder_root_node.h"

#include "critical_section.h"
#include <cassert>

using namespace std;

#ifndef SYNC_BY_WIRE
//
// class FlooderRootNode
//

FlooderRootNode::FlooderRootNode(Timer& timer) : timer(timer),
        transceiver(Cc2520::instance()),frameStart(0), 
        wakeupTime(timer.getValue()+nominalPeriod) 
{
    #ifndef SEND_TIMESTAMPS
    syncFrame = new Frame(1,false,false,1);
    #else
    syncFrame = new Frame(8,false,true);
    #endif //SEND_TIMESTAMPS
}

bool FlooderRootNode::synchronize()
{
    #if FLOPSYNC_DEBUG  >0
    assert(timer.getValue()<wakeupTime);
    #endif//FLOPSYNC_DEBUG
    timer.absoluteSleep(wakeupTime);
    frameStart=wakeupTime+jitterAbsorption+txTurnaroundTime+trasmissionTime;
    transceiver.setMode(Cc2520::TX);
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    const unsigned char payload[1]={0x00};
    const unsigned char fcs[1]={0xFF};
    syncFrame->setPayload(payload);
    syncFrame->setFCS(fcs);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    unsigned char *payload=reinterpret_cast<unsigned char*>(&frameStart);
    syncFrame->setPayload(payload);
    #endif //SEND_TIMESTAMPS
    #if FLOPSYNC_DEBUG  >0
    assert(transceiver.writeFrame(*syncFrame)==0);
    #else//FLOPSYNC_DEBUG
    transceiver.writeFrame(*syncFrame);
    #endif//FLOPSYNC_DEBUG

    #if FLOPSYNC_DEBUG  >0
    assert(timer.getValue()<frameStart-txTurnaroundTime-trasmissionTime);
    #endif//FLOPSYNC_DEBUG
    miosix::ledOn();
    timer.absoluteWaitTrigger(frameStart-txTurnaroundTime-trasmissionTime);
    timer.absoluteWaitTimeoutOrEvent(frameStart-trasmissionTime+preambleFrameTime+delaySendPacketTime);
    transceiver.isSFDRaised();
    timer.absoluteWaitTimeoutOrEvent(frameStart-trasmissionTime+frameTime+delaySendPacketTime);
    transceiver.isTxFrameDone();
    
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    wakeupTime+=nominalPeriod;
    miosix::ledOff(); //Falling edge signals synchronization packet sent
    
    return false; //Root node does not desynchronize
}

FlooderRootNode::~FlooderRootNode()
{
    delete syncFrame;
}

#else//SYNC_BY_WIRE

//**********************************************************
//*              Synchronization by Wire                   *
//**********************************************************
//
// class FlooderSyncNode
//

FlooderRootNode::FlooderRootNode(Timer& timer) : timer(timer),
        transceiver(Cc2520::instance()),frameStart(0), 
        wakeupTime(timer.getValue()+nominalPeriod) {}

bool FlooderRootNode::synchronize()
{
    #if FLOPSYNC_DEBUG  >0
    assert(timer.getValue()<wakeupTime);
    #endif//FLOPSYNC_DEBUG
    timer.absoluteSleep(wakeupTime);
    frameStart=wakeupTime+jitterAbsorption;

    #if FLOPSYNC_DEBUG  >0
    assert(timer.getValue()<frameStart);
    #endif//FLOPSYNC_DEBUG
    miosix::ledOn();
    timer.absoluteWaitTrigger(frameStart);
    miosix::ledOff(); //Falling edge signals synchronization packet sent
    wakeupTime+=nominalPeriod;
    return false; //Root node does not desynchronize
}

FlooderRootNode::~FlooderRootNode()
{}
#endif//SYNC_BY_WIRE