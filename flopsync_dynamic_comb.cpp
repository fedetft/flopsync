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
#include <miosix.h>
#include "drivers/nrf24l01.h"
#include "drivers/rtc.h"
#include "protocol_constants.h"
#include "flopsync2.h"
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
    blueLed::mode(miosix::Mode::OUTPUT);
    puts(experimentName);
    const unsigned char address[]={0xab, 0xcd, 0xef};
    Rtc& rtc=Rtc::instance();
    Nrf24l01& nrf=Nrf24l01::instance();
    nrf.setAddress(address);
    nrf.setFrequency(2450);
    const int node=identifyNode();
    OptimizedFlopsync flopsync;
    #ifndef MULTI_HOP
    FlooderSyncNode flooder(rtc,flopsync);
    #else //SECOND_HOP
    FlooderSyncNode flooder(rtc,flopsync,node-1);
    #endif //SECOND_HOP
    
    AuxiliaryTimer& timer=AuxiliaryTimer::instance();
    
    for(;;)
    {
        if(flooder.synchronize()) flooder.resynchronize();
        
        MonotonicClock clock(flopsync,flooder);
        unsigned int start=node*combSpacing;
        for(unsigned int i=start;i<nominalPeriod-combSpacing/2;i+=3*combSpacing)
        {
            unsigned int wakeupTime=clock.rootFrame2localAbsolute(i)-
                (jitterAbsorption+receiverTurnOn+packetTime+spiPktSend);
            rtc.setAbsoluteWakeupSleep(wakeupTime);
            rtc.sleep();
            blueLed::high();
            rtc.setAbsoluteWakeupWait(wakeupTime+jitterAbsorption);
            nrf.setMode(Nrf24l01::TX);
            nrf.setPacketLength(4);
            timer.initTimeoutTimer(0);
            signed char packet[]=
            {
                flopsync.getSyncError(),
                flopsync.getClockCorrection(),
                flopsync.getReceiverWindow(),
                flooder.isPacketMissed() ? 0x01 : 0x00
            };
            rtc.wait();
            nrf.writePacket(packet);
            timer.waitForPacketOrTimeout();
            blueLed::low();
            nrf.endWritePacket();
            nrf.setMode(Nrf24l01::SLEEP);
        }
    }
}
