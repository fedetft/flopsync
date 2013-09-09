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

using namespace std;

//
// class FloodingScheme
//

void FloodingScheme::resynchronize() {}

FloodingScheme::~FloodingScheme() {}

//
// class Synchronizer
//

#ifdef SEND_TIMESTAMPS
void Synchronizer::receivedTimestamp(unsigned int timestamp) {}

bool Synchronizer::overwritesHardwareClock() const { return false; }
#endif //SEND_TIMESTAMPS

Synchronizer::~Synchronizer() {}

//
// class Clock
//

Clock::~Clock() {}

//
// class FlooderRootNode
//

FlooderRootNode::FlooderRootNode(Timer& rtc) : rtc(rtc),
        nrf(Nrf24l01::instance()), timer(AuxiliaryTimer::instance()),
        frameStart(0), wakeupTime(rtc.getValue()+nominalPeriod) {}

bool FlooderRootNode::synchronize()
{
    assert(static_cast<int>(rtc.getValue()-wakeupTime)<0);
    rtc.setAbsoluteWakeupSleep(wakeupTime);
    rtc.sleep();
    rtc.setAbsoluteWakeupWait(wakeupTime+jitterAbsorption);
    nrf.setMode(Nrf24l01::TX);
    #ifndef SEND_TIMESTAMPS
    nrf.setPacketLength(1);
    const char packet[]={0x00};
    #else //SEND_TIMESTAMPS
    nrf.setPacketLength(4);
    unsigned int timestamp;
    unsigned int *packet=&timestamp;
    #endif //SEND_TIMESTAMPS
    timer.initTimeoutTimer(0);
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    {
        CriticalSection cs;
        rtc.wait();
        miosix::ledOn();
        #ifdef SEND_TIMESTAMPS
        timestamp=rtc.getPacketTimestamp();
        #endif //SEND_TIMESTAMPS
        nrf.writePacket(packet);
        timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU
        frameStart=rtc.getPacketTimestamp();
        miosix::ledOff(); //Falling edge signals synchronization packet sent
    }
    nrf.endWritePacket();
    nrf.setMode(Nrf24l01::SLEEP);
    wakeupTime+=nominalPeriod;
    return false; //Root node does not desynchronize
}

//
// class FlooderSyncNode
//

FlooderSyncNode::FlooderSyncNode(Timer& rtc, Synchronizer& synchronizer,
        unsigned char hop) : rtc(rtc), nrf(Nrf24l01::instance()),
        timer(AuxiliaryTimer::instance()), synchronizer(synchronizer),
        measuredFrameStart(0), computedFrameStart(0), clockCorrection(0),
        receiverWindow(w), missPackets(maxMissPackets+1), hop(hop) {}

