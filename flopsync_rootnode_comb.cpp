/***************************************************************************
 *   Copyright (C) 2012, 2013 by Terraneo Federico                         *
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
#include <miosix.h>
#include "drivers/nrf24l01.h"
#include "drivers/rtc.h"
#include "protocol_constants.h"

using namespace std;

int main()
{
    puts(experimentName);
    const unsigned char address[]={0xab, 0xcd, 0xef};
    static const char packet[]={0x01, 0x02, 0x03, 0x04};
    Rtc& rtc=Rtc::instance();
    Nrf24l01& nrf=Nrf24l01::instance();
    AuxiliaryTimer& timer=AuxiliaryTimer::instance();
    nrf.setAddress(address);
    nrf.setFrequency(2450);
    nrf.setPacketLength(4);
    unsigned int nextWakeup=nominalPeriod;
    for(;;)
    {
        rtc.setAbsoluteWakeup(nextWakeup);
        rtc.sleepAndWait();
        rtc.setAbsoluteWakeup(nextWakeup+jitterAbsorption);
        nrf.setMode(Nrf24l01::TX);
        timer.initTimeoutTimer(0);
        // To minimize jitter in the packet transmission time caused by the
        // variable time sleepAndWait() takes to restart the STM32 PLL an
        // additional wait is done here to absorb the jitter.
        rtc.wait();
        miosix::ledOn();
        nrf.writePacket(packet);
        timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU
        unsigned int timestamp=rtc.getValue();
        miosix::ledOff(); //Falling edge signals synchronization packet sent
        nrf.endWritePacket();
        nrf.setMode(Nrf24l01::SLEEP);
        
        printf("delta=%d\n",timestamp-nextWakeup);
        
        nextWakeup+=nominalPeriod;
    }
    return 0;
}
