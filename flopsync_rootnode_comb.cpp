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
#include "drivers/nrf24l01.h"
#include "drivers/rtc.h"
#include "protocol_constants.h"
#include "flopsync2.h"

using namespace std;

int main()
{
    puts(experimentName);
    const unsigned char address[]={0xab, 0xcd, 0xef};
    Nrf24l01& nrf=Nrf24l01::instance();
    nrf.setAddress(address);
    nrf.setFrequency(2450);
    FlooderRootNode flooder;
    
    Rtc& rtc=Rtc::instance();
    AuxiliaryTimer& timer=AuxiliaryTimer::instance();
    
    for(;;)
    {
        flooder.synchronize();
        
        unsigned int frameStart=flooder.getFrameStart();
        for(unsigned int i=combSpacing,j=0;i<nominalPeriod-combSpacing/2;i+=combSpacing,j++)
        {
            unsigned int wakeupTime=frameStart+i-
                (jitterAbsorption+receiverTurnOn+w+smallPacketTime);
            rtc.setAbsoluteWakeup(wakeupTime);
            rtc.sleepAndWait();
            rtc.setAbsoluteWakeup(wakeupTime+jitterAbsorption);
            nrf.setMode(Nrf24l01::RX);
            nrf.setPacketLength(1);
            rtc.wait();
            nrf.startReceiving();
            timer.initTimeoutTimer(receiverTurnOn+2*w+smallPacketTime);
            bool timeout;
            unsigned int measuredTime;
            for(;;)
            {
                timeout=timer.waitForPacketOrTimeout();
                measuredTime=rtc.getValue();
                if(timeout) break;
                char packet[1];
                nrf.readPacket(packet);
                if(packet[0]==0xff) break;
            }
            timer.stopTimeoutTimer();
            nrf.setMode(Nrf24l01::SLEEP);
            if(timeout) printf("node%d timeout\n",(j & 1)+1);
            else printf("node%d.e=%d\n",(j & 1)+1,measuredTime-(frameStart+i));
        }
    }
}