bool FlooderSyncNode::synchronize()
{
    if(missPackets>maxMissPackets) return true;
    
    computedFrameStart+=nominalPeriod+clockCorrection;
    unsigned int wakeupTime=computedFrameStart-
        (jitterAbsorption+receiverTurnOn+receiverWindow+smallPacketTime);
    assert(static_cast<int>(rtc.getValue()-wakeupTime)<0);
    rtc.setAbsoluteWakeupSleep(wakeupTime);
    rtc.sleep();
    rtc.setAbsoluteWakeupWait(wakeupTime+jitterAbsorption);
    nrf.setMode(Nrf24l01::RX);
    #ifndef SEND_TIMESTAMPS
    nrf.setPacketLength(1);
    #else //SEND_TIMESTAMPS
    nrf.setPacketLength(4);
    #endif //SEND_TIMESTAMPS
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    rtc.wait();
    miosix::ledOn();
    nrf.startReceiving();
    timer.initTimeoutTimer(toAuxiliaryTimer(
        receiverTurnOn+2*receiverWindow+smallPacketTime));
    bool timeout;
    {
        CriticalSection cs;
        for(;;)
        {
            timeout=timer.waitForPacketOrTimeout();
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
            nrf.readPacket(packet);
            #ifndef SEND_TIMESTAMPS
            if(packet[0]==hop) break;
            #else //SEND_TIMESTAMPS
            synchronizer.receivedTimestamp(getRadioTimestamp());
            break;
            #endif //SEND_TIMESTAMPS
            miosix::ledOn();
        }
        timer.stopTimeoutTimer();
        #ifdef MULTI_HOP
        if(!timeout) rebroadcast();
        #endif //MULTI_HOP
    }
    nrf.setMode(Nrf24l01::SLEEP);
    
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
    #ifndef SEND_TIMESTAMPS
    nrf.setPacketLength(1);
    #else //SEND_TIMESTAMPS
    nrf.setPacketLength(4);
    #endif //SEND_TIMESTAMPS
    nrf.setMode(Nrf24l01::RX);
    timer.initTimeoutTimer(0);
    nrf.startReceiving();
    miosix::ledOn();
    for(;;)
    {
        timer.waitForPacketOrTimeout();
        measuredFrameStart=rtc.getPacketTimestamp();
        computedFrameStart=measuredFrameStart;
        #ifndef SEND_TIMESTAMPS
        char packet[1];
        #else //SEND_TIMESTAMPS
        unsigned int *packet=&receivedTimestamp;
        #endif //SEND_TIMESTAMPS
        nrf.readPacket(packet);
        #ifndef SEND_TIMESTAMPS
        if(packet[0]==hop) break;
        #else //SEND_TIMESTAMPS
        synchronizer.receivedTimestamp(getRadioTimestamp());
        if(synchronizer.overwritesHardwareClock())
            computedFrameStart=getRadioTimestamp();
        break;
        #endif //SEND_TIMESTAMPS
    }
    miosix::ledOff();
    nrf.setMode(Nrf24l01::SLEEP);
    synchronizer.reset();
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
    nrf.setMode(Nrf24l01::TX);
    timer.initTimeoutTimer(0);
    const char packet[]={hop+1};
//     rtc.wait();
    miosix::ledOn();
    nrf.writePacket(packet);
    timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU
    miosix::ledOff(); //Falling edge signals synchronization packet sent
    nrf.endWritePacket();
}

//
// class OptimizedRampFlopsync
//

OptimizedRampFlopsync::OptimizedRampFlopsync() { reset(); }

pair<int,int> OptimizedRampFlopsync::computeCorrection(int e)
{
    //u(k)=2u(k-1)-u(k-2)+2.75e(k)-2.984375e(k-1)+e(k-2) with values kept multiplied by 64
    //int u=2*uo-uoo+176*e-191*eo+64*eoo;
    
    //u(k)=2u(k-1)-u(k-2)+2.25e(k)-2.859375e(k-1)+e(k-2) with values kept multiplied by 64
    int u=2*uo-uoo+144*e-183*eo+64*eoo;
    uoo=uo;
    uo=u;
    eoo=eo;
    eo=e;

    int sign=u>=0 ? 1 : -1;
    int uquant=(u+32*sign)/64;
    
    
    //Scale numbers if VHT is enabled to prevent overflows
    int wMax=w/scaleFactor;
    int wMin=minw/scaleFactor;
    e/=scaleFactor;
    
    //Update receiver window size
    sum+=e*fp;
    squareSum+=e*e*fp;
    if(++count>=numSamples)
    {
        //Variance computed as E[X^2]-E[X]^2
        int average=sum/numSamples;
        int var=squareSum/numSamples-average*average/fp;
        //Using the Babylonian method to approximate square root
        int stddev=var/7;
        for(int j=0;j<3;j++) if(stddev>0) stddev=(stddev+var*fp/stddev)/2;
        //Set the window size to three sigma
        int winSize=stddev*3/fp;
        //Clamp between min and max window
        dw=max<int>(min<int>(winSize,wMax),wMin);
        sum=squareSum=count=0;
    }

    return make_pair(uquant,scaleFactor*dw);
}

