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

#include "flopsync2.h"
#include <cstdio>
#include <miosix.h>
#include "protocol_constants.h"

using namespace std;

//
// class FloodingScheme
//

void FloodingScheme::resynchronize() {}

//
// class FlooderRootNode
//

FlooderRootNode::FlooderRootNode() : rtc(Rtc::instance()),
        nrf(Nrf24l01::instance()), timer(AuxiliaryTimer::instance()),
        frameStart(0), wakeupTime(rtc.getValue()+nominalPeriod) {}

bool FlooderRootNode::synchronize()
{
    //TODO: check that wakeupTime hasn't passed
    rtc.setAbsoluteWakeup(wakeupTime);
    rtc.sleepAndWait();
    rtc.setAbsoluteWakeup(wakeupTime+jitterAbsorption);
    nrf.setMode(Nrf24l01::TX);
    nrf.setPacketLength(1);
    timer.initTimeoutTimer(0);
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    rtc.wait();
    miosix::ledOn();
    static const char packet[]={0x00};
    nrf.writePacket(packet);
    timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU
    frameStart=rtc.getValue();
    miosix::ledOff(); //Falling edge signals synchronization packet sent
    nrf.endWritePacket();
    nrf.setMode(Nrf24l01::SLEEP);    
    wakeupTime+=nominalPeriod;
    return false; //Root node does not desynchronize
}

//
// class FlooderSyncNode
//

FlooderSyncNode::FlooderSyncNode() : rtc(Rtc::instance()),
        nrf(Nrf24l01::instance()), timer(AuxiliaryTimer::instance()),
        measuredFrameStart(0), computedFrameStart(0), clockCorrection(0),
        receiverWindow(w), missPackets(maxMissPackets) {}

bool FlooderSyncNode::synchronize()
{
    if(missPackets>=maxMissPackets) return true;
    
    //TODO: check that computedFrameStart hasn't passed
    unsigned int wakeupTime=computedFrameStart-
        (jitterAbsorption+receiverTurnOn+receiverWindow+smallPacketTime);
    rtc.setAbsoluteWakeup(wakeupTime);
    rtc.sleepAndWait();
    rtc.setAbsoluteWakeup(wakeupTime+jitterAbsorption);
    nrf.setMode(Nrf24l01::RX);
    nrf.setPacketLength(1);
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    rtc.wait();
    miosix::ledOn();
    nrf.startReceiving();
    timer.initTimeoutTimer(receiverTurnOn+2*receiverWindow+smallPacketTime);
    bool timeout;
    for(;;)
    {
        timeout=timer.waitForPacketOrTimeout();
        measuredFrameStart=rtc.getValue();
        // Falling edge signals RX received the packet.
        // It happens between 4 and 16us from the transmitter's falling edge
        // (measured with an oscilloscope)
        miosix::ledOff();
        if(timeout) break;
        char packet[1];
        nrf.readPacket(packet);
        if(packet[0]==0x00) break;
    }
    timer.stopTimeoutTimer();
    nrf.setMode(Nrf24l01::SLEEP);
    
    int e=measuredFrameStart-computedFrameStart;
    if(timeout)
    {
        if(++missPackets>=maxMissPackets)
        {
            puts("Lost sync");
            return true;
        }
        ve.setWindow(2*receiverWindow);
        e=0;
        //And leave clockCorrection as the previously computed value
    } else {
        missPackets=0;
        clockCorrection=controller.run(e);
        ve.update(e);
    }
    receiverWindow=max<int>(min<int>(ve.window(),w),minw);

    printf("e=%d u=%d w=%d%s\n",e,clockCorrection,receiverWindow,timeout ? " (miss)" : "");

    computedFrameStart+=nominalPeriod+clockCorrection;
    return false;
}

void FlooderSyncNode::resynchronize()
{ 
    nrf.setPacketLength(1);
    nrf.setMode(Nrf24l01::RX);
    timer.initTimeoutTimer(0);
    nrf.startReceiving();
    for(;;)
    {
        timer.waitForPacketOrTimeout();
        measuredFrameStart=rtc.getValue();
        computedFrameStart=measuredFrameStart+nominalPeriod;
        char packet[1];
        nrf.readPacket(packet);
        if(packet[0]==0x00) break;
    }
    nrf.setMode(Nrf24l01::SLEEP);
    missPackets=0;
}
