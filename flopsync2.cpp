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
#include <cassert>
#include <cstring>

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
void Synchronizer::timestamps(unsigned int globalTime, unsigned int localTime) {}

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
        timestamp=rtc.getValue();
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
    synchronizer.reset();
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
        synchronizer.timestamps(getRadioTimestamp(),measuredFrameStart);
        if(synchronizer.overwritesHardwareClock())
            computedFrameStart=getRadioTimestamp();
        break;
        #endif //SEND_TIMESTAMPS
    }
    miosix::ledOff();
    nrf.setMode(Nrf24l01::SLEEP);
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
    switch(init)
    {
        case 0:
            init=1;
            eo=e;
            uo=2*64*e;
            uoo=64*e;
            return make_pair(2*e,w); //One step of a deadbeat controller
        case 1:
            init=2;
            eo=0;
            uo/=2;
    }
    
    //alpha=3/8
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
    if(init==1)
    {
        init=2;
        eo=0;
        uo/=2;
    }
    //Double receiver window on packet loss, still clamped to max value
    dw=min<int>(2*dw,w/scaleFactor);
    return make_pair(getClockCorrection(),scaleFactor*dw);
}

void OptimizedRampFlopsync::reset()
{
    uo=uoo=eo=eoo=sum=squareSum=count=0;
    dw=w/scaleFactor;
    init=0;
}

int OptimizedRampFlopsync::getClockCorrection() const
{
    //Error measure is unavailable if the packet is lost, the best we can
    //do is to reuse the past correction value
    int sign=uo>=0 ? 1 : -1;
    return (uo+32*sign)/64;
}

//
// class OptimizedRampFlopsync2
//

OptimizedRampFlopsync2::OptimizedRampFlopsync2() { reset(); }

