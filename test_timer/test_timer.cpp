/* 
 * File:   main.cpp
 * Author: gigi
 *
 * Created on January 20, 2014, 10:40 AM
 */

#include <cstdlib>
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
        rtc.setAbsoluteWakeupWait((0xFFFFFFFFll-5*16384/2) + (16384*i)); //un secondo
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
        vht.setAbsoluteWakeupWait(0x16E3600ll*i); //un secondo
        timeout=vht.waitForExtEventOrTimeout();
        blueLed::high(); 
        timeout?
            printf("Timestamp:  %016llX\n",vht.getValue()):
            printf("Packet   :  %016llX\n",vht.getExtEventTimestamp());
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
 
    testRtc();
    //testRtcWaitExtEventOrTimeout();
    //testVhtSleep();
    //testVhtGetPacketTimestamp();
    //testVhtWait();
    //testVhtWaitExtEventOrTimeout();
}