pair<int,int> OptimizedRampFlopsync::lostPacket()
{
    //Double receiver window on packet loss, still clamped to max value
    dw=min<int>(2*dw,w/scaleFactor);
    return make_pair(getClockCorrection(),scaleFactor*dw);
}

void OptimizedRampFlopsync::reset()
{
    uo=uoo=eo=eoo=sum=squareSum=count=0;
    dw=w/scaleFactor;
}

int OptimizedRampFlopsync::getClockCorrection() const
{
    //Error measure is unavailable if the packet is lost, the best we can
    //do is to reuse the past correction value
    int sign=uo>=0 ? 1 : -1;
    return (uo+32*sign)/64;
}

//
// class OptimizedFlopsync
//

OptimizedFlopsync::OptimizedFlopsync() { reset(); }

pair<int,int> OptimizedFlopsync::computeCorrection(int e)
{
    //u(k)=u(k-1)+1.46875*e(k)-e(k-1) with values kept multiplied by 32
    int u=uo+47*e-32*eo;

    //The controller output needs to be quantized, but instead of simply
    //doing u/4 which rounds towards the lowest number use a slightly more
    //advanced algorithm to round towards the closest one, as when the error
    //is close to +/-1 timer tick this makes a significant difference.
    int sign=u>=0 ? 1 : -1;
    int uquant=(u+16*sign)/32;
    
    //Adaptive state quantization, while the output always needs to be
    //quantized, the state is only quantized if the error is zero
    uo= e==0 ? 32*uquant : u;
    eo=e;
    
    //Scale numbers if VHT is enabled to prevent overflows
    int wMax=w/scaleFactor;
    int wMin=minw/scaleFactor;
    e/=scaleFactor;
    
    //Update receiver window size
    sum+=e*fp;
    squareSum+=e*e*fp;
    if(++count>=numSamples)
    {
        //Variance computed as E[X^2]-E[X]^2
        int average=sum/numSamples;
        int var=squareSum/numSamples-average*average/fp;
        //Using the Babylonian method to approximate square root
        int stddev=var/7;
        for(int j=0;j<3;j++) if(stddev>0) stddev=(stddev+var*fp/stddev)/2;
        //Set the window size to three sigma
        int winSize=stddev*3/fp;
        //Clamp between min and max window
        dw=max<int>(min<int>(winSize,wMax),wMin);
        sum=squareSum=count=0;
    }

    return make_pair(uquant,scaleFactor*dw);
}

pair<int,int> OptimizedFlopsync::lostPacket()
{
    //Double receiver window on packet loss, still clamped to max value
    dw=min<int>(2*dw,w/scaleFactor);
    return make_pair(getClockCorrection(),scaleFactor*dw);
}

void OptimizedFlopsync::reset()
{
    uo=eo=sum=squareSum=count=0;
    dw=w/scaleFactor;
}

int OptimizedFlopsync::getClockCorrection() const
{
    //Error measure is unavailable if the packet is lost, the best we can
    //do is to reuse the past correction value
    int sign=uo>=0 ? 1 : -1;
    return (uo+16*sign)/32;
}

//
// class OptimizedDeadbeatFlopsync
//

OptimizedDeadbeatFlopsync::OptimizedDeadbeatFlopsync() { reset(); }

