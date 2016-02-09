/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
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

#ifndef DEMO_H
#define DEMO_H

#include "demo_thermocouple.h"
#include "flopsync_v3/protocol_constants.h"
#include "flopsync_v3/clock.h"
#include "drivers/timer.h"
#include "drivers/cc2520.h"

inline void waitAndStartTransceiver(Timer& timer, Cc2520& transceiver,
                                    unsigned long long when, bool increaseVdd=false)
{
    unsigned long long wakeupTime=when-jitterAbsorption;
    timer.absoluteSleep(wakeupTime);
    #if WANDSTEM_HW_REV>12
    if(increaseVdd) miosix::voltageSelect::high();
    #endif
    transceiver.setMode(Cc2520::IDLE);
    timer.absoluteWait(when);
}

typedef bool (*validateFn)(void*);

inline bool getPacketAt(Timer& timer, Cc2520& transceiver, void *packet, int size,
                        validateFn vfn, unsigned long long when, int freq)
{
    const unsigned long long ourPacketTime=((size+8)*8*hz)/channelbps;
    
    waitAndStartTransceiver(timer,transceiver,when-rxTurnaroundTime-w);
    transceiver.setFrequency(freq);
    transceiver.setAutoFCS(true);
    transceiver.setMode(Cc2520::RX);
    
    miosix::redLed::high();
    bool success=false;
    const int maxIter=10;//Another measure to prevent infinite loops
    for(int i=0;i<maxIter;i++)
    {
        if(timer.absoluteWaitTimeoutOrEvent(when+w+preambleFrameTime)) break;
        unsigned long long measuredTime=timer.getExtEventTimestamp()-preambleFrameTime;
        miosix::redLed::low();
        
        //Prevent infinite loop in RX under yet to be investigated circumstances
        if(measuredTime>when+w) break;
        
        miosix::redLed::high();
        transceiver.isSFDRaised();
        timer.absoluteWaitTimeoutOrEvent(measuredTime+ourPacketTime+delaySendPacketTime);
        transceiver.isRxFrameDone();
        unsigned char len=size;
        unsigned char *data=reinterpret_cast<unsigned char*>(packet);
        if(transceiver.readFrame(len,data)!=1) continue;
        if(len!=size) continue; //Too short, not our packet (redundant check?)
        if(vfn(data)==false) continue;
        success=true;
        break;
    }
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    miosix::redLed::low();
    
    return success;
}

inline void sendPacketAt(Timer& timer, Cc2520& transceiver, Clock& clock,
                         void *packet, int size, unsigned long long when, int freq)
{
    unsigned long long wakeupTime=clock.localTime(when)-
        (jitterAbsorption+txTurnaroundTime+trasmissionTime);
    unsigned long long frameStart=wakeupTime+jitterAbsorption+txTurnaroundTime+trasmissionTime;
    timer.absoluteSleep(wakeupTime);
    miosix::redLed::high();
    transceiver.setFrequency(freq);
    transceiver.setAutoFCS(true);
    transceiver.setMode(Cc2520::TX);

    unsigned char len=size;
    unsigned char *data=reinterpret_cast<unsigned char*>(packet);
    transceiver.writeFrame(len,data);

    timer.absoluteWaitTrigger(frameStart-txTurnaroundTime-trasmissionTime);
    timer.absoluteWaitTimeoutOrEvent(frameStart-trasmissionTime+preambleFrameTime+delaySendPacketTime);
    transceiver.isSFDRaised();
    timer.absoluteWaitTimeoutOrEvent(frameStart-trasmissionTime+packetTime+delaySendPacketTime);
    transceiver.isTxFrameDone();
    miosix::redLed::low();
    transceiver.setMode(Cc2520::DEEP_SLEEP);
}

struct DemoPacket
{
    unsigned long long nodeId;
    int flopsyncError;
    short thermocoupleTemperature;
    unsigned short flopsyncMissCount;
};


const int numNodes=2;

const unsigned long long nodeIds[numNodes]=
{
    0x398f892481e54042,
    0x6a4202f50dd15712
};

template<unsigned N>
inline bool validatePacket(void *packet)
{
    return reinterpret_cast<DemoPacket*>(packet)->nodeId==nodeIds[N];
}

validateFn validateByNodeId[numNodes]=
{
    &validatePacket<0>,
    &validatePacket<1>
};

const int sendCount=2; //Each node transmits the same packet sendCount times

const int sendFrequencies[sendCount]=
{
    2450,
    2460
};

const unsigned long long dataPeriod=static_cast<unsigned long long>(5.0f*hz+0.5f);
const unsigned long long initialDelay=dataPeriod/2;
const unsigned long long spacing=static_cast<unsigned long long>(0.2f*hz+0.5f);

#endif //DEMO_H
