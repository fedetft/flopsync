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
#include <cstring>
#include <limits>
#include <miosix.h>
#include "drivers/cc2520.h"
#include "drivers/timer.h"
#include "drivers/temperature.h"
#include "drivers/leds.h"
#include "flopsync_v3/protocol_constants.h"
#include "flopsync_v3/flooder_sync_node.h"
#include "flopsync_v3/synchronizer.h"
#include "flopsync_v3/flopsync2.h"
#include "flopsync_v3/controller_flopsync.h"
#include "flopsync_v3/fbs.h"
#include "flopsync_v3/ftsp.h"
#include "flopsync_v3/clock.h"
#include "flopsync_v3/monotonic_clock.h"
#include "flopsync_v3/non_monotonic_clock.h"
#include "flopsync_v3/critical_section.h"
#include "board_setup.h"
#include <cassert>
#define numb_nodes 9

using namespace std;

// #include "currentsensor.h"

int identifyNode()
{
    if(strstr(experimentName,"node0")) return 0;
    if(strstr(experimentName,"node1")) return 1;
    if(strstr(experimentName,"node2")) return 2;
    if(strstr(experimentName,"node3")) return 3;
    if(strstr(experimentName,"node4")) return 4;
    if(strstr(experimentName,"node5")) return 5;
    if(strstr(experimentName,"node6")) return 6;
    if(strstr(experimentName,"node7")) return 7;
    if(strstr(experimentName,"node8")) return 8;
    return 9;
}

int main()
{
    lowPowerSetup();
    
    led2::mode(miosix::Mode::OUTPUT);
    
    //HIPEAC+EWSN demo stuff -- begin
//     miosix::currentSense::enable::high();
//     initAdc();
    //HIPEAC+EWSN demo stuff -- end
    
    puts(experimentName);
    Cc2520& transceiver=Cc2520::instance();
    transceiver.setFrequency(2450);
    #ifndef USE_VHT
    Timer& timer=Rtc::instance();
    #else //USE_VHT
    Timer& timer=VHT::instance();
    #endif //USE_VHT
    Synchronizer *sync;
    bool monotonic=false;
    //For comparison between sincronization schemes
    switch(controller)
    {
     case 1: 
         sync=new FLOPSYNC2; //The new FLOPSYNC, FLOPSYNC 2
         monotonic=true; 
         break;
     case 2: 
         sync=new FBS(timer); 
         break;
     case 3: 
         sync=new FTSP; 
         break;
     case 4:
         sync=new ControllerFlopsync; //The old FLOPSYNC, FLOPSYNC 1
         monotonic=true; 
         break;
     default:
         printf("Unknown controller");
         return 0;
    }
    #ifndef MULTI_HOP
    FlooderSyncNode flooder(timer,*sync);
    #elif defined(GLOSSY)
    FlooderSyncNode flooder(timer,*sync);
    #else
    FlooderSyncNode flooder(timer,*sync,node_hop);
    #endif//MULTI_HOP

    Clock *clock;
    if(monotonic) clock=new MonotonicClock(*sync,flooder);
    else clock=new NonMonotonicClock(*sync,flooder);
    
    for(;;)
    {
        if(flooder.synchronize()) flooder.resynchronize();
        
        #ifdef COMB
        
        #ifndef SYNC_BY_WIRE
        unsigned long long start=identifyNode()*combSpacing;
        for(unsigned long long i=start;i<nominalPeriod-combSpacing/2;i+=numb_nodes*combSpacing)
        {   
            #ifdef SENSE_TEMPERATURE
            unsigned short temperature=getADCTemperature();
            #endif //SENSE_TEMPERATURE
            
            unsigned long long wakeupTime=clock->localTime(i)-
                (jitterAbsorption+txTurnaroundTime+trasmissionTime);
            unsigned long long frameStart=wakeupTime+jitterAbsorption+txTurnaroundTime+trasmissionTime;
            timer.absoluteSleep(wakeupTime);
            led2::high();
            transceiver.setAutoFCS(true);
            transceiver.setMode(Cc2520::TX);
    
            Packet packet;
            packet.e=sync->getSyncError();
            packet.u=sync->getClockCorrection();
            packet.w=sync->getReceiverWindow();
                  
            #ifndef SENSE_TEMPERATURE
            packet.miss=flooder.isPacketMissed() ? 1 : 0;
            packet.check=0;
            #else //SENSE_TEMPERATURE
            packet.miss=temperature & 0xff;
            packet.check=(temperature>>8) | 0x10;
            #endif //SENSE_TEMPERATURE
            unsigned char len=sizeof(Packet);
            unsigned char *data=reinterpret_cast<unsigned char*>(&packet);
            transceiver.writeFrame(len,data);
            #if FLOPSYNC_DEBUG  >0
            assert(timer.getValue()<frameStart-txTurnaroundTime-trasmissionTime);
            #endif//FLOPSYNC_DEBUG
            timer.absoluteWaitTrigger(frameStart-txTurnaroundTime-trasmissionTime);
            timer.absoluteWaitTimeoutOrEvent(frameStart-trasmissionTime+preambleFrameTime+delaySendPacketTime);
            transceiver.isSFDRaised();
            timer.absoluteWaitTimeoutOrEvent(frameStart-trasmissionTime+packetTime+delaySendPacketTime);
            transceiver.isTxFrameDone();
            led2::low();
            transceiver.setMode(Cc2520::DEEP_SLEEP);
        }
        #else//SYNC_BY_WIRE
        unsigned long long start=identifyNode()*combSpacing;
        unsigned int j=0;
        for(unsigned long long i=start;i<nominalPeriod-combSpacing/2;i+=combSpacing)
        {   
            unsigned long long wakeupTime=clock->localTime(i)-jitterAbsorption-j*w;
            unsigned long long frameStart=wakeupTime+jitterAbsorption+j*w;
            timer.absoluteSleep(wakeupTime);
            led2::high();
            timer.absoluteWaitTrigger(frameStart);
            led2::low();
        }
        #endif//SYNC_BY_WIRE
        #endif//COMB
    }
}
