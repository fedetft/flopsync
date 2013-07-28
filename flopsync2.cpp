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

#include "flopsync2.h"
#include <cstdio>
#include <cassert>
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
    assert(static_cast<int>(rtc.getValue()-wakeupTime)<0);
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

FlooderSyncNode::FlooderSyncNode(Synchronizer& synchronizer, unsigned char hop)
      : rtc(Rtc::instance()), nrf(Nrf24l01::instance()),
        timer(AuxiliaryTimer::instance()), synchronizer(synchronizer),
        measuredFrameStart(0), computedFrameStart(0),
        receiverWindow(w), missPackets(maxMissPackets+1), hop(hop) {}

bool FlooderSyncNode::synchronize()
{
    if(missPackets>maxMissPackets) return true;
    
    unsigned int wakeupTime=computedFrameStart-
        (jitterAbsorption+receiverTurnOn+receiverWindow+smallPacketTime);
    assert(static_cast<int>(rtc.getValue()-wakeupTime)<0);
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
        if(packet[0]==hop) break;
        miosix::ledOn();
    }
    timer.stopTimeoutTimer();
    #ifdef MULTI_HOP
    if(!timeout) rebroadcast();
    #endif //MULTI_HOP
    nrf.setMode(Nrf24l01::SLEEP);
    
    pair<short,unsigned char> r;
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
    short clockCorrection=r.first;
    receiverWindow=r.second;
    
    printf("e=%d u=%d w=%d%s\n",measuredFrameStart-computedFrameStart,
        clockCorrection,receiverWindow,timeout ? " (miss)" : "");
    
    //Correct frame start considering hops
    measuredFrameStart-=hop*retransmitDelta;
    computedFrameStart+=nominalPeriod+clockCorrection;
    return false;
}

void FlooderSyncNode::resynchronize()
{ 
    nrf.setPacketLength(1);
    nrf.setMode(Nrf24l01::RX);
    timer.initTimeoutTimer(0);
    nrf.startReceiving();
    miosix::ledOn();
    for(;;)
    {
        timer.waitForPacketOrTimeout();
        measuredFrameStart=rtc.getValue();
        computedFrameStart=measuredFrameStart+nominalPeriod;
        char packet[1];
        nrf.readPacket(packet);
        if(packet[0]==hop) break;
    }
    miosix::ledOff();
    nrf.setMode(Nrf24l01::SLEEP);
    synchronizer.reset();
    missPackets=0;
}

void FlooderSyncNode::rebroadcast()
{
    rtc.setAbsoluteWakeup(measuredFrameStart+retransmitDelta
        -receiverTurnOn-smallPacketTime);
    nrf.setMode(Nrf24l01::TX);
    timer.initTimeoutTimer(0);
    rtc.wait();
    miosix::ledOn();
    static const char packet[]={hop+1};
    nrf.writePacket(packet);
    timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU
    miosix::ledOff(); //Falling edge signals synchronization packet sent
    nrf.endWritePacket();
}

//
// class OptimizedFlopsync
//

OptimizedFlopsync::OptimizedFlopsync() { reset(); }

pair<short,unsigned char> OptimizedFlopsync::computeCorrection(short e)
{
    //u(k)=u(k-1)+1.25*e(k)-e(k-1) with values kept multiplied by 4
    short u=uo+5*e-4*eo;

    //The controller output needs to be quantized, but instead of simply
    //doing u/4 which rounds towards the lowest number use a slightly more
    //advanced algorithm to round towards the closest one, as when the error
    //is close to +/-1 timer tick this makes a significant difference.
    short sign=u>=0 ? 1 : -1;
    short uquant=(u+2*sign)/4;

    //Adaptive state quantization, while the output always needs to be
    //quantized, the state is only quantized if the error is zero
    uo= e==0 ? 4*uquant : u;
    eo=e;
    
    //Update variance computation
    sum+=e*fp;
    squareSum+=e*e*fp;
    if(++count>=numSamples)
    {
        sum/=numSamples;
        var=squareSum/numSamples-sum*sum/fp;
        var*=3; //Set the window size to three sigma
        var/=fp;
        var=max<int>(min<int>(var,w),minw); //Clamp between min and max window
        sum=squareSum=count=0;
    }

    return make_pair(uquant,var);
}

pair<short,unsigned char> OptimizedFlopsync::lostPacket()
{
    //Double receiver window on packet loss, still clamped to max value
    var=min<int>(2*var,w);
    return make_pair(getClockCorrection(),var);
}

void OptimizedFlopsync::reset()
{
    uo=eo=sum=squareSum=count=0;
    var=w;
}

int OptimizedFlopsync::getClockCorrection() const
{
    //Error measure is unavailable if the packet is lost, the best we can
    //do is to reuse the past correction value
    short sign=uo>=0 ? 1 : -1;
    return (uo+2*sign)/4;
}

unsigned int root2localFrameTime(const OptimizedFlopsync& fs, unsigned int root)
{
    int signedRoot=root; //Conversion unsigned to signed is *required*
    int period=nominalPeriod;
    return max(0,signedRoot+fs.getClockCorrection()*signedRoot/period);
}
