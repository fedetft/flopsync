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

#include <iostream>
#include "drivers/cc2520.h"
#include "drivers/timer.h"
#include "drivers/frame.h"
#include "drivers/leds.h"
#include "flopsync_v3/protocol_constants.h"
#include "flopsync_v3/flooder_root_node.h"
#include "flopsync_v3/synchronizer.h"
#include "board_setup.h"
#include "demo.h"

using namespace std;

void readSensor(Timer& timer, Cc2520& transceiver, unsigned long long& when, int node)
{
    static int missed[numNodes]={0};
    
    bool alreadyPrinted=false;
    for(int i=0;i<sendCount;i++)
    {
        DemoPacket packet;
        bool success=getPacketAt(timer,transceiver,&packet,sizeof(packet),
                                 validateByNodeId[node],when,sendFrequencies[i]);
        if(!success) missed[node]++;
        when+=spacing;
        
        if(!success || alreadyPrinted) continue;
        unsigned long long timestamp=timer.getValue();
        cout<<packet.nodeId<<' '
            <<timestamp<<' '
            <<decodeTemperature(packet.thermocoupleTemperature)<<' '
            <<missed[node]<<' '
            <<packet.flopsyncError<<' '
            <<packet.flopsyncMissCount<<endl;
        alreadyPrinted=true;
    }
    if(!alreadyPrinted)
    {
        unsigned long long timestamp=timer.getValue();
        cout<<nodeIds[node]<<' '
            <<timestamp<<' '
            <<"NoPacketFromSensor "
            <<missed[node]<<endl;
    }
}

void readAllSensors(Timer& timer, Cc2520& transceiver, unsigned long long when)
{
    when+=spacing; //First time used for sensing, next used for sending
    for(int i=0;i<numNodes;i++)
        readSensor(timer,transceiver,when,i);
    cout<<"end"<<endl;
}

int main()
{
    lowPowerSetup();
    puts(experimentName);

    //HIPEAC+EWSN demo stuff -- begin
//     miosix::currentSense::enable::high();
//     initAdc();
    //HIPEAC+EWSN demo stuff -- end

    Cc2520& transceiver=Cc2520::instance();
    transceiver.setTxPower(Cc2520::P_5);
    #ifndef USE_VHT
    Timer& timer=Rtc::instance();
    #else //USE_VHT
    Timer& timer=VHT::instance();
    #endif //USE_VHT
    FlooderRootNode flooder(timer);
   
    bool removeMe=false;
    for(;;)
    {
        transceiver.setFrequency(2450);
        removeMe^=1;
        if(removeMe) miosix::ledOn(); else miosix::ledOff();
        flooder.synchronize();
        unsigned long long start=flooder.getMeasuredFrameStart();
        for(unsigned long long t=start+initialDelay;t<start+nominalPeriod;t+=dataPeriod)
            readAllSensors(timer,transceiver,t);
    }
}
