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
#include "flopsync_v3/protocol_constants.h"
#include "flopsync_v3/flooder_sync_node.h"
#include "flopsync_v3/synchronizer.h"
#include "flopsync_v3/optimized_ramp_flopsync2.h"
#include "flopsync_v3/clock.h"
#include "flopsync_v3/monotonic_clock.h"
#include "flopsync_v3/non_monotonic_clock.h"
#include "flopsync_v3/critical_section.h"
#include "low_power_setup.h"

using namespace std;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;

int identifyNode()
{
    if(strstr(experimentName,"node0")) return 0;
    if(strstr(experimentName,"node1")) return 1;
    if(strstr(experimentName,"node2")) return 2;
    return 3;
}

int main()
{
    lowPowerSetup();
    
    //miosix::Gpio<GPIOC_BASE,0>::mode(miosix::Mode::INPUT_ANALOG);  

    
    blueLed::mode(miosix::Mode::INPUT);  //FIXME
    puts(experimentName);
    Cc2520& transceiver=Cc2520::instance();
    transceiver.setFrequency(2450);
    const int node=identifyNode();
    #ifndef USE_VHT
    Timer& timer=Rtc::instance();
    #else //USE_VHT
    Timer& timer=VHT::instance();
    #endif //USE_VHT
    Synchronizer *sync;
    bool monotonic=false;
    //For multi hop experiments
    sync=new OptimizedRampFlopsync2; 
    monotonic=true;
//     //For comparison between sincnrfhronization schemes
//     switch(node)
//     {
//         case 1: sync=new OptimizedRampFlopsync2; monotonic=true; break;
//         case 2: sync=new FBS(rtc); break;
//         case 3: sync=new FTSP; break;
//     }
    #ifndef MULTI_HOP
    FlooderSyncNode flooder(timer,*sync);
    #else //MULTI_HOP
    FlooderSyncNode flooder(timer,*sync,node-1);
    #endif //MULTI_HOP

    Clock *clock;
    if(monotonic) clock=new MonotonicClock(*sync,flooder);
    else clock=new NonMonotonicClock(*sync,flooder);
    
    for(;;)
    {
        if(flooder.synchronize()) flooder.resynchronize();
        
//        unsigned int start=node*combSpacing;
//        for(unsigned int i=start;i<nominalPeriod-combSpacing/2;i+=3*combSpacing)
//        {   
//            #ifdef SENSE_TEMPERATURE
//            unsigned short temperature=getDACTemperature();
//            #endif //SENSE_TEMPERATURE
//            
//            unsigned int wakeupTime=clock->rootFrame2localAbsolute(i)-
//                (jitterAbsorption+receiverTurnOn+preamblePacketTime);
//            timer.setAbsoluteWakeupSleep(wakeupTime);
//            timer.sleep();
//            blueLed::high();
//            timer.setAbsoluteTriggerEvent(wakeupTime+jitterAbsorption);
//            transceiver.setMode(Cc2520::TX);
//            transceiver.setAutoFCS(true);
//            timer.setAbsoluteTimeout(0);
//            Packet packet;
//            packet.e=sync->getSyncError();
//            packet.u=max<int>(numeric_limits<short>::min(),
//                min<int>(numeric_limits<short>::max(),
//                sync->getClockCorrection()));
//            packet.w=sync->getReceiverWindow();
//                  
//            #ifndef SENSE_TEMPERATURE
//            packet.miss=flooder.isPacketMissed() ? 1 : 0;
//            packet.check=0;
//            #else //SENSE_TEMPERATURE
//            packet.miss=temperature & 0xff;
//            packet.check=(temperature>>8) | 0x10;
//            #endif //SENSE_TEMPERATURE
//            unsigned char *data=reinterpret_cast<unsigned char*>(&packet);
//            transceiver.writeFrame(data);
//            timer.wait();
//            while(!transceiver.isTxFrameDone()); //FIXME
//            blueLed::low();
//            transceiver.setMode(Cc2520::DEEP_SLEEP);
//        }
    }
}
