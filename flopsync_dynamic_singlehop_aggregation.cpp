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
#include "variance_estimator.h"

using namespace std;

static const char expectedPacket[]={0x01, 0x02, 0x03, 0x04};
Packet packet;
void keepSync(Nrf24l01& nrf, Rtc& rtc, AuxiliaryTimer& timer, unsigned int t)
{
    int missPkts=0;
    bool miss=false;
    int u=0;
    VarianceEstimator ve;
    int dwo=w;
    int dwoo=dwo;
    #ifdef RELATIVE_CLOCK
    auto_ptr<Controller> controller(new Controller3);
    rtc.setRelativeWakeup(nominalPeriod-(radioBoot+receiverTurnOn+dwo+packetTime));
    #else //RELATIVE_CLOCK
    auto_ptr<Controller> controller(new Controller4);
    t+=nominalPeriod-(jitterAbsorption+receiverTurnOn+dwo+packetTime);
    rtc.setAbsoluteWakeup(t);
    #endif //RELATIVE_CLOCK
    for(;;)
    {
        rtc.sleepAndWait();
        rtc.setAbsoluteWakeup(t+jitterAbsorption);
        miosix::ledOn();
        nrf.setMode(Nrf24l01::RX);
        rtc.wait();
        nrf.startReceiving();
        timer.initTimeoutTimer(2*w+receiverTurnOn+packetTime);
        bool timeout;
        unsigned int timestamp=0;
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
        if(timeout)
        {
       	    packet.misspackets++;    
            if(++missPkts>5) 
            {
              packet.unsynctime++;
	      return;
            }

            miss=true;
            ve.setWindow(2*dwo);
        } else missPkts=0;
        #ifdef RELATIVE_CLOCK
        // Not considering pllBoot as the t=rtc.getValue() at the top of the
        // for loop pulls it out of the time count
        int dsetPoint=radioBoot+receiverTurnOn+dwoo+packetTime;
        #else //RELATIVE_CLOCK
        int dsetPoint=jitterAbsorption+receiverTurnOn+dwo+packetTime;
        #endif //RELATIVE_CLOCK
        int ctr=timestamp-t;
        int e=ctr-dsetPoint;
        if(miss==false)
        {
            u=controller->run(e);
            ve.update(e);
        } else {
            ctr=0;
            e=0;
            timestamp=t+dsetPoint;
            //And leave u as the previously computed value
        }

        dwoo=dwo;
        dwo=max<int>(min<int>(ve.window(),w),minw);

        printf("ctr=%d e=%d u=%d w=%d%s\n",ctr,e,u,dwo,miss ? " (miss)":"");
        miss=false;

	#ifdef SECOND_HOP
	packet.e = e;
	packet.u = u;
	packet.w = dwoo;
	packet.sequence++;

	
	unsigned int sendPacket=t+txtime+(u/2)+dwoo;
        rtc.setAbsoluteWakeup(sendPacket);
	rtc.sleepAndWait();
        rtc.setAbsoluteWakeup(sendPacket+jitterAbsorption);
	nrf.setPacketLength(sizeof(Packet));
        nrf.setMode(Nrf24l01::TX);
        timer.initTimeoutTimer(0);
        rtc.wait();
	nrf.writePacket(reinterpret_cast<void*>(&packet));
	
        timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU
        nrf.endWritePacket();
        nrf.setMode(Nrf24l01::SLEEP);
	nrf.setPacketLength(4);
	#endif //SECOND_HOP
	
	t+=nominalPeriod+u-(dwo-dwoo);
        rtc.setAbsoluteWakeup(t);
    }
}

int main()
{
    puts(experimentName);
    packet.e = 0;
    packet.u = 0;
    packet.w = 0;
    packet.misspackets = 0;
    packet.unsynctime = 0;
    packet.sequence =0;
    packet.check = 0xabc;
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
