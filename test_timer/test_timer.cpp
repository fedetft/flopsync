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
    rtc.setValue(0xFFFFFFFFll - 5 * 16384/2);
    printf("Timestamp:  %016llX\n",rtc.getValue());
    for(;;)
    {        
        blueLed::low(); 
        rtc.setAbsoluteWakeupSleep((0xFFFFFFFFll-5*16384/2) + (16384*i)); //1 secondo
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
    
    rtc.setValue(0xFFFFFFFFll - 5 * 16384/2);
    printf("Timestamp:  %016llX\n",rtc.getValue());
    int i=1;
    bool timeout;
    for(;;)
    {        
        blueLed::low(); 
        rtc.setAbsoluteTimeout((0xFFFFFFFFll-5*16384/2) + (16384*i)); //un secondo
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
    unsigned long long timeWakeup;
    vht.setValue(0x1600000); //circa un secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    for(;;)
    {     
        timeWakeup = 0x16E3600ll*i;
        blueLed::low(); 
        vht.setAbsoluteWakeupSleep(timeWakeup); //un secondo
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
    
    vht.setValue(0x1600000); //circa un secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    for(;;)
    {        
        blueLed::low();
        vht.setAbsoluteWakeupWait(0x16E3600ll*i); //un secondo
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
    
    vht.setValue(0x1600000); //circa un secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    bool timeout;
    for(;;)
    {        
        blueLed::low(); 
        vht.setAbsoluteTimeout(0x16E3600ll*i); //un secondo
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
    
    vht.setValue(0x1600000); //circa un secondo 
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
//    typeTimer precInfo;
//    typeTimer lastInfo;
    greenLed::high();
    blueLed::high();
    VHT& vht=VHT::instance();
    printf("Clock monotonic....\n");
    
    
    for(;;)
    {        
        blueLed::low(); 
        prec = last;
        //precInfo = lastInfo;
        vht.setAbsoluteTimeout(0); 
        vht.waitForExtEventOrTimeout();
        blueLed::high(); 
        last = vht.getExtEventTimestamp();
        assert(last>prec);
//      lastInfo = vht.getInfo();
//        if(last<=prec)
//            printf("No monotonic clock:\n" ""
//                   "prec=%016llX  ts=%016llX  ovh=%016llX  cnt=%X  sr=%X cnt=%X  sr=%X\n"
//                   "last=%016llX  ts=%016llX  ovh=%016llX  cnt=%X  sr=%X cnt=%X  sr=%X\n",
//                    prec,precInfo.ts,precInfo.ovf,precInfo.cnt,precInfo.sr,precInfo.cntFirstUIF, precInfo.srFirstUIF,
//                    last,lastInfo.ts,lastInfo.ovf,lastInfo.cnt,lastInfo.sr,lastInfo.cntFirstUIF, lastInfo.srFirstUIF );
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
    testVhtWaitExtEvent();
    //testVhtMonotonic();
    //testVhtEvent();
}
