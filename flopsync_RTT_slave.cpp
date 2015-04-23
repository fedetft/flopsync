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
#include <limits>
#include <miosix.h>
#include "drivers/cc2520.h"
#include "drivers/timer.h"
#include "drivers/temperature.h"
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
#include "drivers/BarraLed.h"
#include <cassert>
#define numb_nodes 9

using namespace std;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;

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
    Synchronizer *sync;
    bool monotonic=false;
    //For comparison between sincnrfhronization schemes
    switch(controller)
    {
     case 1:
         sync=new FLOPSYNC2;
         monotonic=true;
         break;
     case 2:
         sync=new FBS(timer);
         break;
     case 3:
         sync=new FTSP;
         break;
    }
    #ifndef MULTI_HOP
    FlooderSyncNode flooder(timer,*sync);
    #elif defined(GLOSSY)
    FlooderSyncNode flooder(timer,*sync);
    #else
    FlooderSyncNode flooder(timer,*sync,node_hop);
    #endif//MULTI_HOP

    Clock *clock;
    if(monotonic) clock=new MonotonicClock(*sync,flooder);
    else clock=new NonMonotonicClock(*sync,flooder);

    for(;;)
    {
        if(flooder.synchronize()) flooder.resynchronize();
              
        unsigned long long frameStart = clock->localTime(combSpacing);
        unsigned long long wakeupTime = frameStart - (jitterAbsorption+txTurnaroundTime);

        timer.absoluteSleep(wakeupTime);
        blueLed::high();
        
        /** send RTT packet **/
        
        transceiver.setMode(Cc2520::TX);
        transceiver.setAutoFCS(false);

        unsigned char data[] = {0x01,0x02};     //RTT packet payload
        unsigned char len = sizeof(data);      
        
        transceiver.writeFrame(len,data);       //send RTT packet
        timer.absoluteWaitTrigger(frameStart-txTurnaroundTime);
        timer.absoluteWaitTimeoutOrEvent(frameStart+preambleFrameTime+delaySendPacketTime);
        unsigned long long T1 = timer.getExtEventTimestamp();   //get TX SFD timestamp
        transceiver.isSFDRaised();
        timer.absoluteWaitTimeoutOrEvent(frameStart+rttPacketTime+delaySendPacketTime);
        transceiver.isTxFrameDone();
//         transceiver.flushTxFifoBuffer(); // TODO: useful or useless?
//         transceiver.flushRxFifoBuffer(); // TODO: useful or useless?
        
        /** led bar processing **/
        
        packet16 pkt;   //led bar packet
        unsigned long long ledBarPaketTime = static_cast<unsigned long long>(((pkt.getPacketSize()+8)*8*hz)/channelbps+0.5f); //tempo di trasmissione della barra a led; TODO: metterlo a posto!
        
        transceiver.setMode(Cc2520::RX);
        transceiver.setAutoFCS(false);
        bool timeout;
        unsigned long long T2;
        for(;;)
        {
            timeout = timer.absoluteWaitTimeoutOrEvent(frameStart+2*(txTurnaroundTime+preambleFrameTime)+rttRetransmitTime+rttSlackTime);
            T2 = timer.getExtEventTimestamp();
            transceiver.isSFDRaised();
            timer.absoluteWaitTimeoutOrEvent(T2+ledBarPaketTime+delaySendPacketTime);   //TODO: verificare se si può utilizzare T2 come "tempo di inizio"!
            transceiver.isRxFrameDone();
            
            if(timeout)
            {
                iprintf("timeout occurred before T2 SFD is detected\n");
                break;
            }
            
            iprintf("T1 absolute value: %llu\n",T1);
            iprintf("T2 absolute value: %llu\n",T2);
            
            
            len = pkt.getPacketSize();
            int retVal=transceiver.readFrame(len,pkt.getPacket());
            iprintf("retVal: %d\n", retVal);
            if(retVal != 1) break;
            
            miosix::memDump(pkt.getPacket(),len);
            
            std::pair<int, bool> result = pkt.decode();
            
            if(result.second)
            {
                iprintf("result.first = %d, time = %llu\n", result.first, T2 - T1);
                break; 
            }               
        }
         
        blueLed::low();
        transceiver.setMode(Cc2520::DEEP_SLEEP);

        #ifdef COMB

        #ifndef SYNC_BY_WIRE
        unsigned long long start=identifyNode()*combSpacing;
        for(unsigned long long i=start;i<nominalPeriod-combSpacing/2;i+=numb_nodes*combSpacing)
        {
            #ifdef SENSE_TEMPERATURE
            unsigned short temperature=getADCTemperature();
            #endif //SENSE_TEMPERATURE

            unsigned long long wakeupTime=clock->localTime(i)-
                (jitterAbsorption+txTurnaroundTime+trasmissionTime);
            unsigned long long frameStart=wakeupTime+jitterAbsorption+txTurnaroundTime+trasmissionTime;
            timer.absoluteSleep(wakeupTime);
            blueLed::high();
            transceiver.setAutoFCS(true);
            transceiver.setMode(Cc2520::TX);

            Packet packet;
            packet.e=sync->getSyncError();
            packet.u=sync->getClockCorrection();
            packet.w=sync->getReceiverWindow();

            #ifndef SENSE_TEMPERATURE
            packet.miss=flooder.isPacketMissed() ? 1 : 0;
            packet.check=0;
            #else //SENSE_TEMPERATURE
            packet.miss=temperature & 0xff;
            packet.check=(temperature>>8) | 0x10;
            #endif //SENSE_TEMPERATURE
            unsigned char len=sizeof(Packet);
            unsigned char *data=reinterpret_cast<unsigned char*>(&packet);
            transceiver.writeFrame(len,data);
            #if FLOPSYNC_DEBUG  >0
            assert(timer.getValue()<frameStart-txTurnaroundTime-trasmissionTime);
            #endif//FLOPSYNC_DEBUG
            timer.absoluteWaitTrigger(frameStart-txTurnaroundTime-trasmissionTime);
            timer.absoluteWaitTimeoutOrEvent(frameStart-trasmissionTime+preambleFrameTime+delaySendPacketTime);
            transceiver.isSFDRaised();
            timer.absoluteWaitTimeoutOrEvent(frameStart-trasmissionTime+packetTime+delaySendPacketTime);
            transceiver.isTxFrameDone();
            blueLed::low();
            transceiver.setMode(Cc2520::DEEP_SLEEP);
        }
        #else//SYNC_BY_WIRE
        unsigned long long start=identifyNode()*combSpacing;
        unsigned int j=0;
        for(unsigned long long i=start;i<nominalPeriod-combSpacing/2;i+=combSpacing)
        {
            unsigned long long wakeupTime=clock->localTime(i)-jitterAbsorption-j*w;
            unsigned long long frameStart=wakeupTime+jitterAbsorption+j*w;
            timer.absoluteSleep(wakeupTime);
            blueLed::high();
            timer.absoluteWaitTrigger(frameStart);
            blueLed::low();
        }
        #endif//SYNC_BY_WIRE
        #endif//COMB*/
    }
}