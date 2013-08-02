/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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
#include <memory>
#include <miosix.h>
#include "drivers/nrf24l01.h"
#include "drivers/rtc.h"
#include "protocol_constants.h"
#include "flopsync_controllers.h"

using namespace std;

static const char expectedPacket[]={0x01, 0x02, 0x03, 0x04};

void keepSync(Nrf24l01& nrf, Rtc& rtc, AuxiliaryTimer& timer, unsigned int t)
{
    int u=0;
    #ifdef RELATIVE_CLOCK
    auto_ptr<Controller> controller(new Controller3);
    // Not considering pllBoot as the t=rtc.getValue() at the top of the
    // for loop pulls it out of the time count
    const unsigned int setPoint=radioBoot+w;
    rtc.setRelativeWakeup(nominalPeriod-setPoint);
    #else //RELATIVE_CLOCK
    auto_ptr<Controller> controller(new Controller4);
    const unsigned int setPoint=pllBoot+radioBoot+w;
    t+=nominalPeriod-setPoint;
    rtc.setAbsoluteWakeup(t);
    #endif //RELATIVE_CLOCK
    for(;;)
    {
        rtc.sleepAndWait();
        #ifdef RELATIVE_CLOCK
        rtc.setRelativeWakeup(nominalPeriod+u);
        t=rtc.getValue();
        #endif //RELATIVE_CLOCK
        miosix::ledOn();
        nrf.setMode(Nrf24l01::RX);
        nrf.startReceiving();
        timer.initTimeoutTimer(2*w);
        bool timeout;
        unsigned int timestamp;
        for(;;)
        {
            timeout=timer.waitForPacketOrTimeout();
            if(timeout) break;
            char packet[4];
            nrf.readPacket(packet);
            timestamp=rtc.getValue();
            if(memcmp(packet,expectedPacket,4)==0) break;
        }
        timer.stopTimeoutTimer();
        // Falling edge signals RX received the packet.
        // It happens consistently 250us after the transmitter's falling edge
        // (measured with an oscilloscope)
        miosix::ledOff();
        nrf.setMode(Nrf24l01::SLEEP);
        if(timeout) return;
        int ctr=timestamp-t;
        int e=ctr-setPoint;
        u=controller->run(e);
        printf("ctr=%d e=%d u=%d\n",ctr,e,u);

        #ifndef RELATIVE_CLOCK
        t+=nominalPeriod+u;
        rtc.setAbsoluteWakeup(t);
        #endif //RELATIVE_CLOCK
    }
}

int main()
{
    puts(experimentName);
    const unsigned char address[]={0xab, 0xcd, 0xef};
    Nrf24l01& nrf=Nrf24l01::instance();
    Rtc& rtc=Rtc::instance();
    AuxiliaryTimer& timer=AuxiliaryTimer::instance();
    nrf.setAddress(address);
    nrf.setFrequency(2450);
    nrf.setPacketLength(4);
    
    for(;;)
    {
        nrf.setMode(Nrf24l01::RX);
        timer.initTimeoutTimer(0);
        nrf.startReceiving();
        timer.waitForPacketOrTimeout();
        char packet[4];
        nrf.readPacket(packet);
        unsigned int t=rtc.getValue();
        if(memcmp(packet,expectedPacket,4)!=0) continue;
        nrf.setMode(Nrf24l01::SLEEP);
        keepSync(nrf,rtc,timer,t);
        printf("Lost sync\n");
    }
}
