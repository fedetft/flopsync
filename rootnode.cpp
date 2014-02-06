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
#include "low_power_setup.h"

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
   
    for(;;)
    {
        flooder.synchronize();
        
        puts("----");
        unsigned int frameStart=flooder.getMeasuredFrameStart();
        for(unsigned int i=combSpacing,j=0;i<nominalPeriod-combSpacing/2;i+=combSpacing,j++)
        {
            unsigned int wakeupTime=frameStart+i-
                (jitterAbsorption+receiverTurnOn+w+longPacketTime);
            timer.setAbsoluteWakeupSleep(wakeupTime);
            timer.sleep();
            timer.setAbsoluteWakeupWait(wakeupTime+jitterAbsorption);
            transceiver.setMode(Transceiver::RX);
            timer.wait();
            timer.setAbsoluteTimeout(receiverTurnOn+2*w+longPacketTime);
            bool timeout;
            unsigned int measuredTime;
            Packet2 packet;
            for(;;)
            {
                timeout=timer.waitForExtEventOrTimeout();
                measuredTime=timer.getExtEventTimestamp();
                if(timeout) break;
                while(!transceiver.isRxFrameDone()); //FIXME
                transceiver.readFrame(&packet);
                if(packet.check==0 || (packet.check & 0xf0)==0x10) break;
            }
            transceiver.setMode(Cc2520::DEEP_SLEEP);
            if(timeout) printf("node%d timeout\n",(j % 3)+1);
            else {
                if(j<3) printf("e=%d u=%d w=%d%s\n",packet.e,packet.u,packet.w,
                    (packet.miss && packet.check==0) ? "(miss)" : "");
                printf("node%d.e=%d",(j % 3)+1,measuredTime-(frameStart+i));
                if(packet.check==0) printf("\n");
                else {
                    unsigned short temperature=packet.check & 0x0f;
                    temperature<<=8;
                    temperature|=packet.miss;
                    
                    printf(" t=%d\n",temperature);
                }
            }
        }
    }
}