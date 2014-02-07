 
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
    #ifdef FLOPSYNC_DEBUG   
    if(wakeupTime<timer.getValue())
    {
        puts("--FLOPSYNC_DEBUG-- Wakeup is in the past.");
        assert(false);
    }
    #endif//FLOPSYNC_DEBUG
    timer.setAbsoluteWakeupSleep(wakeupTime);
    timer.sleep();
    frameStart=wakeupTime+jitterAbsorption;
    timer.setAbsoluteTriggerEvent(frameStart);
    transceiver.setMode(Cc2520::TX);
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    const unsigned char payload[]={0x00};
    const unsigned char fcs[]={0xFF};
    syncFrame->setPayload(payload);
    syncFrame->setFCS(fcs);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    unsigned long long timestamp = wakeupTime+jitterAbsorption;
    unsigned long long *payload=&timestamp;
    syncFrame->setPayload(payload);
    #endif //SEND_TIMESTAMPS
    transceiver.writeFrame(*syncFrame);
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    timer.wait();
    miosix::ledOn();
    timer.setAbsoluteTimeout(wakeupTime+jitterAbsorption+preamblePacketTime+delaySendPacketTime);
    #ifndef FLOPSYNC_DEBUG
    timer.waitForExtEventOrTimeout();
    #else //FLOPSYNC_DEBUG
    timer.waitForExtEventOrTimeout()?
        puts("--FLOPSYNC_DEBUG-- Timeout occurred."):puts("--FLOPSYNC_DEBUG-- Event occurred.");
    #endif//FLOPSYNC_DEBUG
    #ifndef FLOPSYNC_DEBUG
    transceiver.isSFDRaised();
    #else //FLOPSYNC_DEBUG
    transceiver.isSFDRaised()?
        puts("--FLOPSYNC_DEBUG-- SFD raised."):puts("--FLOPSYNC_DEBUG-- No SFD raised.");
    #endif//FLOPSYNC_DEBUG
    timer.setAbsoluteTimeout(wakeupTime+jitterAbsorption+preamblePacketTime
                           +payloadPacketTime+fcsPacketTime+delaySendPacketTime);
    timer.waitForExtEventOrTimeout();
    #ifndef FLOPSYNC_DEBUG
    transceiver.isTxFrameDone();
    #else //FLOPSYNC_DEBUG
    transceiver.isTxFrameDone()?
        puts("--FLOPSYNC_DEBUG-- Tx Frame done."):puts("--FLOPSYNC_DEBUG-- Tx Frame not done.");
    #endif//FLOPSYNC_DEBUG
    miosix::ledOff(); //Falling edge signals synchronization packet sent
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    wakeupTime+=nominalPeriod;
    return false; //Root node does not desynchronize
}

FlooderRootNode::~FlooderRootNode()
{
    delete syncFrame;
}
