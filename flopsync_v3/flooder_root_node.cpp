 
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

FlooderRootNode::FlooderRootNode(Timer& timer) : timer(timer),
        transceiver(Cc2520::instance()),frameStart(0), 
        wakeupTime(timer.getValue()+nominalPeriod) {}

bool FlooderRootNode::synchronize()
{
    #ifdef FLOPSYNC_DEBUG   
    if(wakeupTime<timer.getValue())
        printf("Wakeup is in the past.\n");
    assert(false);
    #endif//FLOPSYNC_DEBUG
    timer.setAbsoluteWakeupSleep(wakeupTime);
    timer.sleep();
    timer.setAbsoluteWakeupWait(wakeupTime+jitterAbsorption);
    transceiver.setMode(Transceiver::TX);
    #ifndef SEND_TIMESTAMPS
    transceiver.setAutoFCS(false);
    const char payload[]={0x00};
    const char fcs[]={0xFF};
    syncFrame.setPayload(payload);
    syncFrame.setFCS(fcs);
    #else //SEND_TIMESTAMPS
    transceiver.setAutoFCS(true);
    unsigned long long timestamp;
    unsigned long long *payload=&timestamp;
    syncFrame.setPayload(payload);
    #endif //SEND_TIMESTAMPS
    timer.setAbsoluteTimeout(0);
    // To minimize jitter in the packet transmission time caused by the
    // variable time sleepAndWait() takes to restart the STM32 PLL an
    // additional wait is done here to absorb the jitter.
    {
        CriticalSection cs;
        timer.wait();
        miosix::ledOn();
        #ifdef SEND_TIMESTAMPS
        timestamp=timer.getValue();
        #endif //SEND_TIMESTAMPS
        transceiver.writeFrame(*syncFrame);
        timer.waitForExtEventOrTimeout(); //Wait for packet sent, sleeping the CPU TODO: fix its
        frameStart=timer.getExtEventTimestamp();
        miosix::ledOff(); //Falling edge signals synchronization packet sent
    }
    transceiver.sendTxFifoFrame();
    while(!transceiver.isTxFrameDone()); //FIXME
    transceiver.setMode(Transceiver::DEEP_SLEEP);
    wakeupTime+=nominalPeriod;
    return false; //Root node does not desynchronize
}