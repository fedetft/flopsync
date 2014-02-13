 
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

    #if FLOPSYNC_DEBUG>0
    wakeup::mode(miosix::Mode::OUTPUT);
    pll::mode(miosix::Mode::OUTPUT);;
    syncVht::mode(miosix::Mode::OUTPUT);
    xoscRadioBoot::mode(miosix::Mode::OUTPUT);;
    jitterHW::mode(miosix::Mode::OUTPUT);;
    #endif //FLOPSYNC_DEBUG   
}

bool FlooderRootNode::synchronize()
{
    #if FLOPSYNC_DEBUG  >0
    if(wakeupTime<timer.getValue())
    {
        puts("--FLOPSYNC_DEBUG-- Wakeup is in the past.");
        assert(false);
    }
    #endif//FLOPSYNC_DEBUG
    timer.setAbsoluteWakeupSleep(wakeupTime);
    timer.sleep();
    frameStart=wakeupTime+jitterHWAbsorption+radioBoot+txTurnaroundTime+jitterSWAbsorption;
    timer.setAbsoluteWakeupWait(wakeupTime+jitterHWAbsorption+radioBoot);
    transceiver.setMode(Cc2520::TX);
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    const unsigned char payload[]={0x00};
    const unsigned char fcs[]={0xFF};
    syncFrame->setPayload(payload);
    syncFrame->setFCS(fcs);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    unsigned long long *payload=&frameStart;
    syncFrame->setPayload(payload);
    #endif //SEND_TIMESTAMPS
    transceiver.writeFrame(*syncFrame);
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    timer.wait();
    #if FLOPSYNC_DEBUG>0
    jitterHW::high();
    #endif//FLOPSYNC_DEBUG
    
    //CriticalSection cs;
    timer.setAbsoluteTriggerEvent(frameStart-txTurnaroundTime);
    timer.wait();
    miosix::ledOn();
    timer.setAbsoluteTimeoutForEvent(frameStart+preamblePacketTime+delaySendPacketTime);
    #if FLOPSYNC_DEBUG <2
    timer.wait();
    #else //FLOPSYNC_DEBUG
    timer.wait()?
        puts("--FLOPSYNC_DEBUG-- Timeout occurred."):puts("--FLOPSYNC_DEBUG-- Event occurred.");
    #endif//FLOPSYNC_DEBUG
    #if FLOPSYNC_DEBUG <2
    transceiver.isSFDRaised();
    #else //FLOPSYNC_DEBUG
    transceiver.isSFDRaised()?
        puts("--FLOPSYNC_DEBUG-- SFD raised."):puts("--FLOPSYNC_DEBUG-- No SFD raised.");
    #endif//FLOPSYNC_DEBUG
    timer.setAbsoluteTimeoutForEvent(frameStart+packetTime+delaySendPacketTime);
    timer.wait();
    #if FLOPSYNC_DEBUG <2
    transceiver.isTxFrameDone();
    #else //FLOPSYNC_DEBUG
    transceiver.isTxFrameDone()?
        puts("--FLOPSYNC_DEBUG-- Tx Frame done."):puts("--FLOPSYNC_DEBUG-- Tx Frame not done.");
    #endif//FLOPSYNC_DEBUG
    miosix::ledOff(); //Falling edge signals synchronization packet sent
    
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    wakeupTime+=nominalPeriod;
    
    #if FLOPSYNC_DEBUG>0
    wakeup::low();
    pll::low();
    syncVht::low();
    xoscRadioBoot::low();
    jitterHW::low();
    #endif//FLOPSYNC_DEBUG
    
    return false; //Root node does not desynchronize
}

FlooderRootNode::~FlooderRootNode()
{
    delete syncFrame;
}
