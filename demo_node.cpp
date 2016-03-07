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
#include "flopsync_v3/fbs.h"
#include "flopsync_v3/ftsp.h"
#include "flopsync_v3/clock.h"
#include "flopsync_v3/monotonic_clock.h"
#include "flopsync_v3/non_monotonic_clock.h"
#include "flopsync_v3/critical_section.h"
#include "board_setup.h"
#include <cassert>
#include "demo.h"
#define numb_nodes 9

using namespace std;

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

short readThermocouple()
{
    ThermocoupleReader reader;
    miosix::Thread::sleep(150);
    short result=reader.readTemperature(1);
    #if WANDSTEM_HW_REV>12
    miosix::voltageSelect::low(); //Reset voltage to low value
    #endif
    return result;
}

void senseAndSend(Timer& timer, Cc2520& transceiver, Clock& clock,
                  unsigned long long when, DemoPacket& packet)
{
    //The last param is true and means that voltage is increased to 3.1V as soon
    //as we exit deep sleep
    waitAndStartTransceiver(timer,transceiver,clock.localTime(when),true);
    packet.thermocoupleTemperature=readThermocouple(); 
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    
    int id=identifyNode()-1;
    when+=spacing*(1+sendCount*id);
    for(int i=0;i<sendCount;i++)
    {
        sendPacketAt(timer,transceiver,clock,&packet,sizeof(packet),when,sendFrequencies[i]);
        when+=spacing;
    }
}

int main()
{
    lowPowerSetup();
    
    led2::mode(miosix::Mode::OUTPUT);
    puts(experimentName);
    Cc2520& transceiver=Cc2520::instance();
    transceiver.setTxPower(Cc2520::P_5);
    #ifndef USE_VHT
    Timer& timer=Rtc::instance();
    #else //USE_VHT
    Timer& timer=VHT::instance();
    #endif //USE_VHT
    
    Synchronizer *sync=new FLOPSYNC2; 

    #ifndef MULTI_HOP
    FlooderSyncNode flooder(timer,*sync);
    #elif defined(GLOSSY)
    FlooderSyncNode flooder(timer,*sync);
    #else
    FlooderSyncNode flooder(timer,*sync,node_hop);
    #endif//MULTI_HOP

    MonotonicClock clock(*sync,flooder);
    
    unsigned int missedSyncPackets=0;
    for(;;)
    {
        transceiver.setFrequency(2450);
        if(flooder.synchronize()) flooder.resynchronize();
        if(flooder.isPacketMissed()) missedSyncPackets++;
        
        DemoPacket packet;
        packet.flopsyncError=sync->getSyncError();
        packet.flopsyncMissCount=missedSyncPackets;
        packet.nodeId=nodeIds[identifyNode()-1];
        
        for(unsigned long long t=initialDelay;t<nominalPeriod;t+=dataPeriod)
            senseAndSend(timer,transceiver,clock,t,packet);
    }
}
