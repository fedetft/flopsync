 
/***************************************************************************
 *   Copyright (C)  2013 by Terraneo Federico                              *
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
#include "flooder_root_node.h"

#include "critical_section.h"
#include <cassert>

using namespace std;

//
// class FlooderRootNode
//

FlooderRootNode::FlooderRootNode(Timer& rtc) : rtc(rtc),
        tr(Transceiver::instance()), timer(AuxiliaryTimer::instance()),
        frameStart(0), wakeupTime(rtc.getValue()+nominalPeriod) {}

bool FlooderRootNode::synchronize()
{
    assert(static_cast<int>(rtc.getValue()-wakeupTime)<0);
    rtc.setAbsoluteWakeupSleep(wakeupTime);
    rtc.sleep();
    rtc.setAbsoluteWakeupWait(wakeupTime+jitterAbsorption);
    tr.setMode(Transceiver::TX);
    #ifndef SEND_TIMESTAMPS
    tr.setPacketLength(1);
    const char packet[]={0x00};
    #else //SEND_TIMESTAMPS
    tr.setPacketLength(4);
    unsigned int timestamp;
    unsigned int *packet=&timestamp;
    #endif //SEND_TIMESTAMPS
    timer.initTimeoutTimer(0);
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    {
        CriticalSection cs;
        rtc.wait();
        miosix::ledOn();
        #ifdef SEND_TIMESTAMPS
        timestamp=rtc.getValue();
        #endif //SEND_TIMESTAMPS
        tr.writePacket(packet);
        //while(tr.irq()==false) ;
        timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU TODO: fix its
        frameStart=rtc.getPacketTimestamp();
        miosix::ledOff(); //Falling edge signals synchronization packet sent
    }
    tr.endWritePacket();
    tr.setMode(Transceiver::SLEEP);
    wakeupTime+=nominalPeriod;
    return false; //Root node does not desynchronize
}