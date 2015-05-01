/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

#include <cstdio>
#include "drivers/cc2520.h"
#include "drivers/timer.h"
#include "drivers/frame.h"
#include "flopsync_v3/protocol_constants.h"
#include "flopsync_v3/flooder_root_node.h"
#include "flopsync_v3/synchronizer.h"
#include "flopsync_v3/rtt_measure.h"
#include "drivers/BarraLed.h"
#include "board_setup.h"
#include <cassert>

#define numb_nodes 9
using namespace std;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;

int main()
{
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    puts(experimentName);
    Cc2520& transceiver=Cc2520::instance();
    transceiver.setFrequency(2450);
    #ifndef USE_VHT
    Timer& timer=Rtc::instance();
    #else //USE_VHT
    Timer& timer=VHT::instance();
    #endif //USE_VHT
    FlooderRootNode flooder(timer);

    RttMeasure *measure = new RttMeasure(0, transceiver, timer); //this node is the tree's root node, so hopCount is zero.
    
    for(;;)
    {
        flooder.synchronize();
        #if TIMER_DEBUG>0
        puts("----");
        #endif//TIMER_DEBUG
        
        unsigned long long frameStart=flooder.getMeasuredFrameStart(); //removed combSpacing, the master node acts as RTT server immediately after synchronizing
        unsigned long long wakeupTime=frameStart-(jitterAbsorption+rxTurnaroundTime+w);
        timer.absoluteSleep(wakeupTime);
        
        blueLed::high();
        dynamic_cast<VHT&>(timer).disableAutoSyncWithRtc();
        
        measure->rttServer(frameStart, 0); //since this node is at tree's root its cumulated propagation delay is zero.
        
        blueLed::low();
        transceiver.setMode(Cc2520::DEEP_SLEEP);
        dynamic_cast<VHT&>(timer).enableAutoSyncWhitRtc();
                
        #ifdef COMB
        
        #ifndef SYNC_BY_WIRE
//        timer.absoluteSleep(flooder.getMeasuredFrameStart()+nominalPeriod/2 -jitterAbsorption);
//        timer.absoluteWaitTrigger(flooder.getMeasuredFrameStart()+nominalPeriod/2);
        unsigned long long frameStart=flooder.getMeasuredFrameStart();
        unsigned int j=0;
        for(unsigned long long i=combSpacing; i<nominalPeriod-combSpacing/2;i+=combSpacing,j++)
        {
            unsigned long long wakeupTime=frameStart+i-
                (jitterAbsorption+rxTurnaroundTime+w);
            timer.absoluteSleep(wakeupTime);
            greenLed::high();
            transceiver.setMode(Cc2520::IDLE);
            #if FLOPSYNC_DEBUG > 0   
            assert(timer.getValue()<wakeupTime+jitterAbsorption);
            #endif//FLOPSYNC_DEBUG
            timer.absoluteWait(wakeupTime+jitterAbsorption);
            transceiver.setAutoFCS(true);
            transceiver.setMode(Cc2520::RX);
            bool timeout;
            unsigned long long measuredTime;
            Packet packet;
            for(;;)
            {
                timeout=timer.absoluteWaitTimeoutOrEvent(frameStart+i+w+preambleFrameTime);
                measuredTime=timer.getExtEventTimestamp()-preambleFrameTime;
                if(timeout) break;
                transceiver.isSFDRaised();
                timer.absoluteWaitTimeoutOrEvent(measuredTime+packetTime+delaySendPacketTime);
                transceiver.isRxFrameDone();
                unsigned char len=sizeof(Packet);
                unsigned char *data=reinterpret_cast<unsigned char*>(&packet);
                if(transceiver.readFrame(len,data)!=1){
                    timeout=true;
                    break;
                }
                if(packet.check==0 || (packet.check & 0xf0)==0x10) break;
            }
            greenLed::low();
            transceiver.setMode(Cc2520::DEEP_SLEEP);
            if(timeout) printf("node%d timeout\n",(j % numb_nodes)+1);
            else {
                if(j<numb_nodes)
                { 
                    printf("e=%d u=%d w=%d%s\n",packet.e,packet.u,packet.w,
                    (packet.miss && packet.check==0) ? "(miss)" : "");
                }
                int e=frameStart+i-measuredTime;
                printf("node%d.e=%d",(j % numb_nodes)+1,e);
                if(packet.check==0) printf("\n");
                else {
                    unsigned short temperature=packet.check & 0x0f;
                    temperature<<=8;
                    temperature|=packet.miss;
                    
                    printf(" t=%d\n",temperature);
                }
            }
        }
        #else//SYNC_BY_WIRE
        unsigned long long frameStart=flooder.getMeasuredFrameStart();
        unsigned int j=0;
        for(unsigned long long i=combSpacing; i<nominalPeriod-combSpacing/2;i+=combSpacing,j++)
        {
            unsigned long long wakeupTime=frameStart+i-(jitterAbsorption+w);
            timer.absoluteSleep(wakeupTime);
            greenLed::high();
            bool timeout;
            unsigned long long measuredTime;
            Packet packet;
            for(;;)
            {
                timeout=timer.absoluteWaitTimeoutOrEvent(frameStart+i+w);
                measuredTime=timer.getExtEventTimestamp();
                break;
            }
            greenLed::low();
            if(timeout) 
                printf("node1 timeout\n");
            else 
            {
                int e=frameStart+i-measuredTime;
                printf("node1.e=%d\n",e);
            }
        }
    
        #endif//SYNC_BY_WIRE
        #endif//COMB
    }
}
