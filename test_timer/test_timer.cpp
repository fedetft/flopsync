/* 
 * File:   main.cpp
 * Author: gigi
 *
 * Created on January 20, 2014, 10:40 AM
 */
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include "../drivers/timer.h"
#include "test_config.h"

using namespace std;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;
typedef miosix::Gpio<GPIOC_BASE,9> greenLed;
typedef miosix::Gpio<GPIOA_BASE,0> userButton;

static void inline testRtc()
{
    greenLed::high();
    blueLed::high();
    Timer& rtc=Rtc::instance();
    
    int i=1;
    rtc.setValue(0xFFFFFFFFll - 5/2*rtcFreq); 
    printf("Timestamp:  %016llX\n",rtc.getValue());
    for(;;)
    {        
        blueLed::low(); 
        rtc.setAbsoluteWakeupSleep((0xFFFFFFFFll-5/2*rtcFreq) + (1*rtcFreq)); //1 secondo
        rtc.sleep();
        blueLed::high();  
        printf("Timestamp:  %016llX\n",rtc.getValue());
        i++;
    }
}

static void inline testRtcWaitExtEventOrTimeout()
{
    greenLed::high();
    blueLed::high();
    Timer& rtc=Rtc::instance();
    
    rtc.setValue(0xFFFFFFFFll - 5/2*rtcFreq);
    printf("Timestamp:  %016llX\n",rtc.getValue());
    int i=1;
    bool timeout;
    for(;;)
    {        
        blueLed::low(); 
        rtc.setAbsoluteTimeout((0xFFFFFFFFll-5/2*rtcFreq) + (rtcFreq*i)); //un secondo
        timeout=rtc.waitForExtEventOrTimeout();
        blueLed::high(); 
        timeout?
            printf("Timestamp:  %016llX\n",rtc.getValue()):
            printf("Packet   :  %016llX\n",rtc.getExtEventTimestamp());
        i++;
    }
}

static void inline testVhtSleep()
{
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    int i=1;
    vht.setValue(vhtFreq*1/20*0x1ll); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    for(;;)
    {    
        blueLed::low(); 
        vht.setAbsoluteWakeupSleep(vhtFreq*1/2+vhtFreq*i*0x1ll); //un secondo
        vht.sleep();
        blueLed::high(); 
        printf("Timestamp:  %016llX\n",vht.getValue());
        i++;
    }
}

static void inline testVhtGetPacketTimestamp()
{
    greenLed::high();
    blueLed::low();
    Timer& vht=VHT::instance();
    for(;;)
    {        
        miosix::delayMs(5000);
        printf("Timestamp:  %016llX\n",vht.getExtEventTimestamp());
    }
}


static void inline testVhtWait()
{
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    vht.setValue(vhtFreq*1/20*0x1ll); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    for(;;)
    {        
        blueLed::low();
        vht.setAbsoluteWakeupWait(vhtFreq*1/2+vhtFreq*i*0x1ll); //un secondo
        vht.wait();
        blueLed::high();
        printf("Timestamp:  %016llX\n",vht.getValue());
        i++;
    }
}

static void inline testVhtWaitExtEventOrTimeout()
{
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    vht.setValue(vhtFreq*1/2*0x1ll); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    bool timeout;
    for(;;)
    {        
        blueLed::low(); 
        vht.setAbsoluteTimeout(vhtFreq*1/2+vhtFreq*i*0x1ll); //un secondo
        timeout=vht.waitForExtEventOrTimeout();
        blueLed::high(); 
        timeout?
            printf("Timestamp:  %016llX\n",vht.getValue()):
            printf("Packet   :  %016llX\n",vht.getExtEventTimestamp());
        i++;
    }
}

static void inline testVhtWaitExtEvent()
{
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    vht.setValue(vhtFreq*1/2*0x1ll); //mezzo secondo
    printf("Timestamp:  %016llX\n",vht.getValue());
    bool timeout;
    for(;;)
    {        
        blueLed::low(); 
        vht.setAbsoluteTimeout(0); 
        timeout=vht.waitForExtEventOrTimeout();
        blueLed::high(); 
        timeout?
            printf("Timestamp:  %016llX\n",vht.getValue()):
            printf("Packet   :  %016llX\n",vht.getExtEventTimestamp());
    }
}

static void inline testVhtMonotonic()
{
    unsigned long long prec=0;
    unsigned long long last=0;

    #ifdef TIMER_DEBUG
    typeTimer precInfo;
    typeTimer lastInfo;
    #endif//TIMER_DEBUG
    greenLed::high();
    blueLed::high();
    VHT& vht=VHT::instance();
    printf("Clock monotonic....\n");
    for(;;)
    {        
        blueLed::low(); 
        prec = last;
        #ifdef TIMER_DEBUG
        //precInfo = lastInfo;
        #endif//TIMER_DEBUG
        vht.setAbsoluteTimeout(0); 
        vht.waitForExtEventOrTimeout();
        blueLed::high(); 
        last = vht.getExtEventTimestamp();
        #ifndef TIMER_DEBUG
        assert(last>prec);
        #else//TIMER_DEBUG
        lastInfo = vht.getInfo();
        if(last<=prec)
            printf("No monotonic clock:\n" ""
                   "prec=%016llX  ts=%016llX  ovh=%016llX  cnt=%X  sr=%X cnt=%X  sr=%X\n"
                   "last=%016llX  ts=%016llX  ovh=%016llX  cnt=%X  sr=%X cnt=%X  sr=%X\n",
                    prec,precInfo.ts,precInfo.ovf,precInfo.cnt,precInfo.sr,precInfo.cntFirstUIF, precInfo.srFirstUIF,
                    last,lastInfo.ts,lastInfo.ovf,lastInfo.cnt,lastInfo.sr,lastInfo.cntFirstUIF, lastInfo.srFirstUIF );
        #endif//TIMER DEBUG
    }
}

static void inline testVhtEvent()
{
    greenLed::high();
    for(;;)
    {
        blueLed::low();
        userButton::low();
        unsigned int delay = rand() % 3000 + 1;
        miosix::delayUs(delay);
        blueLed::high();
        userButton::high();
        miosix::delayUs(10);
    }
}

static void inline testVhtTriggerEvent()
{
    greenLed::high();
    Timer& vht=VHT::instance();
    vht.setValue(0); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    for(;;)
    {        
        vht.setAbsoluteTriggerEvent(vhtFreq*i); //un secondo
        vht.wait();
       
        printf("Timestamp:  %016llX\n",vht.getValue());
        //blueLed::low();
        i++;
    }
}

/*
 * 
 */
int main(int argc, char** argv) {

    puts("---------------Test timer----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
 
    //testRtc();
    //testRtcWaitExtEventOrTimeout();
    //testVhtSleep();
    //testVhtGetPacketTimestamp();
    //testVhtWait();
    //testVhtWaitExtEventOrTimeout();
    //testVhtWaitExtEvent();
    //testVhtMonotonic();
    //testVhtEvent();
    //testVhtTriggerEvent();
}
