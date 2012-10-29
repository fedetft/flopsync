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
#include <cmath>
#include <unistd.h>
#include <pthread.h>
#include <miosix.h>
#include "drivers/nrf24l01.h"
#include "drivers/rtc.h"
#include "protocol_constants.h"

using namespace std;

static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
static unsigned int period=nominalPeriod;

static void* sendThread(void*)
{
    const unsigned char address[]={0xab, 0xcd, 0xef};
    static const char packet[]={0x01, 0x02, 0x03, 0x04};
    Rtc& rtc=Rtc::instance();
    Nrf24l01& nrf=Nrf24l01::instance();
    AuxiliaryTimer& timer=AuxiliaryTimer::instance();
    nrf.setAddress(address);
    nrf.setFrequency(2450);
    nrf.setPacketLength(4);
    unsigned int nextWakeup=nominalPeriod-jitterAbsorption;
    unsigned int currentPeriod=nextWakeup;
    #ifdef RELATIVE_CLOCK
    rtc.setRelativeWakeup(currentPeriod);
    #endif //RELATIVE_CLOCK
    for(;;)
    {
        #ifdef RELATIVE_CLOCK
        #ifndef INTERACTIVE_ROOTNODE
        rtc.sleepAndWait();
        #else //INTERACTIVE_ROOTNODE
        rtc.wait();
        #endif //INTERACTIVE_ROOTNODE
        rtc.setRelativeWakeup(currentPeriod);
        #else //RELATIVE_CLOCK
        rtc.setAbsoluteWakeup(nextWakeup);
        #ifndef INTERACTIVE_ROOTNODE
        rtc.sleepAndWait();
        #else //INTERACTIVE_ROOTNODE
        rtc.wait();
        #endif //INTERACTIVE_ROOTNODE
        rtc.setAbsoluteWakeup(nextWakeup+jitterAbsorption);
        #endif //RELATIVE_CLOCK
        
        miosix::ledOn();
        nrf.setMode(Nrf24l01::TX);
        timer.initTimeoutTimer(0);
        #ifndef RELATIVE_CLOCK
        // To minimize jitter in the packet transmission time caused by the
        // variable time sleepAndWait() takes to restart the STM32 PLL an
        // additional wait is done here to absorb the jitter. This is only
        // possible with the absolute timer model.
        rtc.wait();
        #endif //RELATIVE_CLOCK
        miosix::ledOff(); //Falling edge signals TX reached time to send packet
        nrf.writePacket(packet);
        timer.waitForPacketOrTimeout(); //Wait for packet sent, sleeping the CPU
        nrf.endWritePacket();
        nrf.setMode(Nrf24l01::SLEEP);
        
        pthread_mutex_lock(&mutex);
        nextWakeup+=period;
        currentPeriod=period;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}

void applyDisturbance(float disturbance)
{
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond,&mutex);
    period+=static_cast<int>(disturbance/1000.0f*16384.0f+0.5f);
    pthread_mutex_unlock(&mutex);
    printf("new period=%6.4f\n",static_cast<float>(period)/16384.0f);
}

void sleepMinutes(int minutes)
{
    printf("sleeping for %d minutes\n",minutes);
    sleep(minutes*60);
}

int main()
{
    puts(experimentName);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr,1024);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_t thread;
    pthread_create(&thread,&attr,sendThread,0);
    pthread_attr_destroy(&attr);

    //This part takes 9 hours
    #ifdef RELATIVE_CLOCK
    float multiplier=1.5f;
    float disturbances[]={1.5f, 1.4f, 1.3f, 1.2f, 1.1f, 1.0f, 0.9f, 0.8f, 0.7f};
    #else //RELATIVE_CLOCK
    float multiplier=3.0f;
    float disturbances[]={3.0f, 2.9f, 2.8f, 2.7f, 2.6f, 2.5f, 2.4f, 2.3f, 2.2f};
    #endif //RELATIVE_CLOCK
    for(int i=0;i<sizeof(disturbances)/sizeof(disturbances[0]);i++)
    {
        sleepMinutes(30);
        applyDisturbance(disturbances[i]);
        sleepMinutes(30);
        applyDisturbance(-disturbances[i]);
    }
    
    //This sleep takes 1 hours
    sleepMinutes(60);

    //The interesting part of this test takes ~10 hours
    //and is equivalent to this Scilab code:
    //t=[0:0.007:5]; r=rand(t)*2-1; plot(3*r.*exp(-t));
    float oldDisturbance=0.0f;
    for(float t=0.0f;;t+=0.007f)
    {
        float r=rand();
        r/=RAND_MAX/2;
        r-=1.0f; //Now a random value between -1 and +1
        float newDisturbance=multiplier*r*exp(-t);
        applyDisturbance(newDisturbance-oldDisturbance);
        oldDisturbance=newDisturbance;
    }
}