pair<int,int> OptimizedDeadbeatFlopsync::computeCorrection(int e)
{
    //u(k)=u(k-1)+2*e(k)-e(k-1)
    int u=uo+2*e-eo;
    uo=u;
    eo=e;
    
    //Scale numbers if VHT is enabled to prevent overflows
    int wMax=w/scaleFactor;
    int wMin=minw/scaleFactor;
    e/=scaleFactor;
    
    //Update receiver window size
    sum+=e*fp;
    squareSum+=e*e*fp;
    if(++count>=numSamples)
    {
        //Variance computed as E[X^2]-E[X]^2
        int average=sum/numSamples;
        int var=squareSum/numSamples-average*average/fp;
        //Using the Babylonian method to approximate square root
        int stddev=var/7;
        for(int j=0;j<3;j++) if(stddev>0) stddev=(stddev+var*fp/stddev)/2;
        //Set the window size to three sigma
        int winSize=stddev*3/fp;
        //Clamp between min and max window
        dw=max<int>(min<int>(winSize,wMax),wMin);
        sum=squareSum=count=0;
    }

    return make_pair(u,scaleFactor*dw);
}

pair<int,int> OptimizedDeadbeatFlopsync::lostPacket()
{
    //Double receiver window on packet loss, still clamped to max value
    dw=min<int>(2*dw,w/scaleFactor);
    return make_pair(getClockCorrection(),scaleFactor*dw);
}

void OptimizedDeadbeatFlopsync::reset()
{
    uo=eo=sum=squareSum=count=0;
    dw=w/scaleFactor;
}

//
// class DummySynchronizer
//

DummySynchronizer::DummySynchronizer() { reset(); }

pair<int,int> DummySynchronizer::computeCorrection(int e)
{
    uo=e;
    return make_pair(uo,w);
}

pair<int,int> DummySynchronizer::lostPacket()
{
    return make_pair(uo,w);
}

void DummySynchronizer::reset()
{
    uo=0;
}

//
// class DummySynchronizer2
//
#ifdef SEND_TIMESTAMPS

DummySynchronizer2::DummySynchronizer2(Timer& timer) : timer(timer) { reset(); }

void DummySynchronizer2::receivedTimestamp(unsigned int timestamp)
{
    //unsigned int rtcVal=timer.getValue();
    //printf("rtc=%d, timestamp=%d\n",rtcVal,timestamp); //FIXME: remove
    timer.setValue(timestamp);
}

pair<int,int> DummySynchronizer2::computeCorrection(int e)
{
    return make_pair(0,w);
}

pair<int,int> DummySynchronizer2::lostPacket()
{
    return make_pair(0,w);
}
#endif //SEND_TIMESTAMPS

//
// class MonotonicClock
//

unsigned int MonotonicClock::rootFrame2localAbsolute(unsigned int root)
{
    #ifdef SEND_TIMESTAMPS
    //Can't have a monotonic clock with a sync scheme that overwrites the RTC
    assert(sync.overwritesHardwareClock()==false);
    #endif //SEND_TIMESTAMPS
    int signedRoot=root; //Conversion unsigned to signed is *required*
    int period=nominalPeriod;
    long long correction=signedRoot;
    correction*=sync.getClockCorrection();
    int sign=correction>=0 ? 1 : -1; //Round towards closest
    int dividedCorrection=(correction+(sign*period/2))/period;
    return flood.getComputedFrameStart()+max(0,signedRoot+dividedCorrection);
}

//
// class NonMonotonicClock
//

unsigned int NonMonotonicClock::rootFrame2localAbsolute(unsigned int root)
{
    int signedRoot=root; //Conversion unsigned to signed is *required*
    int period=nominalPeriod;
    long long correction=signedRoot;
    correction*=sync.getClockCorrection()-sync.getSyncError();
    int sign=correction>=0 ? 1 : -1; //Round towards closest
    int dividedCorrection=(correction+(sign*period/2))/period;
    #ifndef SEND_TIMESTAMPS
    return flood.getMeasuredFrameStart()+max(0,signedRoot+dividedCorrection);
    #else //SEND_TIMESTAMPS
    return (sync.overwritesHardwareClock() ?
        flood.getRadioTimestamp() : flood.getMeasuredFrameStart()) +
        max(0,signedRoot+dividedCorrection);
    #endif //SEND_TIMESTAMPS
}