pair<int,int> OptimizedRampFlopsync2::computeCorrection(int e)
{
    //Controller preinit, for fast boot convergence
    switch(init)
    {
        case 0:
            init=1;
            eo=e;
            uo=2*512*e;
            uoo=512*e;
            return make_pair(2*e,w); //One step of a deadbeat controller
        case 1:
            init=2;
            eo=0;
            uo/=2;
    }
    
    //Flopsync controller, with alpha=3/8
    //u(k)=2u(k-1)-u(k-2)+1.875e(k)-2.578125e(k-1)+0.947265625e(k-2) with values kept multiplied by 512
    int u=2*uo-uoo+960*e-1320*eo+485*eoo;
    uoo=uo;
    uo=u;
    eoo=eo;
    eo=e;

    int sign=u>=0 ? 1 : -1;
    int uquant=(u+256*sign)/512;
    
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

pair<int,int> OptimizedRampFlopsync2::lostPacket()
{
    if(init==1)
    {
        init=2;
        eo=0;
        uo/=2;
    }
    //Double receiver window on packet loss, still clamped to max value
    dw=min<int>(2*dw,w/scaleFactor);
    return make_pair(getClockCorrection(),scaleFactor*dw);
}

void OptimizedRampFlopsync2::reset()
{
    uo=uoo=eo=eoo=sum=squareSum=count=0;
    dw=w/scaleFactor;
    init=0;
}

int OptimizedRampFlopsync2::getClockCorrection() const
{
    //Error measure is unavailable if the packet is lost, the best we can
    //do is to reuse the past correction value
    int sign=uo>=0 ? 1 : -1;
    return (uo+256*sign)/512;
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

#ifdef SEND_TIMESTAMPS
//
// class DummySynchronizer2
//

DummySynchronizer2::DummySynchronizer2(Timer& timer) : timer(timer) { reset(); }

void DummySynchronizer2::timestamps(unsigned int globalTime, unsigned int localTime)
{
    timer.setValue(globalTime+overwriteClockTime);
}

pair<int,int> DummySynchronizer2::computeCorrection(int e)
{
    this->e=e;
    return make_pair(0,w);
}

pair<int,int> DummySynchronizer2::lostPacket()
{
    return make_pair(0,w);
}

//
// class FTSP
//

FTSP::FTSP() : overflowCounterLocal(0), overflowCounterGlobal(0) { reset(); }

void FTSP::timestamps(unsigned int globalTime, unsigned int localTime)
{
    if(!filling || dex>0)
    {
        //Just to have a measure of e(t(k)-)
        offset=rootFrame2localAbsolute(globalTime-this->globalTime);
        offset=(int)localTime-offset;
    }
    
    if(this->localTime>localTime) overflowCounterLocal++;
    if(this->globalTime>globalTime) overflowCounterGlobal++;
    this->globalTime=globalTime;
    this->localTime=localTime;
    
    unsigned long long ovr_local_rtc_base=reg_local_rtcs[dex];
    unsigned long long temp=overflowCounterLocal;
    temp<<=32;
    temp|=localTime;
    reg_local_rtcs[dex]=temp;
    reg_rtc_offs[dex]=(int)localTime-(int)globalTime;
    if(filling && dex==0) local_rtc_base=temp;
    if(!filling) local_rtc_base=ovr_local_rtc_base;
    if(filling) num_reg_data=dex+1;
    else num_reg_data=regression_entries;
    dex++;
    if(filling && dex>=regression_entries) filling=false;
    if(dex>=regression_entries) dex=0;
    
    if(num_reg_data<2)
    {
        a=(double)localTime-(double)globalTime;
        b=0;
        printf("+ b=%e a=%f localTime=%u globalTime=%u\n",b,a,localTime,globalTime);
    }
}

pair<int,int> FTSP::computeCorrection(int e)
{
    this->e=e;
    
    if(num_reg_data<2) return make_pair(e,w);
    
    long long sum_t=0;
    long long sum_o=0;
    long long sum_ot=0;
    long long sum_t2=0;
    
    for(int i=0;i<num_reg_data;i++)
    {
        long long t=reg_local_rtcs[i]-local_rtc_base;
        long long o=reg_rtc_offs[i];
        sum_t+=t;
        sum_o+=o;
        sum_ot+=o*t;
        sum_t2+=t*t;
    }
    long long n=num_reg_data;
    b=(double)(n*sum_ot-sum_o*sum_t)/(double)(n*sum_t2-sum_t*sum_t);
    a=((double)sum_o-b*(double)sum_t)/(double)n;
    printf("b=%e a=%f localTime=%u globalTime=%u\n",b,a,localTime,globalTime);
    
    return make_pair(e,w);
}

pair<int,int> FTSP::lostPacket()
{
    return make_pair(e,w);
}

void FTSP::reset()
{
    e=offset=0;
    
    dex=0;
    filling=true;
    memset(reg_local_rtcs,0,sizeof(reg_local_rtcs));
    memset(reg_rtc_offs,0,sizeof(reg_rtc_offs));
}

unsigned int FTSP::rootFrame2localAbsolute(unsigned int time) const
{
    unsigned long long temp=overflowCounterGlobal;
    temp<<=32;
    temp|=globalTime;
    temp+=time;
    double global=temp;
    //printf("timeBefore=%u ",temp);
    
//     // local=(global+a-b*local_rtc_base)/(1-b)
//     double lr=local_rtc_base;
//     unsigned long long result=(global+a-b*lr)/(1.0-b);
    
    // local=(global+lastOffset-b*lastSyncPoint)/(1.0-b);
    double lastOffset=(int)localTime-(int)globalTime;
    unsigned long long temp2=overflowCounterLocal;
    temp2<<=32;
    temp2|=localTime;
    double lastSyncPoint=temp2;
    unsigned long long result=(global+lastOffset-b*lastSyncPoint)/(1.0-b);
    
    //printf(" timeAfter=%u\n",result);
    return result & 0xffffffff;
}

//
// class FBS
//

FBS::FBS(Timer& timer) : timer(timer) { reset(); }

void FBS::timestamps(unsigned int globalTime, unsigned int localTime)
{
    timer.setValue(globalTime+overwriteClockTime);
    if(!first) offset=(int)localTime-(int)globalTime-u;
    first=false;
}

std::pair<int,int> FBS::computeCorrection(int e)
{
    u=u+kp*(float)offset+(ki-kp)*(float)offseto;
    offseto=offset;
    printf("FBS offset=%d u=%d\n",offset,(int)u);
    return make_pair(u,w);
}

std::pair<int,int> FBS::lostPacket()
{
    return make_pair(u,w);
}

void FBS::reset()
{
    offset=offseto=0;
    u=0.0f;
    first=true;
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
    #ifndef SEND_TIMESTAMPS
    correction*=sync.getClockCorrection()-sync.getSyncError();
    #else //SEND_TIMESTAMPS
    if(sync.overwritesHardwareClock()) correction*=sync.getClockCorrection();
    else correction*=sync.getClockCorrection()-sync.getSyncError();
    #endif //SEND_TIMESTAMPS
    int sign=correction>=0 ? 1 : -1; //Round towards closest
    int dividedCorrection=(correction+(sign*period/2))/period;
    #ifndef SEND_TIMESTAMPS
    return flood.getMeasuredFrameStart()+max(0,signedRoot+dividedCorrection);
    #else //SEND_TIMESTAMPS
    const FTSP *ftsp=dynamic_cast<const FTSP*>(&sync);
    if(ftsp) return ftsp->rootFrame2localAbsolute(root);
    
    return (sync.overwritesHardwareClock() ?
        flood.getRadioTimestamp() : flood.getMeasuredFrameStart()) +
        max(0,signedRoot+dividedCorrection);
    #endif //SEND_TIMESTAMPS
}